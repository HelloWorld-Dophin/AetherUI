//! 字体与文本：FreeType 字形栅格化 + 单通道图集缓存 + 中英混排断行。
//!
//! 刻意不做复杂整形（不引入 HarfBuzz）：界面文案以「字宽累加」为主，
//! 中文按字断行、西文按空格断行，已足够覆盖 Fluent 风格界面的需求。

#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "gfx/geometry.h"

typedef struct FT_LibraryRec_* FT_Library;
typedef struct FT_FaceRec_* FT_Face;

namespace aether {

class Painter;

/// 已栅格化并进入图集的字形。
struct Glyph {
    int gx = 0;
    int gy = 0;
    int w = 0;
    int h = 0;
    int left = 0;
    int top = 0;
    float advance = 0.0f;
    bool valid = false;
};

struct TextFormat {
    /// 主字体在字体栈中的下标。
    int face = 0;
    /// 字号（像素）。
    float size = 14.0f;
    /// 行高（像素），来自排版 Token。
    float line_height = 20.0f;
    /// 换行宽度，0 表示不换行。
    float max_width = 0.0f;
    /// 0 左对齐，1 居中。
    int align = 0;
};

struct PlacedGlyph {
    int atlas_x = 0;
    int atlas_y = 0;
    int w = 0;
    int h = 0;
    float x = 0.0f;
    float y = 0.0f;
};

struct TextLayout {
    std::vector<PlacedGlyph> glyphs;
    float width = 0.0f;
    float height = 0.0f;
};

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    bool init();
    /// 按顺序加入字体栈；下标越小优先级越高（回退顺序）。
    int add_face(const char* path);
    int face_count() const { return static_cast<int>(faces_.size()); }

    /// 字体基线到顶部的距离。
    float ascent(int face, float size);

    /// 单行文本宽度。
    float measure(const char* utf8, int face, float size);

    TextLayout layout(const char* utf8, const TextFormat& fmt);
    void draw(Painter& painter, const TextLayout& layout, float x, float y, Color color);

private:
    struct GlyphKey {
        int face = 0;
        int px = 0;
        uint32_t index = 0;

        bool operator==(const GlyphKey& other) const {
            return face == other.face && px == other.px && index == other.index;
        }
    };

    struct GlyphKeyHash {
        size_t operator()(const GlyphKey& key) const {
            size_t h = key.index * 0x9E3779B97F4A7C15ull;
            h ^= static_cast<size_t>(key.face) * 0xC2B2AE3D27D4EB4Full;
            h ^= static_cast<size_t>(key.px) * 0x165667B19E3779F9ull;
            return h ^ (h >> 29);
        }
    };

    const Glyph& glyph(int face, float size, uint32_t cp);
    int pick_face(uint32_t cp) const;
    bool load_glyph(int face, int px, uint32_t cp, Glyph& out);
    bool atlas_alloc(int w, int h, int& x, int& y);

    FT_Library library_ = nullptr;
    std::vector<FT_Face> faces_;
    std::vector<uint8_t> atlas_;
    int shelf_x_ = 1;
    int shelf_y_ = 1;
    int shelf_h_ = 0;
    std::unordered_map<GlyphKey, Glyph, GlyphKeyHash> cache_;
    Glyph missing_;
};

}  // namespace aether
