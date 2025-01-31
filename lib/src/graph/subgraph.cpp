#include "triskel/graph/subgraph.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <ranges>
#include <span>
#include <vector>

#include <fmt/core.h>
#include <fmt/printf.h>

#include "triskel/graph/igraph.hpp"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace triskel;

// =============================================================================
// Subgraph Editor
// =============================================================================
SubGraphEditor::SubGraphEditor(SubGraph& g) : g_{g}, editor_{g.g_.editor()} {}

void SubGraphEditor::assert_present(EdgeId edge) {
    if (!std::ranges::binary_search(g_.edges_, edge)) {
        fmt::print("The edge {} was not in the subgraph\n", edge);
    }
}

void SubGraphEditor::assert_missing(EdgeId edge) {
    if (std::ranges::binary_search(g_.edges_, edge)) {
        fmt::print("The edge {} is in the subgraph\n", edge);
    }
}

void SubGraphEditor::select_node(NodeId node) {
    auto pos = std::ranges::lower_bound(g_.nodes_, node);

    if (pos == g_.nodes_.end() || *pos != node) {
        g_.nodes_.insert(pos, node);
    }

    select_edges(node);

    if (g_.root_ == NodeId::InvalidID) {
        make_root(node);
    }
}

void SubGraphEditor::unselect_node(NodeId node) {
    assert(g_.contains(node));

    auto pos = std::ranges::lower_bound(g_.nodes_, node);
    g_.nodes_.erase(pos);

    unselect_edges(node);
}

void SubGraphEditor::select_edges(NodeId node) {
    // The node in the complete graph
    auto n = g_.g_.get_node(node);

    for (const auto& edge : n.edges()) {
        if (g_.contains(edge.other(node))) {
            auto pos = std::ranges::lower_bound(g_.edges_, edge.id());

            if (pos == g_.edges_.end() || *pos != edge.id()) {
                g_.edges_.insert(pos, edge.id());
            }
        }
    }
}

void SubGraphEditor::unselect_edges(NodeId node) {
    // The node in the complete graph
    auto n = g_.g_.get_node(node);

    for (const auto& edge : n.edges()) {
        if (std::ranges::binary_search(g_.nodes_, edge.other(n).id())) {
            assert_present(edge);

            auto pos = std::ranges::lower_bound(g_.edges_, edge.id());
            g_.edges_.erase(pos);
        }
    }
}

void SubGraphEditor::make_root(NodeId node) {
    g_.root_ = node;
}

auto SubGraphEditor::make_node() -> Node {
    auto node = editor_.make_node();
    select_node(node);
    // We want the object in the subgraph
    return g_.get_node(node.id());
}

void SubGraphEditor::remove_node(NodeId node) {
    assert(g_.contains(node));
    editor_.remove_node(node);
}

auto SubGraphEditor::make_edge(NodeId from, NodeId to) -> Edge {
    assert(g_.contains(from) && g_.contains(to));
    auto edge = editor_.make_edge(from, to);
    // Select the edge we just added
    // TODO: just add a single edge
    select_edges(from);
    // We want the object in the subgraph
    return g_.get_edge(edge.id());
}

void SubGraphEditor::edit_edge(EdgeId edge, NodeId new_from, NodeId new_to) {
    assert(g_.contains(new_from) && g_.contains(new_to));
    editor_.edit_edge(edge, new_from, new_to);

    select_edges(new_from);
}

void SubGraphEditor::remove_edge(EdgeId edge) {
    assert_present(edge);
    editor_.remove_edge(edge);
}

void SubGraphEditor::push() {
    editor_.push();
}

void SubGraphEditor::pop() {
    editor_.pop();

    // Delete edges
    g_.edges_ = g_.edges_  //
                | std::ranges::views::filter([&](const auto& id) {
                      return static_cast<size_t>(id) < g_.g_.max_edge_id();
                  })  //
                | std::ranges::to<std::vector<EdgeId>>();

    // Delete nodes
    g_.nodes_ = g_.nodes_  //
                | std::ranges::views::filter([&](const auto& id) {
                      return static_cast<size_t>(id) < g_.g_.max_node_id();
                  })  //
                | std::ranges::to<std::vector<NodeId>>();
}

void SubGraphEditor::commit() {
    editor_.commit();
}

// =============================================================================
// Subgraph
// =============================================================================
SubGraph::SubGraph(Graph& g)
    : root_{NodeId::InvalidID}, g_{g}, editor_{*this} {}

auto SubGraph::root() const -> Node {
    return get_node(root_);
}

auto SubGraph::nodes() const -> std::vector<Node> {
    return nodes_  //
           | std::ranges::views::filter([&](const NodeId& id) {
                 return !g_.get_node_data(id).deleted;
             })  //
           | std::ranges::views::transform(
                 [&](const NodeId& id) { return get_node(id); })  //
           | std::ranges::to<std::vector<Node>>();
}

auto SubGraph::edges() const -> std::vector<Edge> {
    return edges_  //
           | std::ranges::views::filter([&](const EdgeId& id) {
                 return !g_.get_edge_data(id).deleted;
             })  //
           | std::ranges::views::transform(
                 [&](const EdgeId& id) { return get_edge(id); })  //
           | std::ranges::to<std::vector<Edge>>();
}

auto SubGraph::get_node(NodeId id) const -> Node {
    // assert(std::ranges::contains(nodes_, id));
    return Node{*this, g_.get_node_data(id)};
}

auto SubGraph::get_edge(EdgeId id) const -> Edge {
    // assert(std::ranges::contains(edges_, id));
    return Edge{*this, g_.get_edge_data(id)};
}

auto SubGraph::get_nodes(const std::span<const NodeId>& ids) const
    -> std::vector<Node> {
    return ids  //
           | std::ranges::views::filter([&](const NodeId& id) {
                 return !g_.get_node_data(id).deleted;
             })  //
           | std::ranges::views::filter([&](const NodeId& id) {
                 return std::ranges::contains(nodes_, id);
             })  //
           | this->node_view();
}

auto SubGraph::get_edges(const std::span<const EdgeId>& ids) const
    -> std::vector<Edge> {
    return ids  //
           | std::ranges::views::filter([&](const EdgeId& id) {
                 return !g_.get_edge_data(id).deleted;
             })  //
           | std::ranges::views::filter([&](const EdgeId& id) {
                 return std::ranges::contains(edges_, id);
             })  //
           | this->edge_view();
}

auto SubGraph::max_node_id() const -> size_t {
    return g_.max_node_id();
}

auto SubGraph::max_edge_id() const -> size_t {
    return g_.max_edge_id();
}

auto SubGraph::node_count() const -> size_t {
    return nodes().size();
}

auto SubGraph::edge_count() const -> size_t {
    return edges().size();
}

auto SubGraph::editor() -> SubGraphEditor& {
    return editor_;
}

auto SubGraph::contains(NodeId node) -> bool {
    return std::ranges::binary_search(nodes_, node) &&
           !g_.get_node_data(node).deleted;
}

auto SubGraph::contains(EdgeId edge) -> bool {
    return std::ranges::binary_search(edges_, edge) &&
           !g_.get_edge_data(edge).deleted;
}
