#include "triskel/analysis/sese.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <ranges>
#include <vector>

#include "triskel/analysis/udfs.hpp"
#include "triskel/graph/graph.hpp"
#include "triskel/graph/igraph.hpp"
#include "triskel/utils/attribute.hpp"
#include "triskel/utils/tree.hpp"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace triskel;

namespace {
using Bracket     = EdgeId;
using BracketList = std::vector<Bracket>;

// TODO: Our implementation is not linear, we should fix that
namespace bl {

auto create() -> BracketList {
    return {};
}

auto size(const BracketList& bl) -> size_t {
    return bl.size();
}

void push(BracketList& bl, Bracket e) {
    bl.push_back(e);
}

auto top(BracketList& bl) -> Bracket {
    return bl.back();
}

void del(BracketList& bl, Bracket e) {
    std::erase(bl, e);
}

/// @brief Adds the elements of bl2 at the end of bl1
auto cat(BracketList& bl1, BracketList& bl2) {
    bl1.insert(bl1.end(), bl2.begin(), bl2.end());
}

}  // namespace bl

}  // namespace

auto SESE::new_class() -> size_t {
    edge_class++;
    return edge_class;
}

SESE::SESE(Graph& g)
    : g_{g},
      his_{g, static_cast<size_t>(-1)},
      blists_{g, {}},
      classes_{g, 0},
      entry_edge_{g, false},
      exit_edge_{g, false},
      recent_sizes_{g, 0},
      recent_classes_{g, 0},
      node_regions{g, nullptr} {
    auto& ge = g_.editor();
    ge.push();

    // TODO: check that the graph is strongly connected.
    // This can be done with a depth first search

    // Adds a single exit node and links it to the start
    preprocess_graph();

    udfs_ = std::make_unique<UnorderedDFSAnalysis>(g);

    for (const auto& n : udfs_->nodes() | std::views::reverse) {
        const size_t hi0 = get_hi0(n);
        const size_t hi1 = get_hi1(n);
        his_.set(n, std::min(hi0, hi1));
        const size_t hi2 = get_hi2(n, hi1);

        auto& blist = blists_.get(n);

        for (const auto& child : udfs_->children(n)) {
            bl::cat(blist, blists_.get(child));
        }

        for (auto d : capping_backedges_) {
            auto b     = g_.get_edge(d);
            auto child = b.other(n);
            if (udfs_->succeed(child, n)) {
                bl::del(blist, b.id());
            }
        }

        for (const auto& b : n.edges()) {
            const auto& t = b.other(n);
            if (is_backedge_stating_from(b, t, n)) {
                bl::del(blist, b.id());

                if (classes_.get(b) == 0) {
                    classes_.set(b, new_class());
                }
            }
        }

        for (const auto& b : n.edges()) {
            const auto& t = b.other(n);
            if (is_backedge_stating_from(b, n, t)) {
                bl::push(blist, b.id());
            }
        }

        if (hi2 < hi0) {
            create_capping_backedge(n, blist, hi2);
        }

        if (!n.is_root()) {
            determine_class(n, blist);
        }
    }

    ge.pop();

    // Construct the program structure tree
    auto visited       = NodeAttribute<bool>{g_, false};
    auto visited_stack = std::vector<NodeClass>{};
    determine_region_boundaries(g_.root(), visited, visited_stack);

    visited           = NodeAttribute<bool>{g_, false};
    auto& root_region = regions.make_node();
    regions.root      = &root_region;
    construct_program_structure_tree(g.root(), &root_region, visited);
}

void SESE::preprocess_graph() {
    auto& ge = g_.editor();

    auto exit = ge.make_node();

    for (const auto& node : g_.nodes()) {
        if (node == exit) {
            continue;
        }

        if (node.child_edges().empty()) {
            ge.make_edge(node, exit);
        }
    }

    ge.make_edge(exit, g_.root());
}

auto SESE::is_backedge_stating_from(const Edge& edge,
                                    const Node& from,
                                    const Node& to) -> bool {
    return udfs_->is_backedge(edge) && udfs_->succeed(from, to);
}

auto SESE::get_hi0(const Node& node) -> size_t {
    size_t hi0 = -1;

    for (const auto& b : node.edges()) {
        const auto& t = b.other(node);

        if (is_backedge_stating_from(b, node, t)) {
            hi0 = std::min(hi0, udfs_->dfs_num(t));
        }
    }

    return hi0;
}

