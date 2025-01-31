#pragma once

#include <cstdint>
#include <vector>

#include "triskel/analysis/patriarchal.hpp"
#include "triskel/graph/igraph.hpp"
#include "triskel/utils/attribute.hpp"

namespace triskel {

struct UnorderedDFSAnalysis : public Patriarchal {
    enum class EdgeType : uint8_t { None, Tree, Back };

    explicit UnorderedDFSAnalysis(const IGraph& g);

    ~UnorderedDFSAnalysis() override = default;

    /// @brief Returns the graph nodes in DFS order
    auto nodes() -> std::vector<Node>;

    /// @brief Is an edge a back edge
    /// In an unordered graph, a backedge is a ?
    auto is_backedge(const Edge& e) -> bool;

    /// @brief Make an edge a back edge
    void set_backedge(const Edge& e);

    /// @brief Is an edge a tree edge
    auto is_tree(const Edge& e) -> bool;

    /// @brief The index of a node in the dfs ordered set of nodes
    auto dfs_num(const Node& n) -> size_t;

   private:
    /// @brief Was this node previously visited in `udfs`
    auto was_visited(const Node& node) -> bool;

    /// @brief Recursive unordered depth first search helper function
    void udfs(const Node& node);

    const IGraph& g_;

    NodeAttribute<size_t> dfs_nums_;

    EdgeAttribute<EdgeType> types_;

    std::vector<NodeId> nodes_;
};

}  // namespace triskel
