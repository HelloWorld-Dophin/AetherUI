#include "text/font.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "gfx/painter.h"

namespace aether {
namespace {

constexpr int kAtlasSize = 1024;
constexpr int kAtlasPad = 1;

bool is_cjk(uint32_t cp) {
    return (cp >= 0x2E80 && cp <= 0x9FFF) || (cp >= 0xF900 && cp <= 0xFAFF) ||
           (cp >= 0xFF00 && cp <= 0xFF60) || (cp >= 0x3000 && cp <= 0x303F);
}

/// 不允许出现在行首的标点（避免「。」「，」被顶到下一行开头）。
bool no_break_before(uint32_t cp) {
    switch (cp) {
        case U'，':
        case U'。':
        case U'、':
        case U'；':
        case U'：':
        case U'？':
        case U'！':
        case U'）':
        case U'】':
        case U'」':
        case U'』':
        case U'》':
        case U'〉':
        case U'”':
        case U'’':
        case U'…':
        case U'·':
            return true;
        default:
            return false;
    }
}

uint32_t next_codepoint(const char*& p) {
    const auto* s = reinterpret_cast<const unsigned char*>(p);
    auto continuation = [](unsigned char c) { return (c & 0xC0) == 0x80; };

    if (s[0] < 0x80) {
        p += 1;
        return s[0];
    }
    if ((s[0] & 0xE0) == 0xC0 && continuation(s[1])) {
        const uint32_t cp = ((s[0] & 0x1Fu) << 6) | (s[1] & 0x3Fu);
        p += 2;
        return cp;
    }
    if ((s[0] & 0xF0) == 0xE0 && continuation(s[1]) && continuation(s[2])) {
        const uint32_t cp = ((s[0] & 0x0Fu) << 12) | ((s[1] & 0x3Fu) << 6) | (s[2] & 0x3Fu);
        p += 3;
        return cp;
    }
    if ((s[0] & 0xF8) == 0xF0 && continuation(s[1]) && continuation(s[2]) && continuation(s[3])) {
        const uint32_t cp = ((s[0] & 0x07u) << 18) | ((s[1] & 0x3Fu) << 12) |
                            ((s[2] & 0x3Fu) << 6) | (s[3] & 0x3Fu);
        p += 4;
        return cp;
    }
    p += 1;
    return U'?';
}

struct Item {
    uint32_t cp = 0;
    const Glyph* glyph = nullptr;
    float advance = 0.0f;
    bool newline = false;
    bool space = false;
};

}  // namespace

TextRenderer::TextRenderer() : atlas_(static_cast<size_t>(kAtlasSize) * kAtlasSize, 0) {}

TextRenderer::~TextRenderer() {
    for (FT_Face face : faces_) {
        FT_Done_Face(face);
    }
    if (library_ != nullptr) {
        FT_Done_FreeType(library_);
    }
}

bool TextRenderer::init() {
    if (library_ != nullptr) {
        return true;
    }
    return FT_Init_FreeType(&library_) == 0;
}

int TextRenderer::add_face(const char* path) {
    if (library_ == nullptr || path == nullptr) {
        return -1;
    }
    FT_Face face = nullptr;
    if (FT_New_Face(library_, path, 0, &face) != 0) {
        return -1;
    }
    FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    faces_.push_back(face);
    return static_cast<int>(faces_.size()) - 1;
}

int TextRenderer::pick_face(uint32_t cp) const {
    for (size_t i = 0; i < faces_.size(); ++i) {
        if (FT_Get_Char_Index(faces_[i], cp) != 0) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool TextRenderer::atlas_alloc(int w, int h, int& x, int& y) {
    if (w <= 0 || h <= 0 || w + kAtlasPad * 2 > kAtlasSize || h + kAtlasPad * 2 > kAtlasSize) {
        return false;
    }
    if (shelf_x_ + w + kAtlasPad > kAtlasSize) {
        shelf_x_ = kAtlasPad;
        shelf_y_ += shelf_h_ + kAtlasPad;
        shelf_h_ = 0;
    }
    if (shelf_y_ + h + kAtlasPad > kAtlasSize) {
        return false;
    }
    x = shelf_x_;
    y = shelf_y_;
    shelf_x_ += w + kAtlasPad;
    shelf_h_ = std::max(shelf_h_, h);
    return true;
}

bool TextRenderer::load_glyph(int face_index, int px, uint32_t cp, Glyph& out) {
    FT_Face face = faces_[static_cast<size_t>(face_index)];
    if (FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(px)) != 0) {
        return false;
    }
    const FT_UInt index = FT_Get_Char_Index(face, cp);
    if (index == 0) {
        return false;
    }
    if (FT_Load_Glyph(face, index, FT_LOAD_RENDER) != 0) {
        return false;
    }

    const FT_GlyphSlot slot = face->glyph;
    out.valid = true;
    out.advance = static_cast<float>(slot->advance.x) / 64.0f;
    out.left = slot->bitmap_left;
    out.top = slot->bitmap_top;
    out.w = static_cast<int>(slot->bitmap.width);
    out.h = static_cast<int>(slot->bitmap.rows);

    if (out.w <= 0 || out.h <= 0) {
        return true;  // 空格等无位图字形，只需要 advance
    }

    int ax = 0;
    int ay = 0;
    if (!atlas_alloc(out.w, out.h, ax, ay)) {
        out.valid = false;
        return false;
    }
    out.gx = ax;
    out.gy = ay;

    const int pitch = std::abs(slot->bitmap.pitch);
    const uint8_t* src = slot->bitmap.buffer;
    for (int row = 0; row < out.h; ++row) {
        std::memcpy(&atlas_[static_cast<size_t>(ay + row) * kAtlasSize + ax], src + static_cast<size_t>(row) * pitch,
                    static_cast<size_t>(out.w));
    }
    return true;
}

const Glyph& TextRenderer::glyph(int face_index, float size, uint32_t cp) {
    const int px = std::max(1, static_cast<int>(std::lround(size)));
    const GlyphKey key{face_index, px, cp};
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        return it->second;
    }

    Glyph loaded;
    if (!load_glyph(face_index, px, cp, loaded)) {
        loaded = missing_;
        loaded.valid = false;
        loaded.advance = size * 0.5f;
    }
    auto [inserted, _] = cache_.emplace(key, loaded);
    return inserted->second;
}

float TextRenderer::ascent(int face_index, float size) {
    if (faces_.empty()) {
        return size * 0.8f;
    }
    FT_Face face = faces_[static_cast<size_t>(std::clamp(face_index, 0, face_count() - 1))];
    FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(std::max(1, static_cast<int>(std::lround(size)))));
    return static_cast<float>(face->size->metrics.ascender) / 64.0f;
}

