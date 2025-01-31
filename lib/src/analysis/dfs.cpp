#include "triskel/analysis/dfs.hpp"

#include <cstddef>
#include <string>
#include <vector>

#include "triskel/analysis/patriarchal.hpp"
#include "triskel/graph/igraph.hpp"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace triskel;

using DFS = DFSAnalysis;

DFS::DFSAnalysis(const IGraph& g)
    : Patriarchal(g),
      g_{g},
      dfs_nums_(g.max_node_id(), 0),
      types_(g.max_edge_id(), EdgeType::None) {
    nodes_.reserve(g.node_count());

    dfs(g.root());
    type_edges();
}

auto DFS::was_visited(const Node& node) -> bool {
    return (dfs_nums_.get(node) != 0) || node.is_root();
}

// NOLINTNEXTLINE(misc-no-recursion)
void DFS::dfs(const Node& node) {
    nodes_.push_back(node.id());
    dfs_nums_.set(node, nodes_.size() - 1);

    for (const auto& edge : node.edges()) {
        const auto child = edge.to();
        if (child == node) {
            continue;
        }

        if (!was_visited(child)) {
            dfs(child);

            add_parent(node, child);
            types_.set(edge, DFS::EdgeType::Tree);
            continue;
        }
    }
}

void DFS::type_edges() {
    for (const auto& edge : g_.edges()) {
        if (is_tree(edge)) {
            continue;
        }

        if (edge.to() == edge.from()) {
            types_.set(edge, DFS::EdgeType::Back);
            continue;
        }

        if (succeed(edge.from(), edge.to())) {
            types_.set(edge, DFS::EdgeType::Back);
            continue;
        }

        if (succeed(edge.to(), edge.from())) {
            types_.set(edge, DFS::EdgeType::Forward);
            continue;
        }

        types_.set(edge, DFS::EdgeType::Cross);
    }
}

auto DFS::nodes() -> std::vector<Node> {
    return g_.get_nodes(nodes_);
}

auto DFS::is_tree(const Edge& e) -> bool {
    return types_.get(e) == EdgeType::Tree;
}

auto DFS::is_backedge(const Edge& e) -> bool {
    return types_.get(e) == EdgeType::Back;
}

auto DFS::is_forward(const Edge& e) -> bool {
    return types_.get(e) == EdgeType::Forward;
}

auto DFS::is_cross(const Edge& e) -> bool {
    return types_.get(e) == EdgeType::Cross;
}

auto DFS::dump_type(const Edge& e) const -> std::string {
    switch (types_.get(e)) {
        case EdgeType::None:
            return "None";
        case EdgeType::Tree:
            return "Tree";
        case EdgeType::Back:
            return "Back";
        case EdgeType::Cross:
            return "Cross";
        case EdgeType::Forward:
            return "Forward";
    }
}

auto DFS::dfs_num(const Node& n) -> size_t {
    return dfs_nums_.get(n);
}
