#pragma once

#include <cstddef>
#include <cstdint>

#include "triskel/graph/igraph.hpp"
#include "triskel/utils/attribute.hpp"

namespace triskel {

struct LowLim {
    /// @brief The least number of any descendant in the search
    size_t low;

    /// @brief The post order traversal number
    size_t lim;
};

struct SpanningTree {
    explicit SpanningTree(const IGraph& g);

    void init_rank();

    void tight_tree_recurs(NodeId n);

    /// @brief Constructs a tight spanning tree and returns the number of nodes
    /// in that tree
    auto tight_tree() -> size_t;

    void build_postorder_tree_rec(const Node& node, NodeId parent, size_t& lim);

    void build_postorder_tree();

    [[nodiscard]] auto lim(NodeId n) const -> size_t;
    [[nodiscard]] auto low(NodeId n) const -> size_t;

    [[nodiscard]] auto slack(EdgeId e) const -> size_t;
    [[nodiscard]] auto is_tight(EdgeId e) const -> bool;

    /// @brief returns 1 for nodes in the head region and -1 for nodes in the
    /// tail region
    /// Testing a node w for an edge u -> v
    // [[nodiscard]] auto get_component(const Edge& e, NodeId w) const ->
    // int32_t;
    [[nodiscard]] auto get_component(const Edge& e, NodeId w) const -> int32_t;

    void init_cut_values(const Node& node, EdgeId tree_edge);
    void init_cut_values();

    auto enter_edge(const Edge& e) -> Edge;

    void update_ranks_rec(const Node& node, NodeId parent, size_t delta);

    void exchange(const Edge& e, const Edge& f);

    auto get_incident_edge() -> Edge;
    void feasible_tree();

    auto normalize_ranks() -> size_t;

    auto leave_edge() -> EdgeId;

    void dump();

    std::deque<NodeId> nodes_;
    std::deque<EdgeId> edges_;

    NodeAttribute<bool> in_tree_;

    EdgeAttribute<bool> e_in_tree_;
    EdgeAttribute<int64_t> cut_;

    NodeAttribute<LowLim> postorder_tree_;

    NodeAttribute<size_t> ranks_;
    const IGraph& g;
};

}  // namespace triskel