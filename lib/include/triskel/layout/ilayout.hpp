#pragma once

#include "triskel/graph/igraph.hpp"
#include "triskel/utils/point.hpp"

namespace triskel {

struct ILayout {
    virtual ~ILayout() = default;

    [[nodiscard]] virtual auto get_x(NodeId node) const -> float = 0;
    [[nodiscard]] virtual auto get_y(NodeId node) const -> float = 0;

    [[nodiscard]] virtual auto get_xy(NodeId node) const -> Point;

    [[nodiscard]] virtual auto get_waypoints(EdgeId edge) const
        -> const std::vector<Point>& = 0;

    [[nodiscard]] virtual auto get_width(NodeId node) const -> float  = 0;
    [[nodiscard]] virtual auto get_height(NodeId node) const -> float = 0;

    [[nodiscard]] virtual auto get_graph_width(const IGraph& graph) const
        -> float;
    [[nodiscard]] virtual auto get_graph_height(const IGraph& graph) const
        -> float;
};
}  // namespace triskel
