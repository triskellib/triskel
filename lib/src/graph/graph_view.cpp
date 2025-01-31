#include "triskel/graph/graph_view.hpp"

#include <cassert>
#include <cstddef>
#include <span>
#include <vector>

#include "triskel/graph/igraph.hpp"
#include "triskel/utils/attribute.hpp"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace triskel;

using NodeView = GraphView::Node;
using EdgeView = GraphView::Edge;

NodeView::Node(NodeId id) : id_{id} {}

auto NodeView::id() const -> NodeId {
    return id_;
}

auto NodeView::edges() const -> std::span<const EdgeView* const> {
    return edges_;
}

auto NodeView::parent_edges() const -> std::span<const EdgeView* const> {
    return {edges_.begin(), edges_.begin() + edge_separator};
}
auto NodeView::child_edges() const -> std::span<const EdgeView* const> {
    return {edges_.begin() + edge_separator, edges_.end()};
}

auto NodeView::neighbors() const -> std::span<const NodeView* const> {
    return neighbors_;
}
auto NodeView::parent_nodes() const -> std::span<const NodeView* const> {
    return {neighbors_.begin(), neighbors_.begin() + node_separator};
}
auto NodeView::child_nodes() const -> std::span<const NodeView* const> {
    return {neighbors_.begin() + node_separator, neighbors_.end()};
}

auto NodeView::is_root() const -> bool {
    return is_root_;
}

// =============================================================================

EdgeView::Edge(EdgeId id, NodeView* to, NodeView* from)
    : id_{id}, to_{to}, from_{from} {}

auto EdgeView::id() const -> EdgeId {
    return id_;
}

auto EdgeView::to() const -> const NodeView& {
    return *to_;
}

auto EdgeView::from() const -> const NodeView& {
    return *from_;
}

auto EdgeView::other(NodeId n) const -> const NodeView& {
    if (n == to()) {
        return from();
    }
    assert(n == from());
    return to();
}

// =============================================================================

GraphView::GraphView(const IGraph& g) {
    nodes_.reserve(g.node_count());
    edges_.reserve(g.edge_count());

    NodeAttribute<NodeView*> node_map{g, nullptr};
    EdgeAttribute<EdgeView*> edge_map{g, nullptr};

    for (const auto& node : g.nodes()) {
        nodes_.emplace_back(node);
        node_map.set(node, &nodes_.back());
    }

    for (const auto& edge : g.edges()) {
        edges_.emplace_back(edge, node_map.get(edge.to()),
                            node_map.get(edge.from()));
        edge_map.set(edge, &edges_.back());
    }

    for (const auto& node : g.nodes()) {
        auto& node_view = *node_map.get(node);

        for (const auto& parent : node.parent_nodes()) {
            node_view.neighbors_.push_back(node_map.get(parent));
            node_view.node_separator++;
        }
        for (const auto& child : node.child_nodes()) {
            node_view.neighbors_.push_back(node_map.get(child));
        }

        for (const auto& parent : node.parent_edges()) {
            node_view.edges_.push_back(edge_map.get(parent));
            node_view.edge_separator++;
        }
        for (const auto& child : node.child_edges()) {
            node_view.edges_.push_back(edge_map.get(child));
        }
    }
}

auto GraphView::root() const -> const Node& {
    return nodes_.front();
}

auto GraphView::nodes() const -> const std::vector<Node>& {
    return nodes_;
}

auto GraphView::edges() const -> const std::vector<Edge>& {
    return edges_;
}

auto GraphView::node_count() const -> size_t {
    return nodes_.size();
}

auto GraphView::edge_count() const -> size_t {
    return nodes_.size();
}