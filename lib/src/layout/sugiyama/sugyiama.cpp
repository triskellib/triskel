#include "triskel/layout/sugiyama/layer_assignement.hpp"
#include "triskel/layout/sugiyama/sugiyama.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <map>
#include <ranges>
#include <stack>
#include <vector>

#include <fmt/core.h>
#include <fmt/printf.h>

#include "triskel/analysis/dfs.hpp"
#include "triskel/graph/igraph.hpp"
#include "triskel/layout/sugiyama/vertex_ordering.hpp"
#include "triskel/utils/attribute.hpp"
#include "triskel/utils/constants.hpp"
#include "triskel/utils/point.hpp"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace triskel;

auto SugiyamaAnalysis::layer_view(size_t layer) {
    return std::ranges::views::filter(
        [layer, this](const Node& node) { return layers_.get(node) == layer; });
}

void SugiyamaAnalysis::normalize_order() {
    for (size_t l = 0; l < layer_count_; ++l) {
        auto& nodes = node_layers_[l];

        // THIS IS IMPORTANT
        std::ranges::shuffle(nodes, rng_);

        std::ranges::sort(nodes, [this](const Node& a, const Node& b) {
            return orders_.get(a) < orders_.get(b);
        });

        auto order = 0;

        for (const auto& node : nodes) {
            orders_.set(node, order);
            order++;
        }
    }
}

SugiyamaAnalysis::SugiyamaAnalysis(IGraph& g)
    : SugiyamaAnalysis(g,
                       NodeAttribute<float>{g, 1.0F},
                       NodeAttribute<float>{g, 1.0F},
                       EdgeAttribute<float>{g, -1.0F},
                       EdgeAttribute<float>{g, -1.0F}) {}

void SugiyamaAnalysis::init_node_layers() {
    node_layers_.clear();
    node_layers_.resize(layer_count_);
    for (const auto& node : g.nodes()) {
        node_layers_[layers_.get(node)].push_back(node);
    }
}

SugiyamaAnalysis::SugiyamaAnalysis(IGraph& g,
                                   const NodeAttribute<float>& heights,
                                   const NodeAttribute<float>& widths)
    : SugiyamaAnalysis(g,
                       heights,
                       widths,
                       EdgeAttribute<float>(g, -1),
                       EdgeAttribute<float>(g, -1),
                       {},
                       {}) {}

SugiyamaAnalysis::SugiyamaAnalysis(IGraph& g,
                                   const NodeAttribute<float>& heights,
                                   const NodeAttribute<float>& widths,
                                   const EdgeAttribute<float>& start_x_offset,
                                   const EdgeAttribute<float>& end_x_offset,
                                   const std::vector<IOPair>& entries,
                                   const std::vector<IOPair>& exits)
    : layers_(g, 0),
      orders_(g, 0),
      waypoints_(g, {}),
      widths_(widths),
      heights_(heights),
      xs_(g, 0.0F),
      ys_(g, 0.0F),
      edge_waypoints_(g, {}),
      is_flipped_(g, false),
      offsets_to_(g, 0.0F),
      offsets_from_(g, 0.0F),
      edge_weights_(g, 1.0F),
      priorities_(g, 0),
      entries(entries),
      exits(exits),
      start_x_offset_(start_x_offset),
      end_x_offset_(end_x_offset),
      g{g}

