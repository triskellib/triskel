#pragma once

#include <memory>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <GL/gl.h>
#include <imgui.h>
#include <imgui_internal.h>

#include "triskel/triskel.hpp"

#include "../external/application.h"
#include "arch/arch.hpp"

struct Triskel : public Application {
    using Application::Application;

    constexpr static auto GUTTER = 50.0F;
    constexpr static auto SIZE   = 100.0F;
    float x_gutter               = 0.0F;
    float y_gutter               = 0.0F;

    explicit Triskel();

    void OnFrame(float /*deltaTime*/) override;
    void OnStart(int argc, char** argv) override;

    void FunctionSelector(const char*, ImVec2 size);

    void DrawLogo();

    void select_function(const std::string& fname);
    std::string selected_function;

    std::unique_ptr<Arch> arch;
    std::unique_ptr<triskel::CFGLayout> layout_;
    std::unique_ptr<triskel::ImguiRenderer> imgui_renderer_;

    // Image
    ImTextureID logo;

    void style() const;
};