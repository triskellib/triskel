#pragma once

#include <cstdint>

namespace triskel {

// =============================================================================
// Layout
// =============================================================================
constexpr float WAYPOINT_WIDTH  = 0.0F;
constexpr float WAYPOINT_HEIGHT = 0.0F;

// Node priorities
constexpr uint8_t DEFAULT_PRIORITY        = 0;
constexpr uint8_t WAYPOINT_PRIORITY       = 1;
constexpr uint8_t ENTRY_WAYPOINT_PRIORITY = 2;
constexpr uint8_t EXIT_WAYPOINT_PRIORITY  = 2;

// The space between nodes
constexpr float X_GUTTER = 50.0F;

// The space between nodes and the first / last edge
constexpr float Y_GUTTER = 40.0F;

// The height of an edge
constexpr float EDGE_HEIGHT = 30.0F;

// =============================================================================
// Display
// =============================================================================
constexpr float DEFAULT_X_GUTTER = 0.0F;
constexpr float DEFAULT_Y_GUTTER = 0.0F;
constexpr float PADDING          = 150.0F;

}  // namespace triskel