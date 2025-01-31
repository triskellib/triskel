#include "triskel/layout/ilayout.hpp"

#include <algorithm>

#include "triskel/graph/igraph.hpp"
#include "triskel/utils/point.hpp"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace triskel;

auto ILayout::get_xy(NodeId node) const -> Point {
    return {.x = get_x(node), .y = get_y(node)};
}

auto ILayout::get_graph_width(const IGraph& graph) const -> float {
    auto max_x = 0.0F;

    for (const auto& node : graph.nodes()) {
        max_x = std::max(max_x, get_x(node) + get_width(node));
    }

    for (const auto& edge : graph.edges()) {
        for (const auto& waypoint : get_waypoints(edge)) {
            max_x = std::max(max_x, waypoint.x);
        }
    }

    return max_x;
}

auto ILayout::get_graph_height(const IGraph& graph) const -> float {
    auto max_y = 0.0F;

    for (const auto& node : graph.nodes()) {
        max_y = std::max(max_y, get_y(node) + get_height(node));
    }

    for (const auto& edge : graph.edges()) {
        for (const auto& waypoint : get_waypoints(edge)) {
            max_y = std::max(max_y, waypoint.y);
        }
    }

    return max_y;
}