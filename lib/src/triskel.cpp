#include "triskel/triskel.hpp"
#include "triskel/internal.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "triskel/graph/graph.hpp"
#include "triskel/graph/igraph.hpp"
#include "triskel/layout/layout.hpp"
#include "triskel/utils/attribute.hpp"
#include "triskel/utils/point.hpp"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace triskel;

namespace {

auto get_node_id(const IGraph& g, size_t node) -> NodeId {
    if (node >= g.max_node_id()) {
        throw std::invalid_argument("ID does not belong to the graph");
    }

    return NodeId{node};
}

auto get_edge_id(const IGraph& g, size_t edge) -> EdgeId {
    if (edge >= g.max_edge_id()) {
        throw std::invalid_argument("ID does not belong to the graph");
    }

    return EdgeId{edge};
}

auto truncate_str(const std::string& str,
                  const std::string& ellipsis = "(...)",
                  size_t max_length           = 80) -> std::string {
    const size_t ellipsisLength = ellipsis.length();

    if (str.length() <= max_length) {
        return str;
    }

    return str.substr(0, max_length - ellipsisLength) + ellipsis;
}

struct CFGLayoutImpl : CFGLayout {
    CFGLayoutImpl(std::unique_ptr<Graph> graph,
                  const NodeAttribute<std::string>& labels,
                  const NodeAttribute<float>& widths,
                  const NodeAttribute<float>& heights,
                  const EdgeAttribute<LayoutBuilder::EdgeType>& edge_types)
        : graph_{std::move(graph)},
          labels_{labels},
          widths_{widths},
          heights_{heights},
          edge_types_(edge_types),
          layout_{*graph_, heights_, widths_} {}

    [[nodiscard]] auto get_coords(size_t node) const -> Point override {
        auto id = get_node_id(*graph_, node);
        return layout_.get_xy(id);
    }

    [[nodiscard]] auto get_waypoints(size_t edge) const
        -> const std::vector<Point>& override {
        auto id = get_edge_id(*graph_, edge);
        return layout_.get_waypoints(id);
    }

    [[nodiscard]] auto get_height() const -> float override {
        return layout_.get_graph_height(*graph_);
    }

    [[nodiscard]] auto get_width() const -> float override {
        return layout_.get_graph_width(*graph_);
    }

    [[nodiscard]] auto node_count() const -> size_t override {
        return graph_->node_count();
    }

    [[nodiscard]] auto edge_count() const -> size_t override {
        return graph_->edge_count();
    }

    void render(Renderer& render) const override {
        auto width  = get_width();
        auto height = get_height();

        render.begin(width, height);

        // Draws the nodes
        for (const auto& node : graph_->nodes()) {
            auto tl     = layout_.get_xy(node);
            auto width  = widths_.get(node);
            auto height = heights_.get(node);

            render.draw_rectangle_border(tl, width, height,
                                         render.STYLE_BASICBLOCK_BORDER);

            render.draw_text(tl, labels_.get(node), render.STYLE_TEXT);
        }

        // Draws the edges
        for (const auto& edge : graph_->edges()) {
            const auto& waypoints = layout_.get_waypoints(edge);

            if (waypoints.empty()) {
                continue;
                // FIXME:
                throw std::invalid_argument("ERROR during layout");
            }

            // Gets the style for this edge
            auto style = render.STYLE_EDGE;

            auto t = edge_types_.get(edge);
            if (t == LayoutBuilder::EdgeType::True) {
                style = render.STYLE_EDGE_T;
            } else if (t == LayoutBuilder::EdgeType::False) {
                style = render.STYLE_EDGE_F;
            }

            auto last = waypoints.front();

            for (size_t i = 1; i < waypoints.size(); ++i) {
                auto waypoint = waypoints[i];

                render.draw_line(last, waypoint, style);

                last = waypoint;
            }

            render.draw_triangle(last,
                                 last + Point{.x = -render.TRIANGLE_SIZE / 2,
                                              .y = -render.TRIANGLE_SIZE},
                                 last + Point{.x = +render.TRIANGLE_SIZE / 2,
                                              .y = -render.TRIANGLE_SIZE},
                                 style.color);
        }

        render.end();
    }

    void render_and_save(ExportingRenderer& renderer,
                         const std::filesystem::path& path) const override {
        render(renderer);
        renderer.save(path);
    }

    std::unique_ptr<Graph> graph_;
    NodeAttribute<std::string> labels_;
    NodeAttribute<float> widths_;
    NodeAttribute<float> heights_;
    EdgeAttribute<LayoutBuilder::EdgeType> edge_types_;

