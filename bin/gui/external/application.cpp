#include "application.h"
#include "platform.h"
#include "renderer.h"

extern "C" {
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "stb_image.h"
}

Application::Application(const char* name) : Application(name, 0, nullptr) {}

Application::Application(const char* name, int argc, char** argv)
    : m_Name(name),
      m_Platform(CreatePlatform(*this)),
      m_Renderer(CreateRenderer()) {
    m_Platform->ApplicationStart(argc, argv);
}

Application::~Application() {
    m_Renderer->Destroy();

    m_Platform->ApplicationStop();

    if (m_Context) {
        ImGui::DestroyContext(m_Context);
        m_Context = nullptr;
    }
}

bool Application::Create(int argc,
                         char** argv,
                         int width /*= -1*/,
                         int height /*= -1*/) {
    m_Context = ImGui::CreateContext();
    ImGui::SetCurrentContext(m_Context);

    if (!m_Platform->OpenMainWindow("Application", width, height))
        return false;

    if (!m_Renderer->Create(*m_Platform))
        return false;

    m_IniFilename = m_Name + ".ini";

    ImGuiIO& io = ImGui::GetIO();
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard
    // Controls
    io.IniFilename = m_IniFilename.c_str();
    io.LogFilename = nullptr;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(m_Platform->GetFramebufferScale());

    RecreateFontAtlas(m_Platform->GetFramebufferScale());

    m_Platform->AcknowledgeWindowScaleChanged();
    m_Platform->AcknowledgeFramebufferScaleChanged();

    Frame();

    OnStart(argc, argv);

    return true;
}

int Application::Run() {
    m_Platform->ShowMainWindow();

    while (m_Platform->ProcessMainWindowEvents()) {
        if (!m_Platform->IsMainWindowVisible())
            continue;

        Frame();
    }

    OnStop();

    return 0;
}

void Application::RecreateFontAtlas(float scale) {
    ImGuiIO& io = ImGui::GetIO();

    IM_DELETE(io.Fonts);

    io.Fonts = IM_NEW(ImFontAtlas);

    ImFontConfig config;
    config.OversampleH = 4;
    config.OversampleV = 4;
    config.PixelSnapH  = false;

    m_DefaultFont = io.Fonts->AddFontFromFileTTF(
        ASSET_PATH"/fonts/Roboto-Medium.ttf", 18.0f * scale, &config);
    m_HeaderFont = io.Fonts->AddFontFromFileTTF(ASSET_PATH"/fonts/Lato-Light.ttf",
                                                20.0f * scale, &config);

    io.Fonts->Build();
}

void Application::Frame() {
    auto& io = ImGui::GetIO();

    if (m_Platform->HasWindowScaleChanged()) {
        m_Platform->AcknowledgeWindowScaleChanged();
    }

    if (m_Platform->HasFramebufferScaleChanged()) {
        RecreateFontAtlas(m_Platform->GetFramebufferScale());
        m_Platform->AcknowledgeFramebufferScaleChanged();
    }

    const float windowScale      = 1.0f;  // m_Platform->GetWindowScale();
    const float framebufferScale = 1.0f;  // m_Platform->GetFramebufferScale();

    if (io.WantSetMousePos) {
        io.MousePos.x *= windowScale;
        io.MousePos.y *= windowScale;
    }

    m_Platform->NewFrame();

    // Don't touch "uninitialized" mouse position
    if (io.MousePos.x > -FLT_MAX && io.MousePos.y > -FLT_MAX) {
        io.MousePos.x /= windowScale;
        io.MousePos.y /= windowScale;
    }
    io.DisplaySize.x /= windowScale;
    io.DisplaySize.y /= windowScale;

    io.DisplayFramebufferScale.x = framebufferScale;
    io.DisplayFramebufferScale.y = framebufferScale;

    m_Renderer->NewFrame();

    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    const auto windowBorderSize = ImGui::GetStyle().WindowBorderSize;
    const auto windowRounding   = ImGui::GetStyle().WindowRounding;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::Begin("Content", nullptr, GetWindowFlags());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, windowBorderSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, windowRounding);

    OnFrame(io.DeltaTime);

    ImGui::PopStyleVar(2);
    ImGui::End();
    ImGui::PopStyleVar(2);

    // Rendering
    m_Renderer->Clear(ImColor(32, 32, 32, 255));
    ImGui::Render();
    m_Renderer->RenderDrawData(ImGui::GetDrawData());

    m_Platform->FinishFrame();
}

void Application::SetTitle(const char* title) {
    m_Platform->SetMainWindowTitle(title);
}

auto Application::Close() -> bool {
    return m_Platform->CloseMainWindow();
}

void Application::Quit() {
    m_Platform->Quit();
}

auto Application::GetName() const -> const std::string& {
    return m_Name;
}

auto Application::DefaultFont() const -> ImFont* {
    return m_DefaultFont;
}

auto Application::HeaderFont() const -> ImFont* {
    return m_HeaderFont;
}

auto Application::LoadTexture(const char* path) -> ImTextureID {
    int width     = 0;
    int height    = 0;
    int component = 0;

    if (auto data = stbi_load(path, &width, &height, &component, 4)) {
        auto texture = CreateTexture(data, width, height);
        stbi_image_free(data);
        return texture;
    }

    return 0;
}

auto Application::CreateTexture(const void* data,
                                int width,
                                int height) -> ImTextureID {
    return m_Renderer->CreateTexture(data, width, height);
}

void Application::DestroyTexture(ImTextureID texture) {
    m_Renderer->DestroyTexture(texture);
}

auto Application::GetTextureWidth(ImTextureID texture) -> int {
    return m_Renderer->GetTextureWidth(texture);
}

auto Application::GetTextureHeight(ImTextureID texture) -> int {
    return m_Renderer->GetTextureHeight(texture);
}

auto Application::GetWindowFlags() const -> ImGuiWindowFlags {
    return ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
           ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
           ImGuiWindowFlags_NoScrollWithMouse |
           ImGuiWindowFlags_NoSavedSettings |
           ImGuiWindowFlags_NoBringToFrontOnFocus;
}

/*
 * MIT License
 *
 * Copyright (c) 2019 Michał Cichoń
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
