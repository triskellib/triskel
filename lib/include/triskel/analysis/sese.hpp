#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "triskel/analysis/udfs.hpp"
#include "triskel/graph/graph.hpp"
#include "triskel/graph/igraph.hpp"
#include "triskel/utils/attribute.hpp"
#include "triskel/utils/tree.hpp"

namespace triskel {

void cycle_equiv(IGraph& g);

// Single entry single exit
struct SESE {
    explicit SESE(Graph& g);

    struct SESERegionData {
        SESERegionData() = default;

        EdgeId entry_edge;
        NodeId entry_node;

        EdgeId exit_edge;
        NodeId exit_node;

        std::vector<NodeId> nodes;
    };
    using SESERegion = Tree<SESERegionData>::Node;

    /// @brief The program structure tree
    Tree<SESERegionData> regions;

    NodeAttribute<SESERegion*> node_regions;

    // index of edge's cycle equivalence set
    EdgeAttribute<size_t> classes_;

    [[nodiscard]] auto get_region(const Node& node) const -> SESERegion& {
        return *node_regions.get(node);
    }

   private:
    using Bracket     = EdgeId;
    using BracketList = std::vector<Bracket>;

    struct NodeClass {
        NodeId id;
        EdgeId edge;
        size_t edge_class;
    };

    /// @brief  Adds a single exit node and links it to the start
    void preprocess_graph();

    /// @brief Is the edge `edge` a backedge from `from` to `to`
    [[nodiscard]] auto is_backedge_stating_from(const Edge& edge,
                                                const Node& from,
                                                const Node& to) -> bool;

    /// @brief Calculates hi0
    /// hi0 is the ?
    [[nodiscard]] auto get_hi0(const Node& node) -> size_t;

    /// @brief Calculates hi1
    [[nodiscard]] auto get_hi1(const Node& node) -> size_t;

    /// @brief Calculates hi2
    [[nodiscard]] auto get_hi2(const Node& node, size_t hi1) -> size_t;

    void create_capping_backedge(const Node& node,
                                 BracketList& blist,
                                 size_t hi2);

    [[nodiscard]] auto get_parent_tree_edge(const Node& node) -> Edge;

    void determine_class(const Node& node, BracketList& blist);

    void determine_region_boundaries(
        const Node& node,
        NodeAttribute<bool>& visited,
        const std::vector<NodeClass>& visited_class);

    /// @brief Build the program structure tree using DFS once we know the entry
    /// and exit edges of each region
    void construct_program_structure_tree(const Node& node,
                                          SESERegion* current_region,
                                          NodeAttribute<bool>& visited);

    /// @brief The graph for which we are identifying SESE regions
    IGraph& g_;

    size_t edge_class = 1;
    [[nodiscard]] auto new_class() -> size_t;

    /// @brief An unordered depth first search of g
    std::unique_ptr<UnorderedDFSAnalysis> udfs_;

    // descendant node of n
    NodeAttribute<size_t> his_;

    // node's bracket list
    NodeAttribute<BracketList> blists_;

    // size of bracket set when e was most recently the topmost edge in a
    // bracket set
    EdgeAttribute<size_t> recent_sizes_;

    // equivalence class number of ree edge for which e was most recently the
    // topmost bracket
    EdgeAttribute<size_t> recent_classes_;

    std::vector<EdgeId> capping_backedges_;

    EdgeAttribute<bool> entry_edge_;
    EdgeAttribute<bool> exit_edge_;
};
}  // namespace triskel