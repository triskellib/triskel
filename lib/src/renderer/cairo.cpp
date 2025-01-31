#include "triskel/triskel.hpp"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <ranges>
#include <string>

#include <cairo/cairo-svg.h>
#include <cairo/cairo.h>

#include "triskel/utils/point.hpp"

/// NOLINTNEXTLINE(google-build-using-namespace)
using namespace triskel;

namespace {
struct CairoRenderer : public ExportingRenderer {
    CairoRenderer() {
        text_surface = make_image_surface(CAIRO_FORMAT_ARGB32, 1, 1);
        text_cr      = make_cairo(text_surface.get());
    }

    void begin(float width, float height) override {
        surface = make_recording_surface(width + (2 * PADDING),
                                         height + (2 * PADDING));
        cr      = make_cairo(surface.get());

        // Background color
        cairo_set_source_rgb(cr.get(), 1.0, 1.0, 1.0);
        cairo_paint(cr.get());

        cairo_translate(cr.get(), PADDING, PADDING);
    }

    void draw_line(Point start, Point end, const StrokeStyle& style) override {
        set_color(style.color);
        cairo_set_line_width(cr.get(), style.thickness);
        cairo_move_to(cr.get(), start.x, start.y);
        cairo_line_to(cr.get(), end.x, end.y);
        cairo_stroke(cr.get());
    }

    void draw_triangle(Point v1, Point v2, Point v3, Color fill) override {
        set_color(fill);

        cairo_move_to(cr.get(), v1.x, v1.y);
        cairo_line_to(cr.get(), v2.x, v2.y);
        cairo_line_to(cr.get(), v3.x, v3.y);
        cairo_close_path(cr.get());

        cairo_fill(cr.get());
    };

    void draw_rectangle(Point tl,
                        float width,
                        float height,
                        Color fill) override {
        set_color(fill);
        cairo_rectangle(cr.get(), tl.x, tl.y, width, height);
        cairo_fill(cr.get());
    };

    void draw_rectangle_border(Point tl,
                               float width,
                               float height,
                               const StrokeStyle& style) override {
        set_color(style.color);

        cairo_set_line_width(cr.get(), style.thickness);
        cairo_rectangle(cr.get(), tl.x, tl.y, width, height);
        cairo_stroke(cr.get());
    };

    // TODO: add Pango support
    void draw_text(Point tl,
                   const std::string& text,
                   const TextStyle& style) override {
        set_color(style.color);

        cairo_select_font_face(cr.get(), "Sans", CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr.get(), style.size);

        tl.y += BLOCK_PADDING + STYLE_TEXT.line_height;
        tl.x += BLOCK_PADDING;
        cairo_move_to(cr.get(), tl.x, tl.y);

        for (auto&& line : std::views::split(text, '\n')) {
            auto str = std::string{line.begin(), line.end()};
            cairo_show_text(cr.get(), str.c_str());

            tl.y += STYLE_TEXT.line_height;
            cairo_move_to(cr.get(), tl.x, tl.y);
        }
    };

    // TODO: add Pango support
    [[nodiscard]] auto measure_text(const std::string& text,
                                    const TextStyle& style) const
        -> Point override {
        auto width  = 0.0F;
        auto height = 0.0F;

        cairo_select_font_face(text_cr.get(), "Sans", CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(text_cr.get(), style.size);

        cairo_text_extents_t extents;

        for (auto&& line : std::views::split(text, '\n')) {
            auto str = std::string{line.begin(), line.end()};
            cairo_text_extents(text_cr.get(), str.c_str(), &extents);
            height += STYLE_TEXT.line_height;
            width = std::max(width, static_cast<float>(extents.x_advance +
                                                       extents.x_bearing));
        }

        return {.x = width + (2 * BLOCK_PADDING),
                .y = height + (2 * BLOCK_PADDING)};
    }

    /// @brief Surface used when measuring text
    std::shared_ptr<cairo_surface_t> text_surface;

    /// @brief cr used when measuring text
    std::shared_ptr<cairo_t> text_cr;

    /// @brief Surface used when rendering
    std::shared_ptr<cairo_surface_t> surface;

    /// @brief cr used when rendering
    std::shared_ptr<cairo_t> cr;

    [[nodiscard]] static auto make_image_surface(_cairo_format format,
                                                 int width,
                                                 int height)
        -> std::shared_ptr<cairo_surface_t> {
        return {
            cairo_image_surface_create(format, width, height),
            [](cairo_surface_t* surface) { cairo_surface_destroy(surface); }};
    }

    [[nodiscard]] static auto make_svg_surface(
        const std::filesystem::path& path,
        int width,
        int height) -> std::shared_ptr<cairo_surface_t> {
        return {
            cairo_svg_surface_create(path.c_str(), width, height),
            [](cairo_surface_t* surface) { cairo_surface_destroy(surface); }};
    }

    [[nodiscard]] static auto make_recording_surface(double width,
                                                     double height)
        -> std::shared_ptr<cairo_surface_t> {
        auto extents = cairo_rectangle_t{0, 0, width, height};
        return {
            cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, &extents),
            [](cairo_surface_t* surface) { cairo_surface_destroy(surface); }};
    }

    [[nodiscard]] static auto make_cairo(cairo_surface_t* surface)
        -> std::shared_ptr<cairo_t> {
        return {cairo_create(surface), [](cairo_t* cr) { cairo_destroy(cr); }};
    }

    void set_color(Color color) {
        cairo_set_source_rgb(cr.get(), color.r / 255.0, color.g / 255.0,
                             color.b / 255.0);
    }
};

struct CairoPNGRenderer : CairoRenderer {
    void save(const std::filesystem::path& path) override {
        cairo_rectangle_t extents;
        cairo_recording_surface_get_extents(surface.get(), &extents);

        auto png_surface = make_image_surface(CAIRO_FORMAT_ARGB32,
                                              static_cast<int>(extents.width),
                                              static_cast<int>(extents.height));
        auto png_cr      = make_cairo(png_surface.get());

        // Replay recorded drawing
        cairo_set_source_surface(png_cr.get(), surface.get(), 0, 0);
        cairo_paint(png_cr.get());

        cairo_surface_write_to_png(png_surface.get(), path.c_str());
    };
};

struct CairoSVGRenderer : CairoRenderer {
    void save(const std::filesystem::path& path) override {
        cairo_rectangle_t extents;
        cairo_recording_surface_get_extents(surface.get(), &extents);

        auto svg_surface =
            make_svg_surface(path, static_cast<int>(extents.width),
                             static_cast<int>(extents.height));
        auto svg_cr = make_cairo(svg_surface.get());

        // Replay recorded drawing
        cairo_set_source_surface(svg_cr.get(), surface.get(), 0, 0);
        cairo_paint(svg_cr.get());
    };
};

}  // namespace

auto triskel::make_png_renderer() -> std::unique_ptr<ExportingRenderer> {
    return std::make_unique<CairoPNGRenderer>();
}

auto triskel::make_svg_renderer() -> std::unique_ptr<ExportingRenderer> {
    return std::make_unique<CairoSVGRenderer>();
}
