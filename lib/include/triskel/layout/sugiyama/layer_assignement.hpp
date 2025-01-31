#pragma once

#include <cstddef>
#include <memory>

#include "triskel/graph/igraph.hpp"
#include "triskel/utils/attribute.hpp"

namespace triskel {

/// @brief The layers of nodes during layered graph drawing
struct LayerAssignment {
    explicit LayerAssignment(const IGraph& graph) : layers{graph, 0} {}
    explicit LayerAssignment(const NodeAttribute<size_t>& layers,
                             size_t layer_count)
        : layers{layers}, layer_count{layer_count} {}

    NodeAttribute<size_t> layers;
    size_t layer_count;
};

/// @brief Calculates layer assignment using longest path from tamassia
auto longest_path_tamassia(const IGraph& graph)
    -> std::unique_ptr<LayerAssignment>;

/// @brief Calculates layer assignment using network simplex
auto network_simplex(const IGraph& graph) -> std::unique_ptr<LayerAssignment>;

}  // namespace triskel