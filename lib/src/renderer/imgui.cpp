#include <algorithm>
#include <memory>
#include <string>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

#include "./external/imgui_canvas.h"
#include "triskel/triskel.hpp"
#include "triskel/utils/point.hpp"

/// NOLINTNEXTLINE(google-build-using-namespace)
using namespace triskel;

namespace {

struct ImGuiRendererImpl : public ImguiRenderer {
    ImGuiRendererImpl() {
        STYLE_BASICBLOCK_BORDER = {.thickness = 3.0F, .color = Black};
        STYLE_EDGE              = {.thickness = 2.0F, .color = Black};
        STYLE_EDGE_T            = {.thickness = 2.0F, .color = Green};
        STYLE_EDGE_F            = {.thickness = 2.0F, .color = Red};
        BLOCK_PADDING           = 20.0F;
        TRIANGLE_SIZE           = 20.0F;
        PADDING                 = 200.0F;
    }

    void Begin(const char* id, Point size) override {
        auto canvas_rect = canvas_.Rect();
        auto view_rect   = canvas_.ViewRect();

        canvas_.Begin(id, convert(size));

        if (should_refit) {
            refit(width_, height_);
            should_refit = false;
        }

        translate();
        scale();
    }

    void scale() {
        auto& io         = ImGui::GetIO();
        auto view_scale  = canvas_.ViewScale();
        auto view_origin = canvas_.ViewOrigin();

        if (ImGui::IsItemHovered() && (io.MouseWheel != 0)) {
            auto scale = view_scale;
            if (view_scale > 1) {
                scale += io.MouseWheel;
            } else {
                if (io.MouseWheel > 0) {
                    scale *= 2;
                } else {
                    scale /= 2;
                }
            }

            // TODO: clamp zoom
            // TODO: zoom not to corner
            canvas_.SetView((view_origin / view_scale * scale), scale);
        }
    }

    void translate() {
        auto view_scale         = canvas_.ViewScale();
        auto view_origin        = canvas_.ViewOrigin();
        static bool is_dragging = false;
        static ImVec2 draw_start_point;

        if ((is_dragging || ImGui::IsItemHovered()) &&
            ImGui::IsMouseDragging(1, 0.0F)) {
            if (!is_dragging) {
                is_dragging      = true;
                draw_start_point = view_origin;
            }

            canvas_.SetView(draw_start_point +
                                ImGui::GetMouseDragDelta(1, 0.0F) * view_scale,
                            view_scale);
        } else if (is_dragging) {
            is_dragging = false;
        }
    }

    void End() override { canvas_.End(); }

    void draw_line(Point start, Point end, const StrokeStyle& style) override {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        auto start_ = convert(start);
        auto end_   = convert(end);

        auto color = convert(style.color);

        draw_list->AddLine(start_, end_, color, style.thickness);
    }

    void draw_triangle(Point v1, Point v2, Point v3, Color fill) override {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        auto v1_ = convert(v1);
        auto v2_ = convert(v2);
        auto v3_ = convert(v3);

        auto fill_ = convert(fill);

        draw_list->AddTriangleFilled(v1_, v2_, v3_, fill_);
    };

    void draw_rectangle(Point tl,
                        float width,
                        float height,
                        Color fill) override {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        auto top_left     = convert(tl);
        auto bottom_right = top_left + ImVec2{width, height};

        auto color = convert(fill);

        draw_list->AddRectFilled(top_left, bottom_right, color);
    };

    void draw_rectangle_border(Point tl,
                               float width,
                               float height,
                               const StrokeStyle& style) override {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        auto top_left     = convert(tl);
        auto bottom_right = top_left + ImVec2{width, height};

        auto color = convert(style.color);

        draw_list->AddRect(top_left, bottom_right, color, 0.0F, 0,
                           style.thickness);
    };

    void draw_text(Point tl,
                   const std::string& text,
                   const TextStyle& style) override {
        ImGui::PushStyleColor(ImGuiCol_Text, convert(style.color));

        ImGui::SetCursorPos(
            convert(tl + Point{.x = BLOCK_PADDING, .y = BLOCK_PADDING}));
        ImGui::TextUnformatted(text.c_str());

        ImGui::PopStyleColor();
    };

    [[nodiscard]] auto measure_text(const std::string& text,
                                    const TextStyle& /*style*/) const
        -> Point override {
        return convert(ImGui::CalcTextSize(text.c_str())) +
               Point{.x = 2 * BLOCK_PADDING, .y = 2 * BLOCK_PADDING};
    }

    [[nodiscard]] static auto convert(Point pt) -> ImVec2 {
        return {pt.x, pt.y};
    }

    [[nodiscard]] static auto convert(ImVec2 v) -> Point {
        return {.x = v.x, .y = v.y};
    }

    [[nodiscard]] static auto convert(Color color) -> ImU32 {
        return IM_COL32(color.r, color.g, color.b, color.a);
    }

    void fit(float width, float height) override {
        should_refit = true;
        width_       = width;
        height_      = height;
    }

    void refit(float width, float height) {
        auto view_scale  = canvas_.ViewScale();
        auto view_origin = canvas_.ViewOrigin();

        auto canvas_size = canvas_.ViewRect();

        if (canvas_size.GetHeight() == 0 || canvas_size.GetWidth() == 0) {
            // TODO: center the first function
            return;
        }

        const float scale_x =
            canvas_size.GetWidth() / (width + 2 * PADDING) * view_scale;
        const float scale_y =
            canvas_size.GetHeight() / (height + 2 * PADDING) * view_scale;
        view_scale = std::min(scale_x, scale_y);
        canvas_.SetView(view_origin, view_scale);

        canvas_.CenterView(ImVec2{width / 2, height / 2});
    }

    ImGuiEx::Canvas canvas_;

    bool should_refit = false;
    float width_;
    float height_;
};

}  // namespace

auto triskel::make_imgui_renderer() -> std::unique_ptr<ImguiRenderer> {
    return std::make_unique<ImGuiRendererImpl>();
}