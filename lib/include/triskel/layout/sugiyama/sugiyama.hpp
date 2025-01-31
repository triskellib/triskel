#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <random>
#include <utility>
#include <vector>

#include "triskel/graph/igraph.hpp"
#include "triskel/layout/ilayout.hpp"
#include "triskel/utils/attribute.hpp"

namespace triskel {

using Pair = std::pair<size_t, size_t>;

struct IOPair {
    NodeId node;
    EdgeId edge;

    // NOLINTNEXTLINE(google-explicit-constructor)
    operator Pair() const {
        return {static_cast<size_t>(node), static_cast<size_t>(edge)};
    }

    friend auto operator==(IOPair lhs, IOPair rhs) -> bool {
        return static_cast<Pair>(lhs) == static_cast<Pair>(rhs);
    }

    friend auto operator<=>(IOPair lhs, IOPair rhs) -> std::strong_ordering {
        return static_cast<Pair>(lhs) <=> static_cast<Pair>(rhs);
    }
};
static_assert(std::is_trivially_copyable_v<IOPair>);

struct SugiyamaAnalysis : public ILayout {
    explicit SugiyamaAnalysis(IGraph& g);

    explicit SugiyamaAnalysis(IGraph& g,
                              const NodeAttribute<float>& heights,
                              const NodeAttribute<float>& widths);

    explicit SugiyamaAnalysis(IGraph& g,
                              const NodeAttribute<float>& heights,
                              const NodeAttribute<float>& widths,
                              const EdgeAttribute<float>& start_x_offset,
                              const EdgeAttribute<float>& end_x_offset,
                              const std::vector<IOPair>& entries = {},
                              const std::vector<IOPair>& exits   = {});

    ~SugiyamaAnalysis() override = default;

    [[nodiscard]] auto get_x(NodeId node) const -> float override;
    [[nodiscard]] auto get_y(NodeId node) const -> float override;
    [[nodiscard]] auto get_waypoints(EdgeId edge) const
        -> const std::vector<Point>& override;

    [[nodiscard]] auto get_graph_width() const -> float;
    [[nodiscard]] auto get_graph_height() const -> float;

    [[nodiscard]] auto get_width(NodeId node) const -> float override;
    [[nodiscard]] auto get_height(NodeId node) const -> float override;

    [[nodiscard]] auto get_io_waypoints() const
        -> const std::map<Pair, std::vector<Point>>&;

   private:
    NodeAttribute<size_t> layers_;
    NodeAttribute<size_t> orders_;
    NodeAttribute<float> widths_;
    NodeAttribute<float> heights_;
    NodeAttribute<float> xs_;
    NodeAttribute<float> ys_;

    // Priorities in the coordinate assignment
    NodeAttribute<uint8_t> priorities_;

    EdgeAttribute<std::vector<Point>> waypoints_;
    EdgeAttribute<float> offsets_to_;
    EdgeAttribute<float> offsets_from_;
    EdgeAttribute<float> edge_weights_;

    auto layer_view(size_t layer);

    // Ensures the order on each layer has nodes 1 unit from each other
    void normalize_order();

    void cycle_removal();

    void layer_assignment();

    /// @brief After layer assignment attempts to move nodes that still have a
    /// degree of liberty to minimize the graph height
    void slide_nodes();

    /// @brief Edit edges so that every edge is pointing down
    /// i.e.: layer(edge.to) > layer(edge.from)
    void flip_edges();

    void remove_long_edges();

    void vertex_ordering();

    /// @brief Computes the x coordinate of each node
    void x_coordinate_assignment();

    /// @brief Compute the y coordinate of each node
    void y_coordinate_assignment();

    void coordinate_assignment_iteration(size_t layer,
                                         size_t next_layer,
                                         float graph_width);

    auto get_priority(const Node& node, size_t layer) -> size_t;

    auto min_x(std::vector<Node>& nodes, size_t id) -> float;

    auto max_x(std::vector<Node>& nodes, size_t id, float graph_width) -> float;

    auto average_position(const Node& node,
                          size_t layer,
                          bool is_going_down) -> float;

    void set_layer(const Node& node, size_t layer);

    /// @brief Creates waypoints to draw the edges connecting nodes
    void waypoint_creation();

    /// @brief Translate edge waypoints after coordinate assignment
    void translate_waypoints();

    /// @brief Calculates Y coordinates for waypoints
    void calculate_waypoints_y();

    auto get_waypoint_y(size_t id,
                        const std::vector<Edge>& edges,
                        std::vector<int64_t>& layers) -> int64_t;

    /// @brief Creates an edge waypoint and sets its layer
    auto create_ghost_node(size_t layer) -> Node;

    /// @brief Creates an edge waypoint
    auto create_waypoint() -> Node;

    void build_waypoints(EdgeId id);
    void build_long_edges_waypoints();

    float width_;
    [[nodiscard]] auto compute_graph_width() -> float;

    float height_;
    [[nodiscard]] auto compute_graph_height() -> float;

    // ----- Entry and exits -----
    /// @brief Ensures the entry/exit nodes are connected to the top/bottom
    /// layers
    void ensure_io_at_extremities();

    std::vector<IOPair> entries;
    std::vector<IOPair> exits;

    EdgeAttribute<float> start_x_offset_;
    EdgeAttribute<float> end_x_offset_;

    std::map<Pair, EdgeId> io_edges_;

    [[nodiscard]] auto is_io_edge(EdgeId edge) const -> bool;

    std::map<Pair, std::vector<Point>> io_waypoints_;

    // Saves the data of the I/O waypoints
    void make_io_waypoint(IOPair pair);

    void make_io_waypoints();
    // -----

    bool has_top_loop_    = false;
    bool has_bottom_loop_ = false;

    // std::vector<Point> exit_waypoints_;
    // EdgeId exit_edge_ = EdgeId::InvalidID;

    EdgeAttribute<std::vector<EdgeId>> edge_waypoints_;
    std::vector<EdgeId> deleted_edges_;

    EdgeAttribute<bool> is_flipped_;

    std::vector<NodeId> dummy_nodes_;

    /// @brief The nodes on a given layer
    std::vector<std::vector<Node>> node_layers_;
    void init_node_layers();

    std::default_random_engine rng_;

    size_t layer_count_;

    IGraph& g;

    friend struct Layout;
};
}  // namespace triskel