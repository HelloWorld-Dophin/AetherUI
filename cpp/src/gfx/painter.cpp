#include "gfx/painter.h"

#include <algorithm>
#include <cmath>

namespace aether {
namespace {

/// src 为预乘 8 位，cov 为 0..1 覆盖率；dst 也是预乘 8 位。
inline void blend(uint32_t& dst, uint32_t src, float cov) {
    if (cov <= 0.0f) {
        return;
    }
    const uint32_t sa8 = (src >> 24) & 0xFF;
    if (cov >= 0.999f && sa8 == 255) {
        dst = src;
        return;
    }

    // 统一在 0..1 归一化域里做预乘合成，避免 0..255 与 0..1 混用导致偏暗。
    const float sa = (static_cast<float>(sa8) / 255.0f) * cov;
    const float sr = (static_cast<float>((src >> 16) & 0xFF) / 255.0f) * cov;
    const float sg = (static_cast<float>((src >> 8) & 0xFF) / 255.0f) * cov;
    const float sb = (static_cast<float>(src & 0xFF) / 255.0f) * cov;

    const float inv = 1.0f - sa;
    const float dr = static_cast<float>((dst >> 16) & 0xFF) / 255.0f;
    const float dg = static_cast<float>((dst >> 8) & 0xFF) / 255.0f;
    const float db = static_cast<float>(dst & 0xFF) / 255.0f;
    const float da = static_cast<float>((dst >> 24) & 0xFF) / 255.0f;

    dst = (to_unorm8(sa + da * inv) << 24) | (to_unorm8(sr + dr * inv) << 16) |
          (to_unorm8(sg + dg * inv) << 8) | to_unorm8(sb + db * inv);
}

}  // namespace

Painter::Painter(Surface& target) : target_(target), clip_(target.bounds()) {}

void Painter::set_clip(Recti clip) { clip_ = intersect(clip, target_.bounds()); }

void Painter::fill_rect(Rect r, Color c) {
    const Recti area = intersect(cover(r), clip_);
    if (area.empty()) {
        return;
    }
    const uint32_t src = premultiply(c);
    for (int y = area.y; y < area.bottom(); ++y) {
        uint32_t* row = target_.row(y);
        for (int x = area.x; x < area.right(); ++x) {
            blend(row[x], src, 1.0f);
        }
    }
}

void Painter::fill_round_rect(Rect r, float radius, Color c) {
    if (r.w <= 0.0f || r.h <= 0.0f) {
        return;
    }
    path_.clear();
    path_.add_round_rect(r, radius);
    if (rasterizer_.rasterize(path_, clip_, mask_)) {
        fill_mask(mask_, c);
    }
}

void Painter::fill_circle(float cx, float cy, float radius, Color c) {
    if (radius <= 0.0f) {
        return;
    }
    path_.clear();
    path_.add_circle(cx, cy, radius);
    if (rasterizer_.rasterize(path_, clip_, mask_)) {
        fill_mask(mask_, c);
    }
}

void Painter::stroke_round_rect(Rect r, float radius, float width, Color c) {
    if (width <= 0.0f || r.w <= 0.0f || r.h <= 0.0f) {
        return;
    }
    // 外轮廓顺时针 + 内轮廓逆时针，非零环绕规则直接挖出环带。
    path_.clear();
    path_.add_round_rect(r, radius, false);
    path_.add_round_rect(r.inset(width), std::max(radius - width, 0.0f), true);
    if (rasterizer_.rasterize(path_, clip_, mask_)) {
        fill_mask(mask_, c);
    }
}

void Painter::stroke_line(float x0, float y0, float x1, float y1, float width, Color c) {
    if (width <= 0.0f || c.a <= 0.0f) {
        return;
    }
    const float dx = x1 - x0;
    const float dy = y1 - y0;
    const float len = std::sqrt(dx * dx + dy * dy);
    if (len <= 1e-4f) {
        return;
    }
    const float half = width * 0.5f;
    // 法向偏移出矩形，两端各延长半个线宽作为方端帽。
    const float nx = -dy / len * half;
    const float ny = dx / len * half;
    const float ex = dx / len * half;
    const float ey = dy / len * half;

    path_.clear();
    path_.move_to(x0 - ex + nx, y0 - ey + ny);
    path_.line_to(x1 + ex + nx, y1 + ey + ny);
    path_.line_to(x1 + ex - nx, y1 + ey - ny);
    path_.line_to(x0 - ex - nx, y0 - ey - ny);
    if (rasterizer_.rasterize(path_, clip_, mask_)) {
        fill_mask(mask_, c);
    }
}

void Painter::fill_radial_glow(float cx, float cy, float radius, Color c) {
    if (radius <= 0.0f || c.a <= 0.0f) {
        return;
    }
    const Recti area = intersect(cover(Rect{cx - radius, cy - radius, radius * 2.0f, radius * 2.0f}), clip_);
    if (area.empty()) {
        return;
    }
    const uint32_t src = premultiply(c);
    const float inv_r2 = 1.0f / (radius * radius);
    for (int y = area.y; y < area.bottom(); ++y) {
        uint32_t* row = target_.row(y);
        const float dy = static_cast<float>(y) + 0.5f - cy;
        const float dy2 = dy * dy;
        for (int x = area.x; x < area.right(); ++x) {
            const float dx = static_cast<float>(x) + 0.5f - cx;
            const float d2 = (dx * dx + dy2) * inv_r2;
            if (d2 >= 1.0f) {
                continue;
            }
            // 无开方衰减：u = 1 - d²/r²，再平方一次让边缘更柔。
            const float u = 1.0f - d2;
            blend(row[x], src, u * u);
        }
    }
}

void Painter::draw_shadow(Rect r, float radius, Color c, float sigma, float dx, float dy) {
    if (sigma <= 0.0f || c.a <= 0.0f) {
        return;
    }
    const int pad = static_cast<int>(std::ceil(sigma * 3.0f)) + 2;
    const Rect shifted{r.x + dx, r.y + dy, r.w, r.h};
    const Recti region =
        intersect(cover(Rect{shifted.x - pad, shifted.y - pad, shifted.w + pad * 2.0f,
                             shifted.h + pad * 2.0f}),
                  clip_);
    if (region.empty()) {
        return;
    }

    path_.clear();
    path_.add_round_rect(shifted, radius);
    if (!rasterizer_.rasterize(path_, region, mask_)) {
        return;
    }
    blur_mask(mask_, sigma);
    fill_mask(mask_, c);
}

void Painter::fill_mask(const Mask& mask, Color c) {
    if (mask.empty() || c.a <= 0.0f) {
        return;
    }
    const Recti area = intersect(Recti{mask.x0, mask.y0, mask.w, mask.h}, clip_);
    if (area.empty()) {
        return;
    }
    const uint32_t src = premultiply(c);
    for (int y = area.y; y < area.bottom(); ++y) {
        const float* mrow = mask.row(y);
        uint32_t* row = target_.row(y);
        for (int x = area.x; x < area.right(); ++x) {
            blend(row[x], src, std::min(mrow[x - mask.x0], 1.0f));
        }
    }
}

void Painter::fill_path(const Path& path, Color c) {
    if (c.a <= 0.0f || path.empty()) {
        return;
    }
    if (rasterizer_.rasterize(path, clip_, mask_)) {
        fill_mask(mask_, c);
    }
}

void Painter::fill_round_rect_clip(Rect r, float radius, Rect clip_rect, Color c) {
    if (r.w <= 0.0f || r.h <= 0.0f || c.a <= 0.0f) {
        return;
    }
    const Recti region = intersect(cover(clip_rect), clip_);
    if (region.empty()) {
        return;
    }
    path_.clear();
    path_.add_round_rect(r, radius);
    if (rasterizer_.rasterize(path_, region, mask_)) {
        fill_mask(mask_, c);
    }
}

void Painter::blit_alpha(int x, int y, const uint8_t* src, int w, int h, int stride, Color c) {
    if (src == nullptr || w <= 0 || h <= 0 || c.a <= 0.0f) {
        return;
    }
    const Recti area = intersect(Recti{x, y, w, h}, clip_);
    if (area.empty()) {
        return;
    }
    const uint32_t premul = premultiply(c);
    for (int yy = area.y; yy < area.bottom(); ++yy) {
        const uint8_t* srow = src + static_cast<size_t>(yy - y) * stride;
        uint32_t* drow = target_.row(yy);
        for (int xx = area.x; xx < area.right(); ++xx) {
            const uint8_t a = srow[xx - x];
            if (a != 0) {
                blend(drow[xx], premul, static_cast<float>(a) / 255.0f);
            }
        }
    }
}

}  // namespace aether
