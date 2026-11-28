#include "ui/title_bar.h"

#include <algorithm>

#include "ui/icons.h"

namespace aether {
namespace {

constexpr float kHeight = 40.0f;
constexpr float kCaptionWidth = 46.0f;
const char* kIconMinimize = "SUBTRACT";
const char* kIconMaximize = "SQUARE";
const char* kIconRestore = "SQUARE_MULTIPLE";
const char* kIconClose = "DISMISS";
const char* kIconSparkle = "SPARKLE";

/// 标题栏图标按名称解析一次，避免每帧线性查找。
struct Glyphs {
    int minimize = -1;
    int maximize = -1;
    int restore = -1;
    int close = -1;
    int sparkle = -1;

    Glyphs() {
        minimize = icons::index_of(kIconMinimize);
        maximize = icons::index_of(kIconMaximize);
        restore = icons::index_of(kIconRestore);
        close = icons::index_of(kIconClose);
        sparkle = icons::index_of(kIconSparkle);
    }
};

const Glyphs& glyphs() {
    static Glyphs table;
    return table;
}

void draw_caption_icon(Painter& painter, int icon, const Rect& area, Color color, float s,
                       bool dark) {
    const float size = 10.0f * s;
    icons::draw(painter, icon, area.x + (area.w - size) * 0.5f, area.y + (area.h - size) * 0.5f,
                size, color, s, icons::variant_for(dark));
}

}  // namespace

void TitleBar::layout(float x, float y, float w, float s) {
    bar = Rect{x, y, w, kHeight * s};
    btn_close = Rect{x + w - kCaptionWidth * s, y, kCaptionWidth * s, kHeight * s};
    btn_maximize = Rect{btn_close.x - kCaptionWidth * s, y, kCaptionWidth * s, kHeight * s};
    btn_minimize = Rect{btn_maximize.x - kCaptionWidth * s, y, kCaptionWidth * s, kHeight * s};
}

void TitleBar::draw(const Context& ctx) const {
    const Theme& theme = ctx.theme;
    const float s = ctx.s;
    Painter& painter = ctx.painter;

    const float radius = maximized ? 0.0f : corner_radius * s;
    painter.fill_round_rect(bar, radius, theme.solid_background_base);
    if (radius > 0.0f) {
        painter.fill_rect(Rect{bar.x, bar.y + radius, bar.w, bar.h - radius},
                          theme.solid_background_base);
    }

    // 标题：强调色 Flash 图标 + 12pt 次级文字。
    const float icon_size = 16.0f * s;
    const float caption_line = theme.caption.line_height * s;
    const float text_x = bar.x + 12.0f * s + icon_size + 8.0f * s;
    icons::draw(painter, glyphs().sparkle, bar.x + 12.0f * s, bar.y + (bar.h - icon_size) * 0.5f,
                icon_size, theme.accent_fill_default, s, icons::variant_for(ctx.dark));
    draw_text(painter, ctx.text, title.c_str(), kFaceRegular, theme.caption.size * s, caption_line,
              text_x, bar.y + (bar.h - caption_line) * 0.5f, theme.text_secondary);

    // 三个窗口按钮。
    if (hover_minimize) {
        painter.fill_rect(btn_minimize, theme.caption_hover);
    }
    if (hover_maximize) {
        painter.fill_rect(btn_maximize, theme.caption_hover);
    }
    if (hover_close) {
        painter.fill_rect(btn_close, theme.close_hover);
    }

    draw_caption_icon(painter, glyphs().minimize, btn_minimize, theme.text_secondary, s, ctx.dark);
    draw_caption_icon(painter, maximized ? glyphs().restore : glyphs().maximize, btn_maximize,
                      theme.text_secondary, s, ctx.dark);
    draw_caption_icon(painter, glyphs().close, btn_close,
                      hover_close ? color_hex(0xFFFFFF) : theme.text_secondary, s, ctx.dark);
}

}  // namespace aether
