#include "triskel/layout/phantom_nodes.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <map>
#include <memory>
#include <ranges>
#include <set>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include "triskel/analysis/dfs.hpp"
#include "triskel/graph/igraph.hpp"
#include "triskel/utils/attribute.hpp"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace triskel;

namespace {

using DomList = NodeAttribute<std::set<NodeId>>;
using IDom    = NodeAttribute<NodeId>;

auto domination_analysis(const IGraph& g) -> DomList {
    const auto& nodes = g.nodes();

    auto all_nodes = std::set<NodeId>{};

    for (const auto& node : nodes) {
        all_nodes.insert(node);
    }

    auto dom = DomList{nodes.size(), all_nodes};

    dom.set(g.root(), std::set<NodeId>{g.root()});

    bool changes = true;

    while (changes) {
        changes = false;
        for (const auto& node : nodes) {
            if (node.is_root()) {
                continue;
            }

            const auto& parents = node.parent_nodes();

            std::set<NodeId> parent_intersection;

            if (parents.size() == 1) {
                parent_intersection = dom.get(parents.front());
            } else if (parents.size() == 2) {
                std::ranges::set_intersection(
                    dom.get(parents.front()), dom.get(parents.back()),
                    std::inserter(parent_intersection,
                                  parent_intersection.begin()));
            } else {
                parent_intersection = dom.get(parents.front());

                for (const auto& parent : parents | std::views::drop(1)) {
                    std::set<NodeId> tmp;
                    std::ranges::set_intersection(
                        parent_intersection, dom.get(parent),
                        std::inserter(tmp, tmp.begin()));
                    parent_intersection = std::move(tmp);
                }
            }

            parent_intersection.insert(node);

            if (dom.get(node).size() != parent_intersection.size()) {
                dom.set(node, parent_intersection);
                changes = true;
            }
        }
    }

    return std::move(dom);
}

auto idom_analysis(const IGraph& g, DomList& dom) -> IDom {
    const auto& nodes = g.nodes();
    auto idom         = IDom(nodes.size(), NodeId::InvalidID);

    for (const auto& node : nodes) {
        if (node.is_root()) {
            idom.set(node, NodeId::InvalidID);
            continue;
        }

        auto& doms = dom.get(node);
        auto sz    = doms.size();

        for (const auto& d : doms) {
            if (dom.get(d).size() == sz - 1) {
                idom.set(node, d);
                break;
            }
        }
    }

    return std::move(idom);
}

using DFSNums = NodeAttribute<std::vector<size_t>>;

auto starts_with(const std::span<size_t>& vec,
                 const std::span<size_t>& prefix) -> bool {
    return vec.size() >= prefix.size() &&
           std::equal(prefix.begin(), prefix.end(), vec.begin());
}

// A radix node ish
struct RNode {
    // The corresponding CFG node
    std::vector<NodeId> node_ids;

    std::vector<size_t> radix;
    std::unordered_map<size_t, std::unique_ptr<RNode>> children;

    RNode() {
        static size_t id_ = 0;
        id                = id_++;
    }

    // NOLINTNEXTLINE
    void dump() {
        fmt::print("n{} [label=\"{}\"]\n", id, radix);

        for (const auto& child : children) {
            fmt::print("n{} -> n{}\n", id, child.second->id);
            child.second->dump();
        }

        for (size_t i = 0; i < node_ids.size(); ++i) {
            auto node_id = node_ids[i];
            fmt::print("n{}_{} [label=\"{}\", shape=square]\n", id, i, node_id);
            fmt::print("n{} -> n{}_{}\n", id, id, i);
        }
    }

    void new_child(const Node& n, const std::span<size_t>& n_radix) {
        assert(n_radix.size() > 0);

        auto node = std::make_unique<RNode>();
        node->radix.assign(n_radix.begin(), n_radix.end());
        node->node_ids = {n};

        children[n_radix.front()] = std::move(node);
    }

    void split(const Node& n, const std::span<size_t>& n_radix) {
        auto i = 0;

        auto size = std::min(radix.size(), n_radix.size());
        while (i < size) {
            if (radix[i] != n_radix[i]) {
                break;
            }

            i++;
        }

        assert(i > 0);

        auto start = std::span<size_t>(radix).first(i);
        auto end   = std::span<size_t>(radix).subspan(i);

        auto new_node = std::make_unique<RNode>();

        new_node->children = std::move(children);
        children.clear();

        new_node->node_ids = std::move(node_ids);
        node_ids.clear();

        radix.assign(start.begin(), start.end());
        new_node->radix.assign(end.begin(), end.end());

        children[radix[i]] = std::move(new_node);

        if (i == n_radix.size()) {
            node_ids.push_back(n);
        } else {
            new_child(n, n_radix.subspan(i));
        }
    }

