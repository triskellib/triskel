#include "renderer.h"

#include <GL/glew.h>
#include <imgui_impl_opengl3.h>
#include <algorithm>

#include "platform.h"

using std::find_if;

struct ImTexture {
    GLuint TextureID = 0;
    int Width        = 0;
    int Height       = 0;
};

struct RendererOpenGL3 final : Renderer {
    auto Create(Platform& platform) -> bool override;
    void Destroy() override;
    void NewFrame() override;
    void RenderDrawData(ImDrawData* drawData) override;
    void Clear(const ImVec4& color) override;
    void Present() override;
    void Resize(int width, int height) override;

    auto FindTexture(ImTextureID texture) -> ImVector<ImTexture>::iterator;
    auto CreateTexture(const void* data,
                       int width,
                       int height) -> ImTextureID override;
    void DestroyTexture(ImTextureID texture) override;
    auto GetTextureWidth(ImTextureID texture) -> int override;
    auto GetTextureHeight(ImTextureID texture) -> int override;

    Platform* m_Platform = nullptr;
    ImVector<ImTexture> m_Textures;
};

auto CreateRenderer() -> std::unique_ptr<Renderer> {
    return std::make_unique<RendererOpenGL3>();
}

auto RendererOpenGL3::Create(Platform& platform) -> bool {
    m_Platform = &platform;

    // Technically we should initialize OpenGL context here,
    // but for now we relay on one created by GLFW3

    bool err = glewInit() != GLEW_OK;

    if (err) {
        return false;
    }

    const char* glslVersion = "#version 130";

    if (!ImGui_ImplOpenGL3_Init(glslVersion)) {
        return false;
    }

    m_Platform->SetRenderer(this);

    return true;
}

void RendererOpenGL3::Destroy() {
    if (m_Platform == nullptr) {
        return;
    }

    m_Platform->SetRenderer(nullptr);

    ImGui_ImplOpenGL3_Shutdown();
}

void RendererOpenGL3::NewFrame() {
    ImGui_ImplOpenGL3_NewFrame();
}

void RendererOpenGL3::RenderDrawData(ImDrawData* drawData) {
    ImGui_ImplOpenGL3_RenderDrawData(drawData);
}

void RendererOpenGL3::Clear(const ImVec4& color) {
    glClearColor(color.x, color.y, color.z, color.w);
    glClear(GL_COLOR_BUFFER_BIT);
}

void RendererOpenGL3::Present() {}

void RendererOpenGL3::Resize(int width, int height) {
    glViewport(0, 0, width, height);
}

auto RendererOpenGL3::CreateTexture(const void* data,
                                    int width,
                                    int height) -> ImTextureID {
    m_Textures.resize(m_Textures.size() + 1);
    ImTexture& texture = m_Textures.back();

    // Upload texture to graphics system
    GLint last_texture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGenTextures(1, &texture.TextureID);
    glBindTexture(GL_TEXTURE_2D, texture.TextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, last_texture);

    texture.Width  = width;
    texture.Height = height;

    return texture.TextureID;
}

auto RendererOpenGL3::FindTexture(ImTextureID textureID)
    -> ImVector<ImTexture>::iterator {
    return find_if(m_Textures.begin(), m_Textures.end(),
                   [textureID](ImTexture& texture) {
                       return texture.TextureID == textureID;
                   });
}

void RendererOpenGL3::DestroyTexture(ImTextureID texture) {
    auto* textureIt = FindTexture(texture);
    if (textureIt == m_Textures.end()) {
        return;
    }

    glDeleteTextures(1, &textureIt->TextureID);

    m_Textures.erase(textureIt);
}

auto RendererOpenGL3::GetTextureWidth(ImTextureID texture) -> int {
    auto* textureIt = FindTexture(texture);
    if (textureIt != m_Textures.end()) {
        return textureIt->Width;
    }
    return 0;
}

auto RendererOpenGL3::GetTextureHeight(ImTextureID texture) -> int {
    auto* textureIt = FindTexture(texture);
    if (textureIt != m_Textures.end()) {
        return textureIt->Height;
    }
    return 0;
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
