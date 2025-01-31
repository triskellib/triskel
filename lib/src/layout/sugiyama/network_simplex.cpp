// Based on https://www.graphviz.org/documentation/TSE93.pdf

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <deque>
#include <memory>
#include <vector>
#include "triskel/graph/igraph.hpp"
#include "triskel/layout/sugiyama/layer_assignement.hpp"
#include "triskel/utils/attribute.hpp"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace triskel;

namespace {

struct SpanningNode {
    EdgeId tree_edge;
    NodeId node;

    SpanningNode* parent;
    std::vector<SpanningNode*> children;
};

struct SpanningTree {
    explicit SpanningTree(const IGraph& g)
        : g{g}, ranks{g, static_cast<size_t>(-1)}, in_tree{g, false} {}

    [[nodiscard]] auto slack(EdgeId e) const -> size_t {
        auto edge = g.get_edge(e);

        return ranks.get(edge.to()) - ranks.get(edge.from()) - 1;
    }

    [[nodiscard]] auto is_tight(EdgeId e) const -> bool {
        return slack(e) == 0;
    }

    auto init_rank() {
        const auto nodes = g.nodes();

        ranks.set(g.root(), 0);
        size_t found_nodes = 1;
        size_t rank        = 1;

        while (found_nodes < nodes.size()) {
            for (const auto& node : nodes) {
                if (ranks.get(node) < rank) {
                    continue;
                }

                for (const auto& parent : node.parent_nodes()) {
                    if (ranks.get(parent) >= rank) {
                        goto continue_loop;
                    }
                }

                ranks.set(node, rank);
                found_nodes += 1;

            continue_loop:
            }

            rank += 1;
        }
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    void tight_tree_recurs(SpanningNode& spanning_node) {
        const auto& node = g.get_node(spanning_node.node);
        in_tree.set(node, true);

        for (const auto& edge : node.edges()) {
            auto neighbor = edge.other(node);

            if (in_tree.get(neighbor)) {
                continue;
            }

            // Only keep tight edges
            if (!is_tight(edge)) {
                continue;
            }

            tree.push_back(SpanningNode{.tree_edge = EdgeId::InvalidID,
                                        .node      = neighbor,
                                        .parent    = &spanning_node,
                                        .children  = {}});

            spanning_node.children.push_back(&tree.back());

            tight_tree_recurs(tree.back());
        }
    }

    /// @brief Constructs a tight spanning tree and returns the number of nodes
    /// in that tree
    auto tight_tree() -> size_t {
        for (auto& spanning_node : tree) {
            tight_tree_recurs(spanning_node);
        }

        return tree.size();
    }

    auto feasible_tree() {
        init_rank();

        // Adds the root to the tree
        tree.push_back(SpanningNode{.tree_edge = EdgeId::InvalidID,
                                    .node      = g.root(),
                                    .parent    = nullptr,
                                    .children  = {}});

        const auto& nodes = g.nodes();
        while (tight_tree() < nodes.size()) {
            auto e           = EdgeId::InvalidID;
            size_t min_slack = -1;

            for (const auto& node : nodes) {
                if (in_tree.get(node)) {
                    continue;
                }

                for (const auto& edge : node.edges()) {
                    auto neighbor = edge.other(node);

                    // We want incident edges
                    if (!in_tree.get(neighbor)) {
                        continue;
                    }

                    auto edge_slack = slack(edge);
                    assert(edge_slack != 1);

                    if (edge_slack < min_slack) {
                        min_slack = edge_slack;
                        e         = edge;
                    }
                }
            }

            auto edge  = g.get_edge(e);
            auto delta = slack(e);

            if (in_tree.get(edge.to())) {
                delta = -delta;
            }

            for (const auto& spanning_node : tree) {
                auto node = spanning_node.node;
                auto rank = ranks.get(node);
                ranks.set(node, rank + delta);
            }
        }

        // INIT CUT VALUES
    }

    auto normalize_ranks() -> size_t {
        size_t max_rank = 0;

        for (const auto& node : g.nodes()) {
            max_rank = std::max(max_rank, ranks.get(node));
        }

        size_t rank_count = 0;
        for (const auto& node : g.nodes()) {
            auto rank = max_rank - ranks.get(node) + 1;
            ranks.set(node, rank);
            rank_count = std::max(rank_count, rank);
        }

        return rank_count + 1;
    }

    std::deque<SpanningNode> tree;

    NodeAttribute<bool> in_tree;

    NodeAttribute<size_t> ranks;
    const IGraph& g;
};

}  // namespace

auto triskel::network_simplex(const IGraph& graph)
    -> std::unique_ptr<LayerAssignment> {
    auto spanning_tree = SpanningTree(graph);
    spanning_tree.feasible_tree();
    auto rank_count = spanning_tree.normalize_ranks();

    // TODO: the rest of network simplex. This works already really well though

    return std::make_unique<LayerAssignment>(spanning_tree.ranks, rank_count);
}
