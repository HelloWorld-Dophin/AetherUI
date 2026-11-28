#include "ui/app_icon.h"

#include <algorithm>
#include <cctype>

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
    // 与 packaging/aetherui.svg 同一构图：圆角方块 + 白色星芒。
    painter.fill_round_rect(Rect{x, y, px, px}, px * 0.22f, color);
    if (builtin_ < 0) {
        return;
    }
    const float glyph = px * 0.62f;
    const float inset = (px - glyph) * 0.5f;
    icons::draw(painter, builtin_, x + inset, y + inset, glyph,
                Color{1.0f, 1.0f, 1.0f, 1.0f}, 1.0f, icons::variant_for(dark));
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
