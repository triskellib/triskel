#include "triskel/analysis/patriarchal.hpp"

#include <cassert>
#include <functional>
#include <queue>
#include <vector>

#include "triskel/graph/igraph.hpp"
#include "triskel/utils/attribute.hpp"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace triskel;

namespace {

using Next = std::function<std::vector<Node>(const Node&)>;

auto bfs(const IGraph& g,
         const Next& next,
         const Node& n1,
         const Node& n2) -> bool {
    auto visited = NodeAttribute<bool>{g.max_node_id(), false};
    auto stack   = std::queue<NodeId>{{n1.id()}};

    while (!stack.empty()) {
        const auto& n = g.get_node(stack.front());
        stack.pop();

        if (visited.get(n)) {
            continue;
        }

        visited.set(n, true);

        for (const auto& child : next(n)) {
            if (visited.get(child)) {
                continue;
            }

            if (child == n2) {
                return true;
            }

            stack.push(child.id());
        }
    }

    return false;
}
}  // namespace

Patriarchal::Patriarchal(const IGraph& g)
    : g_{g}, parents_{g.max_node_id(), {}}, children_{g.max_node_id(), {}} {}

void Patriarchal::add_parent(const Node& parent, const Node& child) {
    auto& parents  = parents_.get(child);
    auto& children = children_.get(parent);

    parents.push_back(parent.id());
    children.push_back(child.id());
}

auto Patriarchal::parents(const Node& n) -> std::vector<Node> {
    return g_.get_nodes(parents_.get(n));
}

auto Patriarchal::parent(const Node& n) -> Node {
    auto& parents = parents_.get(n);
    assert(parents.size() == 1);
    return g_.get_node(parents.front());
}

auto Patriarchal::children(const Node& n) -> std::vector<Node> {
    return g_.get_nodes(children_.get(n));
}

auto Patriarchal::child(const Node& n) -> Node {
    auto& children = children_.get(n);
    assert(children.size() == 1);
    return g_.get_node(children.front());
}

auto Patriarchal::precedes(const Node& n1, const Node& n2) -> bool {
    return bfs(g_, [&](const Node& n) { return children(n); }, n1, n2);
}

auto Patriarchal::succeed(const Node& n1, const Node& n2) -> bool {
    return bfs(g_, [&](const Node& n) { return parents(n); }, n1, n2);
}