{
    auto& ge = g.editor();
    ge.push();

    cycle_removal();

    layer_assignment();

    slide_nodes();

    ensure_io_at_extremities();

    remove_long_edges();

    init_node_layers();

    ge.push();
    flip_edges();

    y_coordinate_assignment();

    vertex_ordering();

    waypoint_creation();

    x_coordinate_assignment();

    translate_waypoints();

    calculate_waypoints_y();

    height_ = compute_graph_height();
    width_  = compute_graph_width();

    ge.pop();

    make_io_waypoints();

    build_long_edges_waypoints();

    // Sets the edge waypoints

    ge.pop();

    init_node_layers();

    // Remove the "kink" in the back edges from the ghost nodes
    for (auto eid : deleted_edges_) {
        auto edge = g.get_edge(eid);
        if (ys_.get(edge.from()) < ys_.get(edge.to())) {
            continue;
        }

        auto& waypoints = waypoints_.get(eid);

        waypoints.erase(waypoints.begin() + 3);
        waypoints.erase(waypoints.begin() + 3);
        // waypoints.erase(waypoints.begin() + 3);
        // waypoints.erase(waypoints.begin() + 3);

        waypoints.erase(waypoints.end() - 4);
        waypoints.erase(waypoints.end() - 4);
        // waypoints.erase(waypoints.end() - 4);
        // waypoints.erase(waypoints.end() - 4);
    }
}

// The idea of this function is to reverse each backedge
// However if the backedge is from a node to itself, we have to remove that
// edge
void SugiyamaAnalysis::cycle_removal() {
    auto dfs = DFSAnalysis(g);
    auto& ge = g.editor();

    for (const auto& edge : g.edges()) {
        if (dfs.is_backedge(edge)) {
            if (edge.to() == edge.from()) {
                ge.remove_edge(edge);
                continue;
            }

            ge.edit_edge(edge, edge.to(), edge.from());
            is_flipped_.set(edge, true);
        }
    }
}

void SugiyamaAnalysis::layer_assignment() {
    auto layers  = network_simplex(g);
    layers_      = layers->layers;
    layer_count_ = layers->layer_count;
}

struct SlideCandidate {
    Node node;
    size_t min_layer;
    size_t max_layer;
    float height;
};

void SugiyamaAnalysis::slide_nodes() {
    auto candidates = std::vector<SlideCandidate>{};

    for (const auto& node : g.nodes()) {
        const auto layer = layers_.get(node);

        auto neighbor_layers =
            node.neighbors()  //
            | std::ranges::views::transform(
                  [&](const Node& n) { return layers_.get(n); })  //
            | std::ranges::to<std::vector<size_t>>();

        auto smaller_layers =
            neighbor_layers |
            std::ranges::views::filter([&](size_t l) { return l <= layer; });

        auto min_layer = layer;
        if (!smaller_layers.empty()) {
            min_layer = std::ranges::max(smaller_layers);
            min_layer += 1;
        }
        assert(min_layer <= layer);

        auto bigger_layers =
            neighbor_layers |
            std::ranges::views::filter([&](size_t l) { return l >= layer; });

        auto max_layer = layer;
        if (!bigger_layers.empty()) {
            max_layer = std::ranges::min(bigger_layers);
            max_layer -= 1;
        }
        assert(layer <= max_layer);

        if (min_layer == max_layer) {
            continue;
        }

        candidates.push_back({.node      = node,
                              .min_layer = min_layer,
                              .max_layer = max_layer,
                              .height    = heights_.get(node)});
    }

    std::ranges::sort(candidates,
                      [](const SlideCandidate& c1, const SlideCandidate& c2) {
                          return c1.height > c2.height;
                      });

    for (const auto& candidate : candidates) {
        const auto& node     = candidate.node;
        const auto min_layer = candidate.min_layer;
        const auto max_layer = candidate.max_layer;
        const auto layer     = layers_.get(node);

        auto best_height = compute_graph_height();
        auto best_layer  = layer;

        for (size_t r = min_layer; r <= max_layer; ++r) {
            if (r == layer) {
                continue;
            }

            layers_.set(node, r);
            const auto height = compute_graph_height();
            if (height < best_height) {
                best_height = height;
                best_layer  = r;
            }
        }

        layers_.set(node, best_layer);
    }
}

void SugiyamaAnalysis::set_layer(const Node& node, size_t layer) {
    assert(layer >= 0);
    assert(layer < layer_count_);

    layers_.set(node, layer);

    if (layer == layer_count_) {
        has_top_loop_ = true;
    }

    if (layer == 0) {
        has_bottom_loop_ = true;
    }
}

