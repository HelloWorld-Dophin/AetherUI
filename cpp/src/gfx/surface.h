//! 预乘 RGBA8 像素缓冲的**非拥有**视图。
//!
//! 之所以不做拥有型，是因为窗口帧缓冲、离屏背景缓存都可以直接借出去画，
//! 避免每帧在 SDL 缓冲与自有缓冲之间拷贝。

#pragma once

#include <cstdint>

#include "gfx/geometry.h"

namespace aether {

class Surface {
public:
    Surface() = default;
    Surface(uint32_t* pixels, int width, int height, int stride)
        : pixels_(pixels), width_(width), height_(height), stride_(stride) {}

    bool valid() const { return pixels_ != nullptr && width_ > 0 && height_ > 0; }
    int width() const { return width_; }
    int height() const { return height_; }
    /// 行跨度，单位是像素而不是字节。
    int stride() const { return stride_; }
    uint32_t* pixels() const { return pixels_; }
    uint32_t* row(int y) const { return pixels_ + static_cast<size_t>(y) * stride_; }
    Recti bounds() const { return Recti{0, 0, width_, height_}; }

    /// 用不透明颜色整屏填充（不参与混合，直接写像素）。
    void fill(Color c);

private:
    uint32_t* pixels_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    int stride_ = 0;
};

}  // namespace aether