    Layout layout_;
};

struct LayoutBuilderImpl : LayoutBuilder {
    LayoutBuilderImpl()
        : graph_{std::make_unique<Graph>()},
          widths_(0, 1.0F),
          heights_(0, 1.0F),
          labels_(0, ""),
          edge_types_(0, EdgeType::Default) {
        // Allow edits to this graph
        graph_->editor().push();
    }

    auto make_node() -> size_t override {
        return static_cast<size_t>(graph_->editor().make_node().id());
    }

    auto make_node(float height, float width) -> size_t override {
        auto node = graph_->editor().make_node().id();

        heights_.set(node, height);
        widths_.set(node, width);

        return static_cast<size_t>(node);
    }

    auto make_node(const std::string& label) -> size_t override {
        auto node = graph_->editor().make_node().id();

        auto bbox = get_string_size(label);

        widths_.set(node, bbox.x);
        heights_.set(node, bbox.y);

        labels_.set(node, label);

        return static_cast<size_t>(node);
    }

    auto make_node(const Renderer& render,
                   const std::string& label) -> size_t override {
        auto node = graph_->editor().make_node().id();
        auto bbox = render.measure_text(label, render.STYLE_TEXT);

        widths_.set(node, bbox.x);
        heights_.set(node, bbox.y);

        labels_.set(node, label);

        return static_cast<size_t>(node);
    }

    void measure_nodes(const Renderer& renderer) override {
        for (const auto& node : graph_->nodes()) {
            const auto& label = labels_.get(node);
            const auto bbox = renderer.measure_text(label, renderer.STYLE_TEXT);
            widths_.set(node, bbox.x);
            heights_.set(node, bbox.y);
        }
    }

    auto make_edge(size_t from, size_t to) -> size_t override {
        auto from_id = get_node_id(*graph_, from);
        auto to_id   = get_node_id(*graph_, to);

        auto edge = graph_->editor().make_edge(from_id, to_id);
        return static_cast<size_t>(edge.id());
    }

    auto make_edge(size_t from, size_t to, EdgeType type) -> size_t override {
        auto from_id = get_node_id(*graph_, from);
        auto to_id   = get_node_id(*graph_, to);

        auto edge = graph_->editor().make_edge(from_id, to_id);
        edge_types_.set(edge, type);
        return static_cast<size_t>(edge.id());
    }

    auto build() -> std::unique_ptr<CFGLayout> override {
        // End edits
        graph_->editor().commit();

        auto layout = std::make_unique<CFGLayoutImpl>(
            std::move(graph_), labels_, widths_, heights_, edge_types_);

        return layout;
    }

    auto graphviz() const -> std::string override { return format_as(*graph_); }

    std::unique_ptr<Graph> graph_;

    NodeAttribute<std::string> labels_;

    NodeAttribute<float> heights_;
    NodeAttribute<float> widths_;

    EdgeAttribute<LayoutBuilder::EdgeType> edge_types_;

    /// @brief Gets the bounding box of a string
    [[nodiscard]] static auto get_string_size(const std::string& str) -> Point {
        auto lines = 0.0F;

        auto width = 0.0F;

        for (auto&& line : std::views::split(str, '\n')) {
            lines++;

            width = std::max(width, static_cast<float>(line.size()));
        }

        return {.x = lines, .y = width};
    }
};

}  // namespace

Renderer::Renderer()
    : STYLE_BASICBLOCK_BORDER{.thickness = 6, .color = Black},
      STYLE_EDGE{.thickness = 4, .color = Black},
      STYLE_EDGE_T{.thickness = 4, .color = Green},
      STYLE_EDGE_F{.thickness = 4, .color = Red},
      STYLE_TEXT{.size = 40.0F, .line_height = 50.0F, .color = Black},
      BLOCK_PADDING{20.0F},
      PADDING{100.0F},
      TRIANGLE_SIZE{30.0F}

{}

auto triskel::make_layout_builder() -> std::unique_ptr<LayoutBuilder> {
    return std::make_unique<LayoutBuilderImpl>();
}

auto triskel::make_layout(std::unique_ptr<Graph> g,
                          const NodeAttribute<float>& width,
                          const NodeAttribute<float>& height,
                          const NodeAttribute<std::string>& label,
                          const EdgeAttribute<LayoutBuilder::EdgeType>&
                              edge_types) -> std::unique_ptr<CFGLayout> {
    return std::make_unique<CFGLayoutImpl>(std::move(g), label, width, height,
                                           edge_types);
}
