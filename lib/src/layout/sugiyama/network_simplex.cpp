// Based on https://www.graphviz.org/documentation/TSE93.pdf

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

#include <fmt/base.h>
#include "triskel/graph/igraph.hpp"
#include "triskel/layout/sugiyama/layer_assignement.hpp"
#include "triskel/layout/sugiyama/network_simplex.hpp"
#include "triskel/utils/attribute.hpp"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace triskel;

SpanningTree::SpanningTree(const IGraph& g)
    : g{g},
      ranks_{g, static_cast<size_t>(-1)},
      in_tree_{g, false},
      e_in_tree_{g, false},
      postorder_tree_{g, {.low = 0, .lim = 0}},
      cut_{g, 0} {}

auto SpanningTree::slack(EdgeId e) const -> size_t {
    auto edge = g.get_edge(e);

    return ranks_.get(edge.to()) - ranks_.get(edge.from()) - 1;
}

auto SpanningTree::is_tight(EdgeId e) const -> bool {
    return slack(e) == 0;
}

auto SpanningTree::leave_edge() -> EdgeId {
    // For cyclical search
    static size_t i = 0;
    const size_t sz = edges_.size();

    for (size_t j = 0; j < sz; j++) {
        auto& e = edges_[(j + i) % sz];
        if (cut_[e] < 0) {
            i = (j + i + 1) % sz;
            return e;
        }
    }
    return EdgeId::InvalidID;
}

void SpanningTree::init_rank() {
    const auto nodes = g.nodes();

    ranks_[g.root()]   = 0;
    size_t found_nodes = 1;
    size_t rank        = 1;

    while (found_nodes < nodes.size()) {
        for (const auto& node : nodes) {
            if (ranks_.get(node) < rank) {
                continue;
            }

            for (const auto& parent : node.parent_nodes()) {
                if (ranks_.get(parent) >= rank) {
                    goto continue_loop;
                }
            }

            ranks_.set(node, rank);
            found_nodes += 1;

        continue_loop:
        }

        rank += 1;
    }
}

// NOLINTNEXTLINE(misc-no-recursion)
void SpanningTree::tight_tree_recurs(NodeId n) {
    const auto& node = g.get_node(n);

    for (const auto& edge : node.edges()) {
        auto neighbor = edge.other(node);

        if (in_tree_[neighbor]) {
            continue;
        }

        // Only keep tight edges
        if (!is_tight(edge)) {
            continue;
        }

        edges_.push_back(edge);
        e_in_tree_[edge] = true;

        nodes_.push_back(neighbor);
        in_tree_[neighbor] = true;

        tight_tree_recurs(neighbor);
    }
}

auto SpanningTree::tight_tree() -> size_t {
    for (auto n : nodes_) {
        tight_tree_recurs(n);
    }

    return nodes_.size();
}

// NOLINTNEXTLINE(misc-no-recursion)
void SpanningTree::build_postorder_tree_rec(const Node& node,
                                            NodeId parent,
                                            size_t& lim) {
    const auto low = lim;

    for (const auto& edge : node.edges()) {
        const auto& child = edge.other(node);
        if (e_in_tree_[edge] && child != parent) {
            build_postorder_tree_rec(child, node, lim);
        }
    }

    postorder_tree_.set(node, {.low = low, .lim = lim});
    lim += 1;
}

void SpanningTree::build_postorder_tree() {
    size_t lim = 0;
    build_postorder_tree_rec(g.root(), NodeId::InvalidID, lim);
}

auto SpanningTree::lim(NodeId n) const -> size_t {
    return postorder_tree_.get(n).lim;
}

auto SpanningTree::low(NodeId n) const -> size_t {
    return postorder_tree_.get(n).low;
}

auto SpanningTree::get_component(const Edge& e, NodeId w) const -> int32_t {
    int32_t c = 1;

    auto u       = e.from();
    const auto v = e.to();

    if (lim(v) <= lim(u)) {
        u = v;
    }

    return low(u) <= lim(w) && lim(w) <= lim(u) ? -c : c;
}

/// @brief Initializes the cut values
// NOLINTNEXTLINE(misc-no-recursion)
void SpanningTree::init_cut_values(const Node& node, EdgeId tree_edge) {
    int64_t cut_value = 0;

    if (tree_edge == EdgeId::InvalidID) {
        for (const auto& edge : node.child_edges()) {
            if (e_in_tree_[edge]) {
                init_cut_values(edge.to(), edge);
            }
        }

        return;
    }

    const auto& tedge = g.get_edge(tree_edge);

    for (const auto& edge : node.child_edges()) {
        cut_value -= 1;

        if (e_in_tree_[edge] && edge != tree_edge) {
            init_cut_values(edge.other(node), edge);

            cut_value += cut_[edge];
        }
    }

    for (const auto& edge : node.parent_edges()) {
        cut_value += 1;

        if (e_in_tree_[edge] && edge != tree_edge) {
            init_cut_values(edge.other(node), edge);

            cut_value -= cut_[edge];
        }
    }

    cut_[tree_edge] =
        g.get_edge(tree_edge).to() == node ? cut_value : -cut_value;
}