auto SugiyamaAnalysis::create_waypoint() -> Node {
    auto& editor  = g.editor();
    auto waypoint = editor.make_node();

    heights_.set(waypoint, WAYPOINT_HEIGHT);
    widths_.set(waypoint, WAYPOINT_WIDTH);
    priorities_.set(waypoint, WAYPOINT_PRIORITY);
    dummy_nodes_.push_back(waypoint.id());

    return waypoint;
}

auto SugiyamaAnalysis::create_ghost_node(size_t layer) -> Node {
    auto waypoint = create_waypoint();
    set_layer(waypoint, layer);
    return waypoint;
}

auto SugiyamaAnalysis::is_io_edge(EdgeId edge) const -> bool {
    return std::ranges::any_of(
        io_edges_, [=](const auto& kv) { return kv.second == edge; });
}

// TODO: split edges on the same layer
void SugiyamaAnalysis::remove_long_edges() {
    std::stack<EdgeId> edges_to_split;

    for (const auto& edge : g.edges()) {
        auto from_layer = layers_.get(edge.from());
        auto to_layer   = layers_.get(edge.to());

        auto bottom_layer = std::min(from_layer, to_layer);
        auto top_layer    = std::max(from_layer, to_layer);

        if (top_layer - bottom_layer > 1 || is_flipped_.get(edge)) {
            edges_to_split.push(edge.id());
        }
    }

    auto& ge = g.editor();
    while (!edges_to_split.empty()) {
        auto edge = g.get_edge(edges_to_split.top());
        edges_to_split.pop();

        auto& waypoints = edge_waypoints_.get(edge);

        ge.remove_edge(edge);

        if (!is_io_edge(edge)) {
            // IO edges are handles differently
            deleted_edges_.push_back(edge);
        }

        // Arrows go from top to bottom

        auto from_layer = layers_.get(edge.from());
        auto to_layer   = layers_.get(edge.to());

        auto bottom_layer = std::min(from_layer, to_layer);
        auto top_layer    = std::max(from_layer, to_layer);

        auto bottom = bottom_layer == from_layer ? edge.from() : edge.to();
        auto top    = top_layer == to_layer ? edge.to() : edge.from();

        auto is_flipped = is_flipped_.get(edge);

        auto is_going_up = ((is_flipped && (from_layer != bottom_layer)) ||
                            (!is_flipped && (from_layer == bottom_layer)));

        if (is_going_up) {
            // We need to insert additional nodes for the edge to wrap around
            bottom_layer -= 2;
            top_layer += 2;
        }

        auto previous_point = bottom;
        for (size_t layer = bottom_layer + 1; layer < top_layer; layer++) {
            auto waypoint = create_ghost_node(layer);

            auto new_edge = ge.make_edge(waypoint, previous_point);
            waypoints.push_back(new_edge.id());
            if (is_going_up && (layer == bottom_layer + 1)) {
                edge_weights_.set(new_edge, 0);
            }

            previous_point = waypoint;
        }

        auto new_edge = ge.make_edge(top, previous_point);
        waypoints.push_back(new_edge.id());
        if (is_going_up) {
            edge_weights_.set(new_edge, 0);
        }

        if (!is_going_up) {
            std::ranges::reverse(waypoints);
            for (const auto id : waypoints) {
                const auto edge = g.get_edge(id);
                ge.edit_edge(edge, edge.to(), edge.from());
            }
        }
    }
}

void SugiyamaAnalysis::vertex_ordering() {
    auto ordering = VertexOrdering(g, layers_, layer_count_);
    orders_       = ordering.orders_;
    for (size_t l = 0; l < layer_count_; ++l) {
        auto& nodes = node_layers_[l];

        std::ranges::sort(nodes, [this](const Node& a, const Node& b) {
            return orders_.get(a) < orders_.get(b);
        });
    }
};

