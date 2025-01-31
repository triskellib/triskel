#pragma once

#include <cstdint>
#include <vector>

#include "triskel/analysis/patriarchal.hpp"
#include "triskel/utils/attribute.hpp"

namespace triskel {

struct Node;
struct Graph;
struct Edge;

struct DFSAnalysis : public Patriarchal {
    explicit DFSAnalysis(const IGraph& g);

    ~DFSAnalysis() override = default;

    /// @brief Returns the graph nodes in DFS order
    auto nodes() -> std::vector<Node>;

    /// @brief Is an edge a back edge
    auto is_backedge(const Edge& e) -> bool;

    /// @brief Is an edge a tree edge
    auto is_tree(const Edge& e) -> bool;

    /// @brief Is an edge a cross edge
    auto is_cross(const Edge& e) -> bool;

    /// @brief Is an edge a forward edge
    auto is_forward(const Edge& e) -> bool;

    /// @brief The index of a node in the dfs ordered set of nodes
    auto dfs_num(const Node& n) -> size_t;

    /// @brief Prints the type of an edge
    // Mainly for debugging
    auto dump_type(const Edge& e) const -> std::string;

   private:
    enum class EdgeType : uint8_t { None, Tree, Back, Cross, Forward };

    /// @brief Was this node previously visited in `dfs`
    auto was_visited(const Node& node) -> bool;

    /// @brief Recursive depth first search helper function
    void dfs(const Node& node);

    /// @brief Types the graphs edges
    void type_edges();

    const IGraph& g_;

    NodeAttribute<size_t> dfs_nums_;

    EdgeAttribute<EdgeType> types_;

    std::vector<NodeId> nodes_;
};
}  // namespace triskel