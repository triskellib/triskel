#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "triskel/graph/igraph.hpp"
#include "triskel/layout/sugiyama/layer_assignement.hpp"
#include "triskel/utils/attribute.hpp"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace triskel;

namespace {

auto select_node(const IGraph& g,
                 std::vector<NodeId>& U,
                 std::vector<NodeId>& Z) -> std::optional<Node> {
    for (const auto& node : g.nodes()) {
        if (std::ranges::contains(U, node.id())) {
            continue;
        }

        auto valid = true;
        for (const auto& child : node.child_nodes()) {
            if (!std::ranges::contains(Z, child.id())) {
                valid = false;
                break;
            }
        }

        if (valid) {
            return node;
        }
    }

    return {};
}

auto layer_assignment(const IGraph& g,
                      NodeAttribute<size_t>& layers) -> size_t {
    auto U = std::vector<NodeId>{};
    auto Z = std::vector<NodeId>{};

    size_t current_layer = 1;

    while (U.size() < g.node_count()) {
        auto v = select_node(g, U, Z);

        if (v.has_value()) {
            layers.set(*v, current_layer);
            U.push_back(v->id());
            continue;
        }

        current_layer++;
        Z.insert(Z.end(), U.begin(), U.end());
    }

    return current_layer + 1;
}
}  // namespace

auto triskel::longest_path_tamassia(const IGraph& graph)
    -> std::unique_ptr<LayerAssignment> {
    auto layers         = std::make_unique<LayerAssignment>(graph);
    layers->layer_count = layer_assignment(graph, layers->layers);

    return layers;
}
