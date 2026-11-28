#include "ui/app_icon.h"

#include <algorithm>
#include <cctype>
#include <cmath>

#include "gfx/surface.h"
#include "ui/icons.h"

namespace aether {
namespace {

bool ends_with_svg(const std::string& value) {
    if (value.size() < 4) {
        return false;
    }
    std::string tail = value.substr(value.size() - 4);
    for (char& c : tail) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return tail == ".svg";
}

/// 品牌图标的内置名（不区分大小写）。
bool is_brand_name(const std::string& value) {
    std::string upper;
    upper.reserve(value.size());
    for (char c : value) {
        upper += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return upper == "AETHERUI" || upper == "AETHER" || upper == "BRAND";
}

/// packaging/aetherui.svg 的竖向渐变：#57C8FF → #1E7FD6 → #0B57B8。
Color brand_gradient(float t) {
    const Color stops[3] = {
        Color{0.341f, 0.784f, 1.000f, 1.0f},
        Color{0.118f, 0.498f, 0.839f, 1.0f},
        Color{0.043f, 0.341f, 0.722f, 1.0f},
    };
    const float positions[3] = {0.0f, 0.55f, 1.0f};
    t = std::clamp(t, 0.0f, 1.0f);
    for (int i = 0; i < 2; ++i) {
        if (t <= positions[i + 1]) {
            const float span = positions[i + 1] - positions[i];
            const float k = span <= 0.0f ? 0.0f : (t - positions[i]) / span;
            return Color{stops[i].r + (stops[i + 1].r - stops[i].r) * k,
                         stops[i].g + (stops[i + 1].g - stops[i].g) * k,
                         stops[i].b + (stops[i + 1].b - stops[i].b) * k, 1.0f};
        }
    }
    return stops[2];
}

}  // namespace

AppIcon::AppIcon(const std::string& source) : source_(source) { load(); }

bool AppIcon::load() {
    builtin_ = -1;
    brand_ = false;
    svg_.clear();
    if (source_.empty()) {
        return false;
    }
    if (is_brand_name(source_)) {
        brand_ = true;
        builtin_ = icons::index_of("SPARKLE");
        return true;
    }
    if (ends_with_svg(source_)) {
        return svg::Image::load(source_.c_str(), svg_);
    }
    builtin_ = icons::index_of(source_.c_str());
    return builtin_ >= 0;
}

void AppIcon::paint_brand(Painter& painter, float x, float y, float px, Color color,
                          bool dark) const {
    (void)color;  // 品牌色固定，不跟主题走
    (void)dark;
    if (px <= 0.0f) {
        return;
    }
    // 与 packaging/aetherui.svg 同一构图：渐变圆角方块 + 白色星芒。
    // Painter 没有渐变填充，用横向色带逐条裁剪模拟竖向渐变。
    const Rect tile{x, y, px, px};
    const float radius = px * 0.22f;
    const Recti saved = painter.clip();
    const int bands = std::max(6, static_cast<int>(px * 0.5f));
    const float band_h = px / static_cast<float>(bands);
    for (int i = 0; i < bands; ++i) {
        const float top = y + static_cast<float>(i) * band_h;
        const Recti band{static_cast<int>(std::floor(x)), static_cast<int>(std::floor(top)),
                         static_cast<int>(std::ceil(px)),
                         static_cast<int>(std::ceil(band_h)) + 1};
        painter.set_clip(intersect(saved, band));
        painter.fill_round_rect(
            tile, radius,
            brand_gradient((static_cast<float>(i) + 0.5f) / static_cast<float>(bands)));
    }
    painter.set_clip(saved);
    if (builtin_ < 0) {
        return;
    }
    const float glyph = px * 0.62f;
    const float inset = (px - glyph) * 0.5f;
    icons::draw(painter, builtin_, x + inset, y + inset, glyph,
                Color{1.0f, 1.0f, 1.0f, 1.0f}, 1.0f, icons::Variant::Regular);
}

void AppIcon::draw(Painter& painter, float x, float y, float size, float scale, Color color,
                   bool dark) const {
    if (size <= 0.0f || color.a <= 0.0f) {
        return;
    }
    if (brand_) {
        paint_brand(painter, x, y, size * scale, color, dark);
        return;
    }
    if (builtin_ >= 0) {
        icons::draw(painter, builtin_, x, y, size, color, scale, icons::variant_for(dark));
        return;
    }
    if (!svg_.valid()) {
        return;
    }
    Path path;
    svg_.build(path, x, y, size * scale);
    painter.fill_path(path, color);
}

bool AppIcon::rasterize(int px, Color color, std::vector<uint32_t>& out) const {
    if (px <= 0 || !valid()) {
        return false;
    }
    const float size = static_cast<float>(px);
    std::vector<uint32_t> pixels(static_cast<size_t>(px) * px, 0);
    Surface surface(pixels.data(), px, px, px);
    Painter painter(surface);

    if (brand_) {
        paint_brand(painter, 0.0f, 0.0f, size, color, true);
    } else if (builtin_ >= 0) {
        icons::draw(painter, builtin_, 0.0f, 0.0f, size, color, 1.0f, icons::Variant::Regular);
    } else {
        Path path;
        svg_.build(path, 0.0f, 0.0f, size);
        painter.fill_path(path, color);
    }

    // 表面是预乘 alpha，SDL 的 ARGB8888 需要直通 alpha，逐像素还原。
    out.resize(pixels.size());
    for (size_t i = 0; i < pixels.size(); ++i) {
        const uint32_t pixel = pixels[i];
        const uint32_t alpha = (pixel >> 24) & 0xFFu;
        if (alpha == 0) {
            out[i] = 0;
            continue;
        }
        const uint32_t r = std::min(255u, ((pixel >> 16) & 0xFFu) * 255u / alpha);
        const uint32_t g = std::min(255u, ((pixel >> 8) & 0xFFu) * 255u / alpha);
        const uint32_t b = std::min(255u, (pixel & 0xFFu) * 255u / alpha);
        out[i] = (alpha << 24) | (r << 16) | (g << 8) | b;
    }
    return true;
}

}  // namespace aether