auto SugiyamaAnalysis::get_priority(const Node& node, size_t layer) -> size_t {
    if (std::ranges::contains(dummy_nodes_, node.id())) {
        return -1;
    }

    return std::ranges::distance(
        node.edges()  //
        | std::ranges::views::transform(
              [&](const Edge& e) { return e.other(node); })  //
        | std::ranges::views::filter(
              [&](const Node& n) { return layers_.get(n) == layer; }));
}

auto SugiyamaAnalysis::min_x(std::vector<Node>& nodes, size_t id) -> float {
    auto priority = priorities_.get(nodes[id]);
    auto w        = 0.0F;

    for (size_t i = id - 1; i < id; --i) {
        w += widths_.get(nodes[i]) + X_GUTTER;

        // Nodes are laid out left to right so we also care about equal
        // priority nodes
        if (priorities_.get(nodes[i]) >= priority) {
            return xs_.get(nodes[i]) + w;
        }
    }
    // The left gutter of this block
    w += X_GUTTER;

    return w;
}

auto SugiyamaAnalysis::max_x(std::vector<Node>& nodes,
                             size_t id,
                             float graph_width) -> float {
    auto priority = priorities_.get(nodes[id]);
    auto w        = widths_.get(nodes[id]) + X_GUTTER;

    for (size_t i = id + 1; i < nodes.size(); ++i) {
        // Nodes are laid out left to right so we only care about higher
        // priority nodes
        if (priorities_.get(nodes[i]) > priority) {
            return xs_.get(nodes[i]) - w;
        }

        w += widths_.get(nodes[i]) + X_GUTTER;
    }
    assert(graph_width >= w);
    return graph_width - w;
}

auto SugiyamaAnalysis::average_position(const Node& node,
                                        size_t layer,
                                        bool is_going_down) -> float {
    auto n = 0.0F;
    auto d = 0.0F;

    for (const auto& edge : node.edges()) {
        const auto child = edge.other(node);

        if (layers_.get(child) == layer) {
            const auto w          = edge_weights_.get(edge);
            const auto& waypoints = waypoints_.get(edge);
            auto waypoint_offset  = waypoints[1].x - waypoints[2].x;

            if (is_going_down) {
                waypoint_offset *= -1;
            }

            n += (xs_.get(child) + waypoint_offset) * w;
            d += w;
        }
    }

    if (d == 0.0F) {
        return -1.0F;
    }

    return n / d;
}

void SugiyamaAnalysis::coordinate_assignment_iteration(size_t layer,
                                                       size_t next_layer,
                                                       float graph_width) {
    auto nodes = node_layers_[layer];

    auto sorted_indexes =
        std::views::iota(static_cast<size_t>(0), nodes.size()) |
        std::ranges::to<std::vector<size_t>>();

    std::ranges::sort(sorted_indexes, [&](size_t a, size_t b) {
        auto pa = priorities_.get(nodes[a]);
        auto pb = priorities_.get(nodes[b]);
        return (pa > pb);
    });

    for (auto i : sorted_indexes) {
        const auto& node = nodes[i];

        auto lo = min_x(nodes, i);
        auto hi = max_x(nodes, i, graph_width);

        if (std::abs(hi - lo) < 0.01) {
            hi = lo;
        }

        if (lo > hi) {
            lo = (lo + hi) / 2;
            hi = lo;
            // FIXME: weird
            // fmt::print("LO/HI error\n");
        }

        assert(lo <= hi);

        auto avg = average_position(node, next_layer, next_layer < layer);

        if (avg >= 0) {
            auto x = std::clamp(avg, lo, hi);
            xs_.set(node, x);
        } else {
            auto x = std::clamp(xs_.get(node), lo, hi);
            xs_.set(node, x);
        }
    }
}

auto SugiyamaAnalysis::compute_graph_width() -> float {
    auto graph_width = 0.0F;

    for (const auto& layer : node_layers_) {
        auto layer_width = X_GUTTER;

        for (const auto& node : layer) {
            layer_width += widths_.get(node) + X_GUTTER;
        }

        graph_width = std::max(graph_width, layer_width);
    }

    return graph_width;
}