void SpanningTree::init_cut_values() {
    build_postorder_tree();
    init_cut_values(g.root(), EdgeId::InvalidID);
}

auto SpanningTree::enter_edge(const Edge& e) -> Edge {
    auto min_slack = std::numeric_limits<size_t>::max();
    auto f         = EdgeId::InvalidID;

    const auto from_component = get_component(e, e.from());
    const auto to_component   = get_component(e, e.to());

    for (const auto& edge : g.edges()) {
        if (get_component(e, edge.to()) == from_component &&
            get_component(e, edge.from()) == to_component && edge != e) {
            const auto s = slack(edge);
            if (s < min_slack) {
                min_slack = s;
                f         = edge;
            }
        }
    }

    assert(f != EdgeId::InvalidID);
    return g.get_edge(f);
}

// NOLINTNEXTLINE(misc-no-recursion)
void SpanningTree::update_ranks_rec(const Node& node,
                                    NodeId parent,
                                    size_t delta) {
    assert(node != g.root());
    ranks_[node] += delta;

    for (const auto& edge : node.child_edges()) {
        const auto& neighbor = edge.to();
        if (e_in_tree_[edge] && neighbor != parent) {
            update_ranks_rec(neighbor, node, delta);
        }
    }

    for (const auto& edge : node.parent_edges()) {
        const auto& neighbor = edge.from();
        if (e_in_tree_[edge] && neighbor != parent) {
            update_ranks_rec(neighbor, node, delta);
        }
    }
}

void SpanningTree::exchange(const Edge& e, const Edge& f) {
    e_in_tree_[e] = false;

    // update rank of the tail section
    auto head = e.from();
    auto tail = e.to();

    if (lim(head) <= lim(tail)) {
        std::swap(tail, head);
    }

    size_t delta = slack(f);
    update_ranks_rec(tail, NodeId::InvalidID, delta);

    e_in_tree_[f] = true;
    std::ranges::replace(edges_, e, f);

    build_postorder_tree();
    init_cut_values(g.root(), EdgeId::InvalidID);
}

auto SpanningTree::get_incident_edge() -> Edge {
    auto e           = EdgeId::InvalidID;
    size_t min_slack = std::numeric_limits<size_t>::max();

    for (const auto& node : g.nodes()) {
        if (in_tree_.get(node)) {
            continue;
        }

        for (const auto& edge : node.edges()) {
            auto neighbor = edge.other(node);

            // We want incident edges
            if (!in_tree_.get(neighbor)) {
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
    assert(e != EdgeId::InvalidID);
    return g.get_edge(e);
}

void SpanningTree::feasible_tree() {
    init_rank();

    nodes_.push_back(g.root());
    in_tree_[g.root()] = true;

    // Adds the root to the tree
    while (tight_tree() < g.node_count()) {
        auto edge  = get_incident_edge();
        auto delta = slack(edge);

        if (in_tree_[edge.to()]) {
            delta = -delta;
        }

        for (const auto& node : nodes_) {
            ranks_[node] += delta;
        }
    }

    init_cut_values();
}

auto SpanningTree::normalize_ranks() -> size_t {
    size_t max_rank = 0;

    for (const auto& node : g.nodes()) {
        max_rank = std::max(max_rank, ranks_.get(node));
    }

    size_t rank_count = 0;
    for (const auto& node : g.nodes()) {
        auto rank = max_rank - ranks_.get(node) + 1;
        ranks_.set(node, rank);
        rank_count = std::max(rank_count, rank);
    }

    return rank_count + 1;
}

void SpanningTree::dump() {
    size_t max_rank = 0;

    for (const auto& node : g.nodes()) {
        max_rank = std::max(max_rank, ranks_.get(node));
    }

    fmt::print("digraph G {{\n");
    for (const auto& node : g.nodes()) {
        fmt::print("{} [label=\"{} {} {}\"]\n", node, node, lim(node),
                   low(node));
    }

    for (size_t rank = 0; rank < max_rank; rank++) {
        fmt::print("{{ rank=same;");

        for (const auto& node : g.nodes()) {
            if (ranks_[node] == rank) {
                fmt::print("{};", node);
            }
        }
        fmt::print("}}\n");
    }

    for (const auto& edge : g.edges()) {
        fmt::print("{} -> {} {}\n", edge.from(), edge.to(),
                   e_in_tree_[edge] ? fmt::format("[label=\"{}\"]", cut_[edge])
                                    : "[style=dotted]");
    }
    fmt::print("}}\n");
}

auto triskel::network_simplex(const IGraph& graph)
    -> std::unique_ptr<LayerAssignment> {
    auto spanning_tree = SpanningTree(graph);
    spanning_tree.feasible_tree();

    EdgeId id;
    while ((id = spanning_tree.leave_edge()) != EdgeId::InvalidID) {
        auto e = graph.get_edge(id);
        auto f = spanning_tree.enter_edge(e);
        spanning_tree.exchange(e, f);
    }

    auto rank_count = spanning_tree.normalize_ranks();

    // Balance

    return std::make_unique<LayerAssignment>(spanning_tree.ranks_, rank_count);
}