    size_t id;
};

// A radix tree ish
struct RTree {
    RTree() = default;

    // Insert a list into the radix tree
    void insert(const Node& n, const std::span<size_t>& dfsnums) {
        auto* cursor = &root;

        size_t i = 0;

        while (i < dfsnums.size()) {
            auto key = dfsnums[i];

            if (!cursor->children.contains(key)) {
                // create a new node
                cursor->new_child(n, dfsnums.subspan(i));
                return;
            }

            cursor = cursor->children[key].get();

            auto& radix = cursor->radix;
            if (starts_with(dfsnums.subspan(i), radix)) {
                i += radix.size();
            } else {
                // Split the node
                cursor->split(n, dfsnums.subspan(i));
                return;
            }
        }

        // An element with this key is already in the tree
        cursor->node_ids.push_back(n);
    }

    [[nodiscard]] auto bfs() -> std::vector<RNode*> {
        auto rnodes = std::vector<RNode*>{};

        auto current_layer = std::make_unique<std::vector<RNode*>>();

        auto next_layer = std::make_unique<std::vector<RNode*>>();
        next_layer->push_back(&root);

        while (!next_layer->empty()) {
            std::swap(current_layer, next_layer);
            next_layer->clear();

            for (auto* rnode : *current_layer) {
                rnodes.push_back(rnode);
                for (auto& child : rnode->children) {
                    next_layer->push_back(child.second.get());
                }
            }
        }

        return rnodes;
    }

    void dump() {
        fmt::print("digraph G {{\n");
        root.dump();
        fmt::print("}}\n");
    }

    RNode root;
};

// Split the entry edges using a radix tree
void split_node(IGraph& graph, Node& node, DFSNums& dfs_nums) {
    auto& editor = graph.editor();
    auto rtree   = RTree{};

    for (const auto& parent : node.parent_nodes()) {
        rtree.insert(parent, dfs_nums.get(parent));
    }

    std::map<size_t, NodeId> rnode_to_graph;

    // reverse bfs
    auto rnodes = rtree.bfs();
    std::ranges::reverse(rnodes);

    for (auto* rnode : rnodes) {
        if (rnode->children.empty() && rnode->node_ids.size() == 1) {
            rnode_to_graph[rnode->id] = rnode->node_ids.front();
            continue;
        }

        // We need to create a new node for this element
        auto node                 = editor.make_node();
        rnode_to_graph[rnode->id] = node.id();

        // Creates an edge from each of the children to the parent
        for (auto& child : rnode->children) {
            auto child_id = rnode_to_graph.at(child.second->id);
            editor.make_edge(child_id, node);
        }

        // Create an edge from each leaf to the parent
        for (auto& leaf_id : rnode->node_ids) {
            editor.make_edge(leaf_id, node);
        }
    }

    // Deletes all the edges to the node
    for (const auto& edge : node.parent_edges()) {
        editor.remove_edge(edge);
    }

    // Adds an edge from the root to our node
    editor.make_edge(rnode_to_graph.at(rtree.root.id), node);
}

// NOLINTNEXTLINE(misc-no-recursion)
auto make_dfsnums(NodeId root,
                  NodeAttribute<std::vector<size_t>>& dfsnums,
                  IDom& idoms,
                  NodeId node) -> std::vector<size_t> {
    auto& nums = dfsnums.get(node);

    if (!nums.empty()) {
        return nums;
    }

    if (node == root) {
        return nums;
    }

    auto idom     = idoms.get(node);
    auto previous = make_dfsnums(root, dfsnums, idoms, idom);
    nums          = std::vector<size_t>(previous);
    nums.push_back(static_cast<size_t>(idom));

    return nums;
}

}  // namespace

void triskel::create_phantom_nodes(IGraph& g) {
    auto dom = domination_analysis(g);

    auto idoms = idom_analysis(g, dom);

    const auto& nodes = g.nodes();
    auto dfsnums      = NodeAttribute<std::vector<size_t>>{nodes.size(), {}};

    for (const auto& node : nodes) {
        make_dfsnums(g.root(), dfsnums, idoms, node);
    }

    auto& editor = g.editor();

    for (auto& node : g.nodes()) {
        if (node.parent_nodes().size() >= 3) {
            split_node(g, node, dfsnums);
            continue;
        }
        if (node.child_edges().size() > 1 && node.parent_edges().size() > 1) {
            // Create a single parent for this node
            auto parent = editor.make_node();

            for (const auto& edge : node.parent_edges()) {
                editor.edit_edge(edge, edge.from(), parent);
            }

            editor.make_edge(parent, node);
        }
    }

    fmt::print("Added fantom nodes\n");
}