auto SugiyamaAnalysis::get_graph_width() const -> float {
    return width_;
}

auto SugiyamaAnalysis::get_graph_height() const -> float {
    return height_;
}

auto SugiyamaAnalysis::compute_graph_height() -> float {
    auto y         = 0.0F;
    auto layer_gap = 0.0F;

    if (has_top_loop_) {
        y -= 2 * Y_GUTTER;
    }

    // The highest layer is on top
    for (size_t layer = layer_count_ - 1; layer < layer_count_; --layer) {
        // The height of the biggest node in this layer
        auto layer_height = 0.0F;

        // The space between this layer and the next
        layer_gap = 2.0F * Y_GUTTER;

        auto nodes = g.nodes() | layer_view(layer) |
                     std::ranges::to<std::vector<Node>>();
        for (const auto& node : nodes) {
            layer_height = std::max(layer_height, heights_.get(node));
            layer_gap +=
                static_cast<float>(node.child_edges().size()) * EDGE_HEIGHT;
        }

        if (layer_gap == 2.0F * Y_GUTTER) {
            // No edges in this gap
            layer_gap = 0;
        }

        y += layer_height + layer_gap;
    }

    if (has_bottom_loop_) {
        y -= 2 * Y_GUTTER;
    }

    return y;
}

void SugiyamaAnalysis::x_coordinate_assignment() {
    auto priorities = NodeAttribute<size_t>{g.max_node_id(), 0};

    const float graph_width = compute_graph_width();

    // Init X coordinates
    for (size_t layer = 0; layer < layer_count_; ++layer) {
        auto nodes = node_layers_[layer];

        std::ranges::sort(nodes, [&](const Node& a, const Node& b) {
            return orders_.get(a) < orders_.get(b);
        });

        auto x = X_GUTTER;

        for (const auto& node : nodes) {
            xs_.set(node, x);
            x += widths_.get(node) + X_GUTTER;
        }
    }

    for (size_t i = 0; i < 5; ++i) {
        for (size_t r = 0 + 1; r < layer_count_; ++r) {
            coordinate_assignment_iteration(r, r - 1, graph_width);
        }

        for (size_t r = layer_count_ - 1; r < layer_count_; --r) {
            coordinate_assignment_iteration(r, r + 1, graph_width);
        }
    }

    for (size_t r = 0 + 1; r < layer_count_; ++r) {
        coordinate_assignment_iteration(r, r - 1, graph_width);
    }
}

auto SugiyamaAnalysis::get_x(NodeId node) const -> float {
    return xs_.get(node);
}
auto SugiyamaAnalysis::get_y(NodeId node) const -> float {
    return ys_.get(node);
}

auto SugiyamaAnalysis::get_width(NodeId node) const -> float {
    return widths_.get(node);
}

auto SugiyamaAnalysis::get_height(NodeId node) const -> float {
    return heights_.get(node);
}

auto SugiyamaAnalysis::get_waypoints(EdgeId edge) const
    -> const std::vector<Point>& {
    return waypoints_.get(edge);
}