auto SESE::get_hi1(const Node& node) -> size_t {
    size_t hi1 = -1;

    for (const auto& b : node.edges()) {
        const auto& child = b.other(node);

        if ((!udfs_->parents(child).empty()) &&
            (udfs_->parent(child) == node)) {
            hi1 = std::min(hi1, his_.get(child));
        }
    }

    return hi1;
}

auto SESE::get_hi2(const Node& node, size_t hi1) -> size_t {
    size_t hi2 = -1;

    // Any child c of n having c.hi = hi1 should be interpreted as A child c of
    // n having c.hi = hi1
    bool skipped_one = false;

    for (const auto& b : node.edges()) {
        const auto& child = b.other(node);

        if ((!udfs_->parents(child).empty()) &&
            (udfs_->parent(child) == node)) {
            const auto hi = his_.get(child);
            if (!skipped_one && hi == hi1) {
                skipped_one = true;
            } else {
                hi2 = std::min(hi2, hi);
            }
        }
    }

    return hi2;
}

void SESE::create_capping_backedge(const Node& node,
                                   BracketList& blist,
                                   size_t hi2) {
    auto& ge     = g_.editor();
    const auto d = ge.make_edge(node, udfs_->nodes()[hi2]);
    udfs_->set_backedge(d);
    capping_backedges_.push_back(d.id());
    bl::push(blist, d.id());
}

void SESE::determine_class(const Node& node, BracketList& blist) {
    auto e = get_parent_tree_edge(node);
    auto b = g_.get_edge(bl::top(blist));

    auto& recent_size  = recent_sizes_.get(b);
    auto& recent_class = recent_classes_.get(b);
    if (recent_size != bl::size(blist)) {
        recent_size  = bl::size(blist);
        recent_class = new_class();
    }

    auto& e_class = classes_.get(e);
    e_class       = recent_class;

    if (recent_size == 1) {
        classes_.set(b, e_class);
    }
}

auto SESE::get_parent_tree_edge(const Node& node) -> Edge {
    auto e_id = EdgeId::InvalidID;

    // TODO: move this to udfs
    for (const auto& edge : node.edges()) {
        const auto& t = edge.other(node);
        if ((udfs_->is_tree(edge)) && (udfs_->parent(node) == t)) {
            e_id = edge.id();
            break;
        }
    }
    assert(e_id != EdgeId::InvalidID);

    return g_.get_edge(e_id);
}

// NOLINTNEXTLINE(misc-no-recursion)
void SESE::determine_region_boundaries(
    const Node& node,
    NodeAttribute<bool>& visited,
    const std::vector<NodeClass>& visited_class_) {
    visited.set(node, true);

    for (const auto& edge : node.child_edges()) {
        auto visited_class = visited_class_;
        const auto& child  = edge.to();

        auto edge_class = classes_.get(edge);

        bool exiting_region = false;
        size_t i            = 0;
        for (const auto& node_class : visited_class | std::views::reverse) {
            i++;

            if (node_class.edge_class == edge_class) {
                exit_edge_.set(edge, true);
                entry_edge_.set(node_class.edge, true);

                // We could count how many regions need to be created

                exiting_region = true;
                break;
            }
        }

        if (exiting_region) {
            for (size_t j = 0; j < i; j++) {
                visited_class.pop_back();
            }
        }

        if (!visited.get(child)) {
            visited_class.push_back({child.id(), edge.id(), edge_class});
            determine_region_boundaries(child, visited, visited_class);
        }
    }
}

// NOLINTNEXTLINE(misc-no-recursion)
void SESE::construct_program_structure_tree(const Node& node,
                                            SESERegion* current_region,
                                            NodeAttribute<bool>& visited) {
    visited.set(node, true);

    node_regions.set(node, current_region);
    (*current_region)->nodes.push_back(node);

    auto& region = get_region(node);

    for (const auto& edge : node.child_edges()) {
        auto* curr = current_region;
        auto child = edge.to();

        if (exit_edge_.get(edge)) {
            region->exit_edge = edge;
            region->exit_node = edge.from();
            curr              = &region.parent();
        }

        if (entry_edge_.get(edge)) {
            auto& region = regions.make_node();

            curr->add_child(&region);
            region->entry_edge = edge;
            region->entry_node = edge.to();
            curr               = &region;
        }

        if (!visited.get(child)) {
            construct_program_structure_tree(child, curr, visited);
        }
    }
}