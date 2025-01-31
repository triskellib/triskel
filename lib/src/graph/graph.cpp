#include "triskel/graph/graph.hpp"

#include <cassert>
#include <cstddef>
#include <ranges>
#include <stack>
#include <vector>

#include "triskel/graph/igraph.hpp"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace triskel;

// =============================================================================
// GraphEditor
// =============================================================================
GraphEditor::GraphEditor(Graph& g) : g_{g} {}

GraphEditor::~GraphEditor() {
    assert(frames.empty());
}

auto GraphEditor::make_node() -> Node {
    g_.data_.nodes.push_back(NodeData{
        .id = NodeId{g_.data_.nodes.size()}, .edges = {}, .deleted = false});
    const auto& n = g_.data_.nodes.back();

    // Sets the root if it is not defined
    if (g_.data_.root == NodeId::InvalidID) {
        g_.data_.root = n.id;
    }

    frame().created_nodes_count += 1;
    return g_.get_node(n.id);
}

void GraphEditor::remove_node(NodeId id) {
    const auto& node = g_.get_node(id);
    auto& n          = g_.get_node_data(id);

    // Not handled
    assert(!node.is_root());

    for (const auto& edge : node.edges()) {
        remove_edge(edge);
    }

    n.deleted = true;
    frame().deleted_nodes.push(n.id);
}

auto GraphEditor::make_edge(NodeId from, NodeId to) -> Edge {
    g_.data_.edges.push_back(
        EdgeData{.id = EdgeId{g_.data_.edges.size()}, .from = from, .to = to});
    const auto& e = g_.data_.edges.back();

    g_.get_node_data(from).edges.push_back(e.id);
    g_.get_node_data(to).edges.push_back(e.id);

    frame().created_edges.push(e.id);
    return g_.get_edge(e.id);
}

void GraphEditor::remove_edge(EdgeId edge) {
    auto& e   = g_.get_edge_data(edge);
    e.deleted = true;

    auto& from = g_.get_node_data(e.from);
    auto& to   = g_.get_node_data(e.to);

    std::erase(from.edges, edge);
    std::erase(to.edges, edge);

    frame().deleted_edges.push(e.id);
}

void GraphEditor::edit_edge(EdgeId edge, NodeId new_from, NodeId new_to) {
    auto& e = g_.get_edge_data(edge);
    frame().modified_edges.push(e);

    auto& old_from = g_.get_node_data(e.from);
    auto& old_to   = g_.get_node_data(e.to);

    auto& from = g_.get_node_data(new_from);
    auto& to   = g_.get_node_data(new_to);

    std::erase(old_from.edges, e.id);
    std::erase(old_to.edges, e.id);

    from.edges.push_back(e.id);
    to.edges.push_back(e.id);

    e.from = new_from;
    e.to   = new_to;
}

auto GraphEditor::frame() -> Frame& {
    assert(!frames.empty() &&
           "Using the graph editor without a frame. You need to call `push` "
           "before using the editor");
    return frames.top();
}

void GraphEditor::push() {
    frames.push(Frame{});
}

void GraphEditor::pop() {
    auto& f = frames.top();

    // The order here is important, otherwise we might modify deleted elements
    // Revert edited edges
    while (!f.modified_edges.empty()) {
        auto e = f.modified_edges.top();
        f.modified_edges.pop();

        auto& edge = g_.get_edge_data(e.id);

        auto& new_from = g_.get_node_data(edge.from);
        auto& new_to   = g_.get_node_data(edge.to);

        auto& from = g_.get_node_data(e.from);
        auto& to   = g_.get_node_data(e.to);

        std::erase(new_from.edges, e.id);
        std::erase(new_to.edges, e.id);

        from.edges.push_back(e.id);
        to.edges.push_back(e.id);

        edge.from = e.from;
        edge.to   = e.to;
    }

    // Revert removed edges
    while (!f.deleted_edges.empty()) {
        auto eid = f.deleted_edges.top();
        f.deleted_edges.pop();

        auto& e   = g_.get_edge_data(eid);
        e.deleted = false;

        auto& from = g_.get_node_data(e.from);
        auto& to   = g_.get_node_data(e.to);

        from.edges.push_back(eid);
        to.edges.push_back(eid);
    }

    // Revert removed nodes
    while (!f.deleted_nodes.empty()) {
        auto nid = f.deleted_nodes.top();
        f.deleted_nodes.pop();

        g_.get_node_data(nid).deleted = false;
    }

    // Revert created edges
    auto created_edge_count = 0;
    while (!f.created_edges.empty()) {
        auto eid = f.created_edges.top();
        f.created_edges.pop();

        auto& e = g_.get_edge_data(eid);

        auto& from = g_.get_node_data(e.from);
        auto& to   = g_.get_node_data(e.to);

        std::erase(from.edges, eid);
        std::erase(to.edges, eid);

        created_edge_count++;
    }

    for (size_t i = 0; i < created_edge_count; ++i) {
        g_.data_.edges.pop_back();
    }

    // Revert created nodes
    for (size_t i = 0; i < f.created_nodes_count; ++i) {
        g_.data_.nodes.pop_back();
    }

    frames.pop();
}

void GraphEditor::commit() {
    // Deletes all changes
    frames = std::stack<Frame>();
}

// =============================================================================
// Graph
// =============================================================================

Graph::Graph()
    : data_{.root = NodeId::InvalidID, .nodes = {}, .edges = {}},
      editor_{*this} {}

auto Graph::root() const -> Node {
    return get_node(data_.root);
}

auto Graph::nodes() const -> std::vector<Node> {
    return data_.nodes  //
           | std::ranges::views::filter(
                 [&](const NodeData& n) { return !n.deleted; })  //
           | std::ranges::views::transform(
                 [&](const NodeData& n) { return get_node(n.id); })  //
           | std::ranges::to<std::vector<Node>>();
}

auto Graph::edges() const -> std::vector<Edge> {
    return data_.edges  //
           | std::ranges::views::filter(
                 [&](const EdgeData& e) { return !e.deleted; })  //
           | std::ranges::views::transform(
                 [&](const EdgeData& e) { return get_edge(e.id); })  //
           | std::ranges::to<std::vector<Edge>>();
}

auto Graph::get_node(NodeId id) const -> Node {
    assert(id != NodeId::InvalidID);
    return Node{*this, get_node_data(id)};
}

auto Graph::get_edge(EdgeId id) const -> Edge {
    assert(id != EdgeId::InvalidID);
    return Edge{*this, get_edge_data(id)};
}

auto Graph::get_edge_data(EdgeId id) -> EdgeData& {
    return data_.edges[static_cast<size_t>(id)];
}

auto Graph::get_edge_data(EdgeId id) const -> const EdgeData& {
    return data_.edges[static_cast<size_t>(id)];
}

auto Graph::get_node_data(NodeId id) -> NodeData& {
    return data_.nodes[static_cast<size_t>(id)];
}

auto Graph::get_node_data(NodeId id) const -> const NodeData& {
    return data_.nodes[static_cast<size_t>(id)];
}

auto Graph::max_node_id() const -> size_t {
    return data_.nodes.size();
}

auto Graph::max_edge_id() const -> size_t {
    return data_.edges.size();
}

auto Graph::node_count() const -> size_t {
    return nodes().size();
}

auto Graph::edge_count() const -> size_t {
    return edges().size();
}

auto Graph::editor() -> GraphEditor& {
    return editor_;
}