// TODO: it's kind of odd that the xs are offsets and ys are coords
void SugiyamaAnalysis::waypoint_creation() {
    for (const auto& edge : g.edges()) {
        auto& waypoints = waypoints_.get(edge);
        waypoints.resize(4, {.x = 0.0F, .y = 0.0F});
    }

    for (size_t layer = 0; layer < layer_count_; ++layer) {
        // Sort the nodes by order
        auto nodes = node_layers_[layer];

        std::ranges::sort(nodes, [&](const Node& a, const Node& b) {
            return orders_.get(a) < orders_.get(b);
        });

        // EXIT EDGES
        for (const auto& node : nodes) {
            auto y0 = ys_.get(node) + heights_.get(node);

            // Sort the edges by destination order
            auto edges = node.child_edges();
            std::ranges::sort(edges, [&](const Edge& a, const Edge& b) {
                auto order_a = orders_.get(a.to());
                auto order_b = orders_.get(b.to());

                if (order_a == order_b) {
                    return end_x_offset_.get(a) < end_x_offset_.get(b);
                }

                return order_a < order_b;
            });

            auto spacer =
                widths_.get(node) / static_cast<float>(edges.size() + 1);

            auto x = spacer;

            for (const auto& edge : edges) {
                // Creates 4 waypoints (Xs):
                // We calculate the x and y coordinates of the first 2 nodes
                // And the y coordinate of the extremities
                // |__X__|
                //    |
                //    X----X
                //         |
                //       __X__
                //      |     |

                assert(ys_.get(edge.to()) > ys_.get(edge.from()));

                auto& waypoints = waypoints_.get(edge);

                if (start_x_offset_.get(edge) < 0) {
                    waypoints[0].x = x;
                    waypoints[1].x = x;
                    start_x_offset_.set(edge, x);
                } else {
                    // The value is imposed
                    waypoints[0].x = start_x_offset_.get(edge);
                    waypoints[1].x = start_x_offset_.get(edge);
                }

                waypoints[0].y = y0;
                waypoints[3].y = ys_.get(edge.to());

                x += spacer;
            }
        }

        // SPECIAL HANDLING OF EXIT NODE
        for (auto exit_pair : exits) {
            auto edge = io_edges_[exit_pair];
            if (start_x_offset_.get(edge) < 0) {
                continue;
            }

            auto& waypoints = waypoints_.get(edge);
            waypoints[0].x  = start_x_offset_.get(edge);
            waypoints[1].x  = start_x_offset_.get(edge);
        }

        // ENTRY EDGES
        for (const auto& node : nodes) {
            auto edges = node.parent_edges();
            std::ranges::sort(edges, [&](const Edge& a, const Edge& b) {
                auto order_a = orders_.get(a.from());
                auto order_b = orders_.get(b.from());

                // TODO: lexicographic comparison to account for back edges
                // For this I need to know if it's coming from the left or
                // right...

                if (order_a == order_b) {
                    return start_x_offset_.get(a) < start_x_offset_.get(b);
                }

                return order_a < order_b;
            });

            auto spacer =
                widths_.get(node) / static_cast<float>(edges.size() + 1);

            auto x = spacer;

            for (const auto& edge : edges) {
                // Creates 4 waypoints (Xs):
                // We calculate the x and y coordinates of the last 2 nodes
                // |__X__|
                //    |
                //    X----X
                //         |
                //       __X__
                //      |     |

                auto& waypoints = waypoints_.get(edge);

                if (end_x_offset_.get(edge) < 0) {
                    waypoints[2].x = x;
                    waypoints[3].x = x;
                    end_x_offset_.set(edge, x);
                } else {
                    // The value is imposed
                    waypoints[2].x = end_x_offset_.get(edge);
                    waypoints[3].x = end_x_offset_.get(edge);
                }

                x += spacer;
            }
        }

        // SPECIAL HANDLING OF Entry NODE
        for (auto entry_pair : exits) {
            auto edge = io_edges_[entry_pair];

            if (end_x_offset_.get(edge) < 0) {
                continue;
            }

            auto& waypoints = waypoints_.get(edge);
            waypoints[2].x  = end_x_offset_.get(edge);
            waypoints[3].x  = end_x_offset_.get(edge);
        }
    }
}

// Consider edges are overlapping if they are too close
float TOLERANCE = 10.0F;

