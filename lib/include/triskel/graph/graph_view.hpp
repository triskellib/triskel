/*
 * A graph optimized for reading.
 * If the graph is modified, a new `graph view` needs to be generated
 */
#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "triskel/graph/igraph.hpp"

namespace triskel {
/// @brief A graph optimized for reading
struct GraphView {
    struct Edge;

    struct Node : public Identifiable<NodeTag> {
        explicit Node(NodeId id);

        [[nodiscard]] auto id() const -> NodeId final;

        [[nodiscard]] auto edges() const -> std::span<const Edge* const>;
        [[nodiscard]] auto child_edges() const -> std::span<const Edge* const>;
        [[nodiscard]] auto parent_edges() const -> std::span<const Edge* const>;

        [[nodiscard]] auto neighbors() const -> std::span<const Node* const>;
        [[nodiscard]] auto child_nodes() const -> std::span<const Node* const>;
        [[nodiscard]] auto parent_nodes() const -> std::span<const Node* const>;

        [[nodiscard]] auto is_root() const -> bool;

       private:
        NodeId id_;

        // The first half are parent edge, the second half are child edges
        std::vector<Edge*> edges_;
        int64_t edge_separator = 0;

        // The first half are parent nodes, the second half are child nodes
        std::vector<Node*> neighbors_;
        int64_t node_separator = 0;

        bool is_root_ = false;

        friend struct GraphView;
    };

    struct Edge : public Identifiable<EdgeTag> {
        explicit Edge(EdgeId id, Node* to, Node* from);

        [[nodiscard]] auto id() const -> EdgeId final;

        [[nodiscard]] auto to() const -> const Node&;
        [[nodiscard]] auto from() const -> const Node&;

        /// @brief Returns the other side of the edge
        [[nodiscard]] auto other(NodeId n) const -> const Node&;

       private:
        EdgeId id_;

        Node* to_;
        Node* from_;

        friend struct GraphView;
    };

    // TODO: can we do something cool with ownership to ensure the graph does
    // not get modified while the graph view exists ?
    explicit GraphView(const IGraph& g);

    /// @brief The root of this graph
    [[nodiscard]] auto root() const -> const Node&;

    /// @brief The nodes in this graph
    [[nodiscard]] auto nodes() const -> const std::vector<Node>&;

    /// @brief The edges in this graph
    [[nodiscard]] auto edges() const -> const std::vector<Edge>&;

    /// @brief The number of nodes in this graph
    [[nodiscard]] auto node_count() const -> size_t;

    /// @brief The number of edges in this graph
    [[nodiscard]] auto edge_count() const -> size_t;

   private:
    std::vector<Node> nodes_;
    std::vector<Edge> edges_;
};
}  // namespace triskel