#include "triskel/graph/igraph.hpp"

#include <cassert>
#include <ranges>
#include <span>
#include <string>
#include <vector>

#include <fmt/core.h>
#include <fmt/format.h>

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace triskel;

// =============================================================================
// Nodes
// =============================================================================
auto Node::operator=(const Node& node) -> Node& {
    if (this == &node) {
        return *this;
    }

    assert(&g_ == &node.g_);
    n_ = node.n_;
    return *this;
}

auto Node::id() const -> NodeId {
    return n_->id;
}

auto Node::edges() const -> std::vector<Edge> {
    return g_.get_edges(n_->edges);
}

auto Node::child_edges() const -> std::vector<Edge> {
    return edges()  //
           | std::ranges::views::filter(
                 [&](const Edge& e) { return e.from() == *this; })  //
           | std::ranges::to<std::vector<Edge>>();
}

auto Node::parent_edges() const -> std::vector<Edge> {
    return edges()  //
           | std::ranges::views::filter(
                 [&](const Edge& e) { return e.to() == *this; })  //
           | std::ranges::to<std::vector<Edge>>();
}

auto Node::child_nodes() const -> std::vector<Node> {
    return edges()  //
           | std::ranges::views::filter(
                 [&](const Edge& e) { return e.from() == *this; })  //
           | std::ranges::views::transform(
                 [&](const Edge& e) { return e.to(); })  //
           | std::ranges::to<std::vector<Node>>();
}

auto Node::parent_nodes() const -> std::vector<Node> {
    return edges()  //
           | std::ranges::views::filter(
                 [&](const Edge& e) { return e.to() == *this; })  //
           | std::ranges::views::transform(
                 [&](const Edge& e) { return e.from(); })  //
           | std::ranges::to<std::vector<Node>>();
}

auto Node::neighbors() const -> std::vector<Node> {
    return edges()  //
           | std::ranges::views::transform(
                 [&](const Edge& e) { return e.other(*this); })  //
           | std::ranges::to<std::vector<Node>>();
}

auto Node::is_root() const -> bool {
    return *this == g_.root();
}

// =============================================================================
// Edges
// =============================================================================
Edge::Edge(const IGraph& g, const EdgeData& e) : g_{g}, e_{&e} {}

auto Edge::operator=(const Edge& other) -> Edge& {
    if (this == &other) {
        return *this;
    }

    assert(&g_ == &other.g_);
    e_ = other.e_;
    return *this;
}

auto Edge::id() const -> EdgeId {
    return e_->id;
}

auto Edge::from() const -> Node {
    return g_.get_node(e_->from);
}

auto Edge::to() const -> Node {
    return g_.get_node(e_->to);
}

auto Edge::other(NodeId n) const -> Node {
    if (n == to()) {
        return from();
    }

    return to();
}

// =============================================================================
// Graphs
// =============================================================================
auto IGraph::get_nodes(const std::span<const NodeId>& ids) const
    -> std::vector<Node> {
    return ids | node_view();
}

auto IGraph::get_edges(const std::span<const EdgeId>& ids) const
    -> std::vector<Edge> {
    return ids | edge_view();
}

// =============================================================================
// Formats
// =============================================================================
auto triskel::format_as(const Node& n) -> std::string {
    return fmt::format("n{}", n.id());
}

auto triskel::format_as(const Edge& e) -> std::string {
    return fmt::format("{} -> {}", e.from(), e.to());
}

auto triskel::format_as(const IGraph& g) -> std::string {
    auto s = std::string{"digraph G {\n"};

    for (auto node : g.nodes()) {
        s += fmt::format("{}\n", node);
    }

    s += "\n";

    for (auto edge : g.edges()) {
        s += fmt::format("{}\n", edge);
    }

    s += "}\n";

    return s;
}