// Use dynamic programming to determine edge heights
//
// We want to ensure the down portion of an edge does not cross with the
// horizontal part of another edge.
//
// This is a topological sort. This function is roughly the DFS solution
//
// In the example we want to ensure 1-2 is on a higher level than b-c
// because c.x is in [1.x, 2.x]
//
//  a  0       |
//  |  |      \/
//  |  1 ------- 2
//  |            |
//  b---------c  |
//            |  |
//            d  3
//
// NOLINTNEXTLINE(misc-no-recursion)
auto SugiyamaAnalysis::get_waypoint_y(size_t id,
                                      const std::vector<Edge>& edges,
                                      std::vector<int64_t>& layers) -> int64_t {
    if (layers[id] != std::numeric_limits<int64_t>::min()) {
        // This includes std::numeric_limits<int64_t>::max()
        return layers[id];
    }

    // Marker
    layers[id] = std::numeric_limits<int64_t>::max();

    const auto& edge = edges[id];

    // Find all segments that contain the third waypoint (2) (It's the top of
    // 2-3, the segment going down)
    //
    //  0
    //  |
    //  1 --- 2
    //        |
    //        3
    //

    const auto& waypoints = waypoints_.get(edge);

    auto x1 = waypoints[1].x;
    auto x2 = waypoints[2].x;

    // Add a temporary mark to this node to detect infinite loops

    // The layer the edge 1-2 will rest on
    int64_t lmax = std::numeric_limits<int64_t>::min();
    int64_t lmin = std::numeric_limits<int64_t>::max();

    for (size_t i = 0; i < edges.size(); i++) {
        if (i == id) {
            continue;
        }

        const auto& other           = edges[i];
        const auto& waypoints_other = waypoints_.get(other);

        auto other_start = std::min(waypoints_other[1].x, waypoints_other[2].x);
        auto other_end   = std::max(waypoints_other[1].x, waypoints_other[2].x);

        if (other_start - TOLERANCE <= x2 && x2 <= other_end + TOLERANCE) {
            auto l = get_waypoint_y(i, edges, layers);

            if (l == std::numeric_limits<int64_t>::max()) {
                // We are in a loop: the system is over constrained
                // Example:
                // 0          a
                // |          |
                // 1 --- 2    |
                //       |    |
                //  c ---+--- b
                //  |    |
                //  d    3

                // We'll ignore this constraint
                continue;
            }

            lmin = std::min(l - 1, lmin);
        }

        else if (other_start - TOLERANCE <= x1 && x1 <= other_end + TOLERANCE) {
            // We don't want to perform a recursive call
            auto l = layers[i];

            if (l == std::numeric_limits<int64_t>::max() ||
                l == std::numeric_limits<int64_t>::min()) {
                continue;
            }

            lmax = std::max(l + 1, lmax);
        }
    }

    int64_t layer = 0;

    if (lmin != std::numeric_limits<int64_t>::max()) {
        layer = lmin;
    } else if (lmax != std::numeric_limits<int64_t>::min()) {
        layer = lmax;
    }

    layers[id] = layer;
    return layer;
}

void SugiyamaAnalysis::calculate_waypoints_y() {
    for (size_t layer = 0; layer < layer_count_; ++layer) {
        // Sort the nodes by order
        auto edges = std::vector<Edge>{};

        for (const auto& node : node_layers_[layer]) {
            for (const auto& edge : node.child_edges()) {
                edges.push_back(edge);
            }
        }

        auto layers       = std::vector<int64_t>(edges.size(),
                                                 std::numeric_limits<int64_t>::min());
        int64_t max_layer = std::numeric_limits<int64_t>::min();
        int64_t min_layer = std::numeric_limits<int64_t>::max();

        for (size_t i = 0; i < edges.size(); ++i) {
            auto layer = get_waypoint_y(i, edges, layers);
            max_layer  = std::max(layer, max_layer);
            min_layer  = std::min(layer, min_layer);
        }

        for (size_t i = 0; i < edges.size(); ++i) {
            const auto& edge = edges[i];
            auto& waypoints  = waypoints_.get(edge);

            waypoints[1].y =
                waypoints[3].y - Y_GUTTER -
                static_cast<float>(layers[i] - min_layer) * EDGE_HEIGHT;
            waypoints[2].y =
                waypoints[3].y - Y_GUTTER -
                static_cast<float>(layers[i] - min_layer) * EDGE_HEIGHT;
        }
    }
}

