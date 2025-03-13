#pragma once

#include "triskel/graph/igraph.hpp"
#include "triskel/utils/attribute.hpp"

namespace triskel {

/// @brief Calculates the immediate dominators of nodes in a graph
[[nodiscard]] auto make_idoms(const IGraph& g) -> NodeAttribute<NodeId>;

}  // namespace triskel
