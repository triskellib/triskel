#pragma once

#include <memory>

struct Platform;
struct ImDrawData;
struct ImVec4;
using ImTextureID = unsigned long long;

struct Renderer {
    virtual ~Renderer(){};

    virtual auto Create(Platform& platform) -> bool = 0;
    virtual void Destroy()                          = 0;

    virtual void NewFrame() = 0;

    virtual void RenderDrawData(ImDrawData* drawData) = 0;

    virtual void Clear(const ImVec4& color) = 0;
    virtual void Present()                  = 0;

    virtual void Resize(int width, int height) = 0;

    virtual auto CreateTexture(const void* data,
                               int width,
                               int height) -> ImTextureID     = 0;
    virtual void DestroyTexture(ImTextureID texture)          = 0;
    virtual auto GetTextureWidth(ImTextureID texture) -> int  = 0;
    virtual auto GetTextureHeight(ImTextureID texture) -> int = 0;
};

auto CreateRenderer() -> std::unique_ptr<Renderer>;

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
