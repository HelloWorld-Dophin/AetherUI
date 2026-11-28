//! 基础几何与颜色类型。
//!
//! 约定：对外的颜色一律是**直通 alpha**（非预乘）的 `float`；
//! 只有落到像素缓冲时才转换成预乘 8 位，避免反复直通/预乘互转的误差。

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace aether {

struct Point {
    float x = 0.0f;
    float y = 0.0f;
};

struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 0.0f;
};

inline constexpr Color color_rgba8(int r, int g, int b, float a = 1.0f) {
    return Color{static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f,
                 static_cast<float>(b) / 255.0f, a};
}

inline constexpr Color color_hex(uint32_t rgb, float a = 1.0f) {
    return color_rgba8(static_cast<int>((rgb >> 16) & 0xFF), static_cast<int>((rgb >> 8) & 0xFF),
                       static_cast<int>(rgb & 0xFF), a);
}

inline constexpr Color with_alpha(Color c, float a) {
    c.a = a;
    return c;
}

inline Color mix(Color from, Color to, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return Color{from.r + (to.r - from.r) * t, from.g + (to.g - from.g) * t,
                 from.b + (to.b - from.b) * t, from.a + (to.a - from.a) * t};
}

inline uint8_t to_unorm8(float v) {
    return static_cast<uint8_t>(std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f));
}

/// 直通 alpha → 预乘 8 位像素，布局 `0xAARRGGBB`
/// （小端内存字节序为 B,G,R,A，即 `SDL_PIXELFORMAT_ARGB8888`）。
inline uint32_t premultiply(Color c) {
    const float a = std::clamp(c.a, 0.0f, 1.0f);
    const uint32_t ai = to_unorm8(a);
    const uint32_t r = to_unorm8(c.r * a);
    const uint32_t g = to_unorm8(c.g * a);
    const uint32_t b = to_unorm8(c.b * a);
    return (ai << 24) | (r << 16) | (g << 8) | b;
}

struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    float right() const { return x + w; }
    float bottom() const { return y + h; }

    bool contains(float px, float py) const {
        return px >= x && py >= y && px < x + w && py < y + h;
    }

    Rect inset(float d) const { return Rect{x + d, y + d, w - d * 2.0f, h - d * 2.0f}; }
};

struct Recti {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;

    int right() const { return x + w; }
    int bottom() const { return y + h; }
    bool empty() const { return w <= 0 || h <= 0; }
};

inline Recti intersect(const Recti& a, const Recti& b) {
    const int x0 = std::max(a.x, b.x);
    const int y0 = std::max(a.y, b.y);
    const int x1 = std::min(a.right(), b.right());
    const int y1 = std::min(a.bottom(), b.bottom());
    if (x1 <= x0 || y1 <= y0) {
        return Recti{};
    }
    return Recti{x0, y0, x1 - x0, y1 - y0};
}

/// 把浮点矩形向外扩张到整数像素，保证抗锯齿边缘不被裁掉。
inline Recti cover(const Rect& r) {
    const int x0 = static_cast<int>(std::floor(r.x));
    const int y0 = static_cast<int>(std::floor(r.y));
    const int x1 = static_cast<int>(std::ceil(r.right()));
    const int y1 = static_cast<int>(std::ceil(r.bottom()));
    return Recti{x0, y0, x1 - x0, y1 - y0};
}

}  // namespace aether