void SugiyamaAnalysis::translate_waypoints() {
    for (const auto& edge : g.edges()) {
        auto& waypoints = waypoints_.get(edge);

        waypoints[0].x += xs_.get(edge.from());
        waypoints[1].x += xs_.get(edge.from());

        waypoints[2].x += xs_.get(edge.to());
        waypoints[3].x += xs_.get(edge.to());
    }
}

void SugiyamaAnalysis::flip_edges() {
    auto& ge = g.editor();
    for (const auto& edge : g.edges()) {
        assert(layers_.get(edge.to()) != layers_.get(edge.from()));

        if (layers_.get(edge.from()) < layers_.get(edge.to())) {
            ge.edit_edge(edge, edge.to(), edge.from());
        }
    }
}

void SugiyamaAnalysis::y_coordinate_assignment() {
    auto y = 0.0F;

    // The highest layer is on top
    for (size_t layer = layer_count_ - 1; layer <= layer_count_; --layer) {
        // The height of the biggest node in this layer
        auto layer_height = 0.0F;

        // The space between this layer and the next
        auto layer_gap = 2.0F * Y_GUTTER;

        auto nodes = node_layers_[layer];
        for (const auto& node : nodes) {
            ys_.set(node, y);
            layer_height = std::max(layer_height, heights_.get(node));
            layer_gap +=
                static_cast<float>(node.child_edges().size()) * EDGE_HEIGHT;
        }

        if (layer_gap == 2.0F * Y_GUTTER) {
            // No edges in this gap
            layer_gap = 0.0F;
        }

        y += layer_height + layer_gap;
    }
}

void SugiyamaAnalysis::ensure_io_at_extremities() {
    auto top_layer = layer_count_;
    layer_count_ += 1;

    // Creates nodes at the top layer that entry nodes are linked to
    for (auto entry_pair : entries) {
        const auto ghost      = create_ghost_node(top_layer);
        auto& editor          = g.editor();
        auto edge             = editor.make_edge(ghost, entry_pair.node);
        io_edges_[entry_pair] = edge;
    }

    // Creates nodes at the bottom layer that exit nodes are linked to
    for (auto exit_pair : exits) {
        const auto ghost     = create_ghost_node(0);
        auto& editor         = g.editor();
        auto edge            = editor.make_edge(exit_pair.node, ghost);
        io_edges_[exit_pair] = edge;
    }

    // These are not loops!
    has_top_loop_    = true;
    has_bottom_loop_ = true;
}

auto SugiyamaAnalysis::get_io_waypoints() const
    -> const std::map<Pair, std::vector<Point>>& {
    return io_waypoints_;
}

void SugiyamaAnalysis::make_io_waypoint(IOPair pair) {
    auto edge = io_edges_[pair];
    assert(edge != EdgeId::InvalidID);
    build_waypoints(edge);
    io_waypoints_[pair] = waypoints_.get(edge);
}

void SugiyamaAnalysis::make_io_waypoints() {
    for (auto pair : entries) {
        make_io_waypoint(pair);
    }

    for (auto pair : exits) {
        make_io_waypoint(pair);
    }
}

void SugiyamaAnalysis::build_long_edges_waypoints() {
    for (const auto id : deleted_edges_) {
        build_waypoints(id);
    }
}

void SugiyamaAnalysis::build_waypoints(EdgeId id) {
    auto& waypoints      = waypoints_.get(id);
    auto& edge_waypoints = edge_waypoints_.get(id);

    // No waypoints to build
    if (edge_waypoints.empty()) {
        return;
    }

    for (const auto& edge : g.get_edges(edge_waypoints)) {
        auto& ws = waypoints_.get(edge);
        if (layers_.get(edge.from()) < layers_.get(edge.to())) {
            waypoints.push_back(ws[0]);
            waypoints.push_back(ws[1]);
            waypoints.push_back(ws[2]);
            waypoints.push_back(ws[3]);
        } else {
            waypoints.push_back(ws[3]);
            waypoints.push_back(ws[2]);
            waypoints.push_back(ws[1]);
            waypoints.push_back(ws[0]);
        }
    }
}