#include "platform.h"

#include "application.h"
#include "renderer.h"

#include <GLFW/glfw3.h>

#include <imgui.h>
#include "imgui_impl_glfw.h"

struct PlatformGLFW final : Platform {
    static PlatformGLFW* s_Instance;

    explicit PlatformGLFW(Application& application);

    auto ApplicationStart(int argc, char** argv) -> bool override;
    void ApplicationStop() override;
    auto OpenMainWindow(const char* title,
                        int width,
                        int height) -> bool override;
    auto CloseMainWindow() -> bool override;
    [[nodiscard]] auto GetMainWindowHandle() const -> void* override;
    void SetMainWindowTitle(const char* title) override;
    void ShowMainWindow() override;
    auto ProcessMainWindowEvents() -> bool override;
    [[nodiscard]] auto IsMainWindowVisible() const -> bool override;
    void SetRenderer(Renderer* renderer) override;
    void NewFrame() override;
    void FinishFrame() override;
    void Quit() override;

    void UpdatePixelDensity();

    Application& m_Application;
    GLFWwindow* m_Window = nullptr;
    bool m_QuitRequested = false;
    bool m_IsMinimized   = false;
    bool m_WasMinimized  = false;
    Renderer* m_Renderer = nullptr;
};

auto CreatePlatform(Application& application) -> std::unique_ptr<Platform> {
    return std::make_unique<PlatformGLFW>(application);
}

PlatformGLFW::PlatformGLFW(Application& application)
    : m_Application(application) {}

auto PlatformGLFW::ApplicationStart(int /*argc*/, char** /*argv*/) -> bool {
    return glfwInit() != 0;
}

void PlatformGLFW::ApplicationStop() {
    glfwTerminate();
}

auto PlatformGLFW::OpenMainWindow(const char* title,
                                  int width,
                                  int height) -> bool {
    if (m_Window != nullptr) {
        return false;
    }

    glfwWindowHint(GLFW_VISIBLE, 0);

    using InitializerType =
        bool (*)(GLFWwindow* window, bool install_callbacks);

    InitializerType initializer = nullptr;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    initializer = &ImGui_ImplGlfw_InitForOpenGL;
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GL_TRUE);

    width  = width < 0 ? 1440 : width;
    height = height < 0 ? 800 : height;

    m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (m_Window == nullptr) {
        return false;
    }

    if ((initializer == nullptr) || !initializer(m_Window, true)) {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
        return false;
    }

    glfwSetWindowUserPointer(m_Window, this);

    glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
        auto* self =
            reinterpret_cast<PlatformGLFW*>(glfwGetWindowUserPointer(window));
        if (!self->m_QuitRequested) {
            self->CloseMainWindow();
        }
    });

    glfwSetWindowIconifyCallback(m_Window, [](GLFWwindow* window,
                                              int iconified) {
        auto* self =
            reinterpret_cast<PlatformGLFW*>(glfwGetWindowUserPointer(window));
        if (iconified != 0) {
            self->m_IsMinimized  = true;
            self->m_WasMinimized = true;
        } else {
            self->m_IsMinimized = false;
        }
    });

    auto onFramebuferSizeChanged = [](GLFWwindow* window, int width,
                                      int height) {
        auto* self =
            reinterpret_cast<PlatformGLFW*>(glfwGetWindowUserPointer(window));
        if (self->m_Renderer != nullptr) {
            self->m_Renderer->Resize(width, height);
            self->UpdatePixelDensity();
        }
    };

    glfwSetFramebufferSizeCallback(m_Window, onFramebuferSizeChanged);

    auto onWindowContentScaleChanged = [](GLFWwindow* window, float /*xscale*/,
                                          float /*yscale*/) {
        auto* self =
            reinterpret_cast<PlatformGLFW*>(glfwGetWindowUserPointer(window));
        self->UpdatePixelDensity();
    };

    glfwSetWindowContentScaleCallback(m_Window, onWindowContentScaleChanged);

    UpdatePixelDensity();

    glfwMakeContextCurrent(m_Window);

    glfwSwapInterval(1);  // Enable vsync

    return true;
}

auto PlatformGLFW::CloseMainWindow() -> bool {
    if (m_Window == nullptr) {
        return true;
    }

    auto canClose = m_Application.CanClose();

    glfwSetWindowShouldClose(m_Window, canClose ? 1 : 0);

    return canClose;
}

auto PlatformGLFW::GetMainWindowHandle() const -> void* {
    return nullptr;
}

void PlatformGLFW::SetMainWindowTitle(const char* title) {
    glfwSetWindowTitle(m_Window, title);
}

void PlatformGLFW::ShowMainWindow() {
    if (m_Window == nullptr) {
        return;
    }

    glfwShowWindow(m_Window);
}

auto PlatformGLFW::ProcessMainWindowEvents() -> bool {
    if (m_Window == nullptr) {
        return false;
    }

    if (m_IsMinimized) {
        glfwWaitEvents();
    } else {
        glfwPollEvents();
    }

    if (m_QuitRequested || (glfwWindowShouldClose(m_Window) != 0)) {
        ImGui_ImplGlfw_Shutdown();

        glfwDestroyWindow(m_Window);

        return false;
    }

    return true;
}

bool PlatformGLFW::IsMainWindowVisible() const {
    if (m_Window == nullptr) {
        return false;
    }

    if (m_IsMinimized) {
        return false;
    }

    return true;
}

void PlatformGLFW::SetRenderer(Renderer* renderer) {
    m_Renderer = renderer;
}

void PlatformGLFW::NewFrame() {
    ImGui_ImplGlfw_NewFrame();

    if (m_WasMinimized) {
        ImGui::GetIO().DeltaTime = 0.1e-6F;
        m_WasMinimized           = false;
    }
}

void PlatformGLFW::FinishFrame() {
    if (m_Renderer != nullptr) {
        m_Renderer->Present();
    }

    glfwSwapBuffers(m_Window);
}

void PlatformGLFW::Quit() {
    m_QuitRequested = true;

    glfwPostEmptyEvent();
}

void PlatformGLFW::UpdatePixelDensity() {
    float xscale;
    float yscale;
    glfwGetWindowContentScale(m_Window, &xscale, &yscale);
    float scale = xscale > yscale ? xscale : yscale;

    float windowScale      = scale;
    float framebufferScale = scale;

    SetWindowScale(
        windowScale);  // this is how windows is scaled, not window content

    SetFramebufferScale(framebufferScale);
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