float TextRenderer::measure(const char* utf8, int face_index, float size) {
    if (utf8 == nullptr || faces_.empty()) {
        return 0.0f;
    }
    float width = 0.0f;
    const char* p = utf8;
    while (*p != '\0') {
        const uint32_t cp = next_codepoint(p);
        if (cp == U'\n') {
            break;
        }
        const int picked = pick_face(cp);
        if (picked < 0) {
            continue;
        }
        width += glyph(picked, size, cp).advance;
    }
    return width;
}

TextLayout TextRenderer::layout(const char* utf8, const TextFormat& fmt) {
    TextLayout result;
    if (utf8 == nullptr || faces_.empty()) {
        return result;
    }

    std::vector<Item> items;
    items.reserve(std::strlen(utf8));
    {
        const char* p = utf8;
        while (*p != '\0') {
            const uint32_t cp = next_codepoint(p);
            Item item;
            item.cp = cp;
            if (cp == U'\n') {
                item.newline = true;
                items.push_back(item);
                continue;
            }
            const int picked = pick_face(cp);
            if (picked >= 0) {
                item.glyph = &glyph(picked, fmt.size, cp);
                item.advance = item.glyph->advance;
            } else {
                item.advance = fmt.size * 0.5f;
            }
            item.space = (cp == U' ');
            items.push_back(item);
        }
    }

    const size_t n = items.size();
    if (n == 0) {
        return result;
    }

    // 前缀和：任意行宽 = prefix[end] - prefix[start]。
    std::vector<float> prefix(n + 1, 0.0f);
    for (size_t i = 0; i < n; ++i) {
        prefix[i + 1] = prefix[i] + items[i].advance;
    }

    std::vector<std::pair<size_t, size_t>> lines;
    const float max_width = fmt.max_width;
    size_t start = 0;
    size_t i = 0;
    while (i < n) {
        if (items[i].newline) {
            lines.emplace_back(start, i);
            start = i + 1;
            ++i;
            continue;
        }
        if (max_width > 0.0f && prefix[i + 1] - prefix[start] > max_width && i > start) {
            size_t break_at = i;
            for (size_t k = i; k > start; --k) {
                const bool after_space = items[k - 1].space;
                const bool before_cjk = is_cjk(items[k].cp) && !no_break_before(items[k].cp);
                if (after_space || before_cjk) {
                    break_at = k;
                    break;
                }
            }
            lines.emplace_back(start, break_at);
            start = break_at;
            continue;  // 换行后重新判断，可能一行都放不下
        }
        ++i;
    }
    if (start < n) {
        lines.emplace_back(start, n);
    }
    if (lines.empty()) {
        return result;
    }

    const float line_ascent = ascent(fmt.face, fmt.size);
    const float line_height = fmt.line_height > 0.0f ? fmt.line_height : fmt.size * 1.4f;
    result.height = static_cast<float>(lines.size()) * line_height;

    float widest = 0.0f;
    for (const auto& [begin, end] : lines) {
        float line_width = prefix[end] - prefix[begin];
        // 行尾空格不占布局宽度（从尾部逐个剔除，注意不能用常量 end 做循环变量）。
        size_t trimmed = end;
        while (trimmed > begin && items[trimmed - 1].space) {
            line_width -= items[trimmed - 1].advance;
            --trimmed;
        }
        widest = std::max(widest, line_width);
    }
    result.width = widest;

    for (size_t line_index = 0; line_index < lines.size(); ++line_index) {
        const auto [begin, end] = lines[line_index];
        const float line_width = prefix[end] - prefix[begin];
        const float base_x =
            fmt.align == 1 ? (std::max(max_width, widest) - line_width) * 0.5f : 0.0f;
        const float baseline = static_cast<float>(line_index) * line_height + line_ascent;

        float pen = base_x;
        for (size_t k = begin; k < end; ++k) {
            const Item& item = items[k];
            if (item.glyph != nullptr && item.glyph->valid && item.glyph->w > 0 && item.glyph->h > 0) {
                PlacedGlyph placed;
                placed.atlas_x = item.glyph->gx;
                placed.atlas_y = item.glyph->gy;
                placed.w = item.glyph->w;
                placed.h = item.glyph->h;
                placed.x = std::round(pen + static_cast<float>(item.glyph->left));
                placed.y = std::round(baseline - static_cast<float>(item.glyph->top));
                result.glyphs.push_back(placed);
            }
            pen += item.advance;
        }
    }

    return result;
}

void TextRenderer::draw(Painter& painter, const TextLayout& layout, float x, float y, Color color) {
    if (layout.glyphs.empty() || atlas_.empty()) {
        return;
    }
    for (const PlacedGlyph& g : layout.glyphs) {
        const uint8_t* src = atlas_.data() + static_cast<size_t>(g.atlas_y) * kAtlasSize + g.atlas_x;
        painter.blit_alpha(static_cast<int>(std::lround(x + g.x)), static_cast<int>(std::lround(y + g.y)),
                           src, g.w, g.h, kAtlasSize, color);
    }
}

}  // namespace aether
