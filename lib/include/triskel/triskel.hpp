#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "triskel/utils/point.hpp"

namespace triskel {

struct Color {
    /// red
    uint8_t r;

    /// green
    uint8_t g;

    /// blue
    uint8_t b;

    /// alpha
    uint8_t a;

    [[nodiscard]] static auto from_hex(uint32_t rgba) -> Color;
};

constexpr Color Black{.r = 0, .g = 0, .b = 0, .a = 255};
constexpr Color Red{.r = 0xf4, .g = 0x43, .b = 0x36, .a = 255};
constexpr Color Green{.r = 0x4c, .g = 0xaf, .b = 0x50, .a = 255};

struct Renderer {
    struct StrokeStyle {
        float thickness;
        Color color;
    };

    struct TextStyle {
        float size;
        float line_height;
        Color color;
    };

    Renderer();

    virtual ~Renderer() = default;

    /// @brief Called before the drawing begins
    virtual void begin(float width, float height) {};

    /// @brief Called after the drawing ends
    virtual void end() {};

    /// @brief Draw a line from `start` to `end`
    virtual void draw_line(Point start,
                           Point end,
                           const StrokeStyle& style) = 0;

    /// @brief Draws a triangle between vertices v1, v2 and v3
    virtual void draw_triangle(Point v1, Point v2, Point v3, Color fill) = 0;

    /// @brief Draw a rectangle with top left at `tl`, width
    /// `width` and height `height`
    virtual void draw_rectangle(Point tl,
                                float width,
                                float height,
                                Color fill) = 0;

    /// @brief Draw the border of a rectangle with top left at `tl`, width
    /// `width` and height `height`
    virtual void draw_rectangle_border(Point tl,
                                       float width,
                                       float height,
                                       const StrokeStyle& style) = 0;

    /// @brief Draws text with top left at `tl`
    virtual void draw_text(Point tl,
                           const std::string& text,
                           const TextStyle& style) = 0;

    /// @brief Gets text dimension
    [[nodiscard]] virtual auto measure_text(const std::string& text,
                                            const TextStyle& style) const
        -> Point = 0;

    StrokeStyle STYLE_BASICBLOCK_BORDER;

    StrokeStyle STYLE_EDGE;

    StrokeStyle STYLE_EDGE_T;
    StrokeStyle STYLE_EDGE_F;

    TextStyle STYLE_TEXT;

    float BLOCK_PADDING;
    float PADDING;

    float TRIANGLE_SIZE;
};

struct ExportingRenderer : Renderer {
    ExportingRenderer() = default;

    ~ExportingRenderer() override = default;

    /// @param path the path where the render will be saved
    virtual void save(const std::filesystem::path& path) = 0;
};

struct CFGLayout {
    CFGLayout() = default;

    virtual ~CFGLayout() = default;

    /// @brief Return the x coordinate of the top left of the `node` basic
    /// block
    [[nodiscard]] virtual auto get_coords(size_t node) const -> Point = 0;

    /// @brief Returns the waypoints that the edge `edge` should follow
    [[nodiscard]] virtual auto get_waypoints(size_t edge) const
        -> const std::vector<Point>& = 0;

    /// @brief Returns the height of the graph
    [[nodiscard]] virtual auto get_height() const -> float = 0;

    /// @brief Returns the width of the graph
    [[nodiscard]] virtual auto get_width() const -> float = 0;

    /// @brief Returns the number of nodes
    [[nodiscard]] virtual auto node_count() const -> size_t = 0;

    /// @brief Returns the number of edges
    [[nodiscard]] virtual auto edge_count() const -> size_t = 0;

    /// @brief Renders the cfg
    virtual void render(Renderer& renderer) const = 0;

    /// @brief Save the cfg
    /// @param path the path where the render will be saved
    virtual void render_and_save(ExportingRenderer& renderer,
                                 const std::filesystem::path& path) const = 0;
};

struct LayoutBuilder {
    enum class EdgeType : uint8_t { Default, True, False };

    LayoutBuilder() = default;

    virtual ~LayoutBuilder() = default;

    /// @brief Adds a node to the graph.
    /// @return The id of the node in the graph
    virtual auto make_node() -> size_t = 0;

    /// @brief Adds a node to the graph.
    /// This node will have a custom size
    /// @return The id of the node in the graph
    virtual auto make_node(float height, float width) -> size_t = 0;

    /// @brief Adds a node to the graph.
    /// The label will determine the width and height of this node
    /// @return The id of the node in the graph
    virtual auto make_node(const std::string& label) -> size_t = 0;

    /// @brief Adds a node to the graph.
    /// The label will determine the width and height of this node using the
    /// renderer
    /// @return The id of the node in the graph
    virtual auto make_node(const Renderer& renderer,
                           const std::string& label) -> size_t = 0;

    /// @brief Calculates the dimension of each node using the renderer
    /// This will overwrite any existing widths and heights
    virtual void measure_nodes(const Renderer& renderer) = 0;

    /// @brief Adds an edge to the layout.
    /// @return The id of the edge in the graph
    virtual auto make_edge(size_t from, size_t to) -> size_t = 0;

    /// @brief Adds an edge to the layout.
    /// @return The id of the edge in the graph
    virtual auto make_edge(size_t from, size_t to, EdgeType type) -> size_t = 0;

    /// @brief Lays out the CFG
    [[nodiscard]] virtual auto build() -> std::unique_ptr<CFGLayout> = 0;
};

[[nodiscard]] auto make_layout_builder() -> std::unique_ptr<LayoutBuilder>;

}  // namespace triskel

#ifdef TRISKEL_CAIRO
namespace triskel {
/// @brief Builds a renderer to draw CFGs to a PNG file.
/// This required cairo
[[nodiscard]] auto make_png_renderer() -> std::unique_ptr<ExportingRenderer>;

/// @brief Builds a renderer to draw CFGs to an SVG file.
/// This required cairo
[[nodiscard]] auto make_svg_renderer() -> std::unique_ptr<ExportingRenderer>;
}  // namespace triskel
#endif

#ifdef TRISKEL_IMGUI
namespace triskel {

/// @brief A renderer which takes an id and size
struct ImguiRenderer : Renderer {
    ~ImguiRenderer() override = default;

    /// @brief Start the ImGui component
    /// The call to `render` should be performed between `Begin` and `End`
    virtual void Begin(const char* id, Point size) = 0;

    /// @brief End the ImGui component
    // NOLINTNEXTLINE(bugprone-virtual-near-miss)
    virtual void End() = 0;

    /// @brief Resize and center the canvas to the given dimensions
    virtual void fit(float width, float height) = 0;
};

/// @brief Builds a renderer to draw CFGs in Dear ImGui
/// This required imgui
[[nodiscard]] auto make_imgui_renderer() -> std::unique_ptr<ImguiRenderer>;
}  // namespace triskel
#endif

#ifdef TRISKEL_LLVM
#include <llvm/IR/Function.h>

namespace triskel {
[[nodiscard]] auto make_layout(llvm::Function* function,
                               Renderer* render = nullptr)
    -> std::unique_ptr<CFGLayout>;
}  // namespace triskel
#endif
