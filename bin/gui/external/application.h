#pragma once
#include <imgui.h>
#include <memory>
#include <string>

struct Platform;
struct Renderer;

struct Application {
    explicit Application(const char* name);
    Application(const char* name, int argc, char** argv);
    ~Application();

    auto Create(int argc, char** argv, int width = -1, int height = -1) -> bool;

    auto Run() -> int;

    void SetTitle(const char* title);

    auto Close() -> bool;
    void Quit();

    [[nodiscard]] auto GetName() const -> const std::string&;

    [[nodiscard]] auto DefaultFont() const -> ImFont*;
    [[nodiscard]] auto HeaderFont() const -> ImFont*;

    auto LoadTexture(const char* path) -> ImTextureID;
    auto CreateTexture(const void* data, int width, int height) -> ImTextureID;
    void DestroyTexture(ImTextureID texture);
    auto GetTextureWidth(ImTextureID texture) -> int;
    auto GetTextureHeight(ImTextureID texture) -> int;

    virtual void OnStart(int argc, char** argv) {}
    virtual void OnStop() {}
    virtual void OnFrame(float deltaTime) {}

    [[nodiscard]] virtual auto GetWindowFlags() const -> ImGuiWindowFlags;

    virtual auto CanClose() -> bool { return true; }

   private:
    void RecreateFontAtlas(float scale);

    void Frame();

    std::string m_Name;
    std::string m_IniFilename;
    std::unique_ptr<Platform> m_Platform;
    std::unique_ptr<Renderer> m_Renderer;
    ImGuiContext* m_Context = nullptr;
    ImFont* m_DefaultFont   = nullptr;
    ImFont* m_HeaderFont    = nullptr;
};

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
