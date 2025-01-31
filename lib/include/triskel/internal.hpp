#pragma once

#include <memory>
#include <string>

#include "triskel/graph/graph.hpp"
#include "triskel/triskel.hpp"
#include "triskel/utils/attribute.hpp"

namespace triskel {
[[nodiscard]] auto make_layout(std::unique_ptr<Graph> g,
                               const NodeAttribute<float>& width,
                               const NodeAttribute<float>& height,
                               const NodeAttribute<std::string>& label,
                               const EdgeAttribute<LayoutBuilder::EdgeType>&
                                   edge_types) -> std::unique_ptr<CFGLayout>;
}