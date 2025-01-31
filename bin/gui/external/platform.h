#pragma once
#include <memory>

struct Application;
struct Renderer;

struct Platform {
    virtual ~Platform() = default;

    virtual auto ApplicationStart(int argc, char** argv) -> bool = 0;
    virtual void ApplicationStop()                               = 0;

    virtual auto OpenMainWindow(const char* title,
                                int width,
                                int height) -> bool                 = 0;
    virtual auto CloseMainWindow() -> bool                          = 0;
    [[nodiscard]] virtual auto GetMainWindowHandle() const -> void* = 0;
    virtual void SetMainWindowTitle(const char* title)              = 0;
    virtual void ShowMainWindow()                                   = 0;
    virtual auto ProcessMainWindowEvents() -> bool                  = 0;
    [[nodiscard]] virtual auto IsMainWindowVisible() const -> bool  = 0;

    virtual void SetRenderer(Renderer* renderer) = 0;

    virtual void NewFrame()    = 0;
    virtual void FinishFrame() = 0;

    virtual void Quit() = 0;

    [[nodiscard]] auto HasWindowScaleChanged() const -> bool {
        return m_WindowScaleChanged;
    }
    void AcknowledgeWindowScaleChanged() { m_WindowScaleChanged = false; }
    [[nodiscard]] auto GetWindowScale() const -> float { return m_WindowScale; }
    void SetWindowScale(float windowScale) {
        if (windowScale == m_WindowScale) {
            return;
        }
        m_WindowScale        = windowScale;
        m_WindowScaleChanged = true;
    }

    [[nodiscard]] auto HasFramebufferScaleChanged() const -> bool {
        return m_FramebufferScaleChanged;
    }
    void AcknowledgeFramebufferScaleChanged() {
        m_FramebufferScaleChanged = false;
    }
    [[nodiscard]] auto GetFramebufferScale() const -> float {
        return m_FramebufferScale;
    }
    void SetFramebufferScale(float framebufferScale) {
        if (framebufferScale == m_FramebufferScale) {
            return;
        }
        m_FramebufferScale        = framebufferScale;
        m_FramebufferScaleChanged = true;
    }

   private:
    bool m_WindowScaleChanged      = false;
    float m_WindowScale            = 1.0F;
    bool m_FramebufferScaleChanged = false;
    float m_FramebufferScale       = 1.0F;
};

auto CreatePlatform(Application& application) -> std::unique_ptr<Platform>;

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
