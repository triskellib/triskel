#include "triskel.hpp"

#include <algorithm>
#include <cassert>

#include <fmt/format.h>
#include <imgui.h>
#include <imgui_internal.h>

#include "triskel/triskel.hpp"

namespace {
auto Splitter(bool split_vertically,
              float thickness,
              float* size1,
              float* size2,
              float min_size1,
              float min_size2,
              float splitter_long_axis_size = -1.0F) -> bool {
    ImGuiContext& g     = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiID id          = window->GetID("##Splitter");
    ImRect bb;
    bb.Min = window->DC.CursorPos +
             (split_vertically ? ImVec2(*size1, 0.0F) : ImVec2(0.0F, *size1));
    bb.Max = bb.Min + ImGui::CalcItemSize(
                          split_vertically
                              ? ImVec2(thickness, splitter_long_axis_size)
                              : ImVec2(splitter_long_axis_size, thickness),
                          0.0F, 0.0F);
    return ImGui::SplitterBehavior(bb, id,
                                   split_vertically ? ImGuiAxis_X : ImGuiAxis_Y,
                                   size1, size2, min_size1, min_size2, 0.0F);
}
}  // namespace

void Triskel::select_function(const std::string& fname) {
    selected_function = fname;

    layout_ = arch->select_function(fname, *imgui_renderer_);

    imgui_renderer_->fit(layout_->get_width(), layout_->get_height());
}

Triskel::Triskel() : Application("Triskel") {}

void Triskel::OnStart(int argc, char** argv) {
    ImGui::StyleColorsLight();

    SetTitle("Triskel");

    // Image
    logo = LoadTexture(ASSET_PATH "/img/Triskel.png");

    if (argc <= 1) {
        fmt::print("Missing path\n");
    }
    std::string path = argv[1];

    if (path.ends_with(".bc") || path.ends_with(".ll")) {
        arch = make_llvm_arch();
    } else {
        arch = make_bin_arch();
    }

    imgui_renderer_ = triskel::make_imgui_renderer();

    arch->start(path);
}

void Triskel::OnFrame(float /*deltaTime*/) {
    auto availableRegion = ImGui::GetContentRegionAvail();

    static float s_SplitterSize  = 6.0F;
    static float s_SplitterArea  = 0.0F;
    static float s_LeftPaneSize  = 0.0F;
    static float s_RightPaneSize = 0.0F;

    if (s_SplitterArea != availableRegion.x) {
        if (s_SplitterArea == 0.0F) {
            s_SplitterArea = availableRegion.x;
            s_LeftPaneSize = ImFloor(availableRegion.x * 0.25F);
            s_RightPaneSize =
                availableRegion.x - s_LeftPaneSize - s_SplitterSize;
        } else {
            auto ratio     = availableRegion.x / s_SplitterArea;
            s_SplitterArea = availableRegion.x;
            s_LeftPaneSize = s_LeftPaneSize * ratio;
            s_RightPaneSize =
                availableRegion.x - s_LeftPaneSize - s_SplitterSize;
        }
    }

    Splitter(true, s_SplitterSize, &s_LeftPaneSize, &s_RightPaneSize, 100.0F,
             100.0F);

    FunctionSelector("##function_selection", ImVec2(s_LeftPaneSize, -1));

    ImGui::SameLine(0.0F, s_SplitterSize);

    if (layout_ != nullptr) {
        imgui_renderer_->Begin("##main_graph", {s_RightPaneSize, 0.0F});
        layout_->render(*imgui_renderer_);
        imgui_renderer_->End();
    } else {
        DrawLogo();
    }
}

void Triskel::DrawLogo() {
    auto cursor = ImGui::GetCursorScreenPos();

    auto width  = static_cast<float>(GetTextureWidth(logo));
    auto height = static_cast<float>(GetTextureHeight(logo));

    if (2 * width > ImGui::GetContentRegionAvail().x) {
        auto scale_factor = ImGui::GetContentRegionAvail().x / (2 * width);

        width *= scale_factor;
        height *= scale_factor;
    }

    float xOffset = (ImGui::GetContentRegionAvail().x - width) / 2.0F;
    float yOffset = (ImGui::GetContentRegionAvail().y - height) / 2.0F;

    // Ensure offsets are not negative
    xOffset = std::max(xOffset, 0.0F);
    yOffset = std::max(yOffset, 0.0F);

    // Adjust the cursor position
    ImGui::SetCursorPosX(cursor.x + xOffset);
    ImGui::SetCursorPosY(cursor.y + yOffset);
    ImGui::Image(logo, ImVec2(width, height));
}

void Triskel::FunctionSelector(const char* id, ImVec2 size) {
    ImGui::BeginChild(id, size, 0, 0);
    {
        static char filter[128] = "";
        ImGui::InputText("Search", filter, IM_ARRAYSIZE(filter));

        ImGui::BeginChild("function-search",
                          {ImGui::GetContentRegionAvail().x, 500.0F});

        // Move this to arch
        if (arch != nullptr) {
            for (const auto& item : arch->function_names) {
                auto black_text = (item == selected_function);
                // TODO: disable declarations
                // auto disable    = f->isDeclaration();
                if (strlen(filter) == 0 ||
                    item.find(filter) != std::string::npos) {
                    if (ImGui::Selectable(item.c_str(),
                                          item == selected_function)) {
                        if (item != selected_function) {
                            select_function(item);
                        }
                    }
                }
            }
        }

        ImGui::EndChild();

        ImGui::Separator();
    }
    ImGui::EndChild();
}
