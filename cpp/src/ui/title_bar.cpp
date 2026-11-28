#include "ui/title_bar.h"

#include <algorithm>
#include <cmath>

#include "ui/icons.h"

namespace aether {
namespace {

constexpr float kHeight = 40.0f;
constexpr float kCaptionWidth = 52.0f;

enum class Caption { Minimize, Maximize, Restore, Close };

/// 窗口按钮按 Fluent 的 1px 线稿绘制：直接按像素对齐画线，任何 DPI 下都清晰，
/// 不像缩放的矢量图标那样发虚。描边带一点点圆角，避免方角显得生硬。
void draw_caption_glyph(Painter& painter, Caption which, const Rect& area, Color color, float s) {
    const float t = std::max(1.0f, std::round(s));
    const float px = std::round(area.x + area.w * 0.5f);
    const float py = std::round(area.y + area.h * 0.5f);
    const float half = std::round(5.0f * s);
    const float radius = std::max(1.0f, std::round(1.5f * s));
    switch (which) {
        case Caption::Minimize:
            painter.fill_rect(Rect{px - half, py, half * 2.0f, t}, color);
            break;
        case Caption::Maximize:
            painter.stroke_round_rect(Rect{px - half, py - half, half * 2.0f, half * 2.0f},
                                      radius, t, color);
            break;
        case Caption::Restore: {
            // 前框：左下角的完整小方框；后框只画右上两条边（L 形），
            // 两个方框错开而不是重叠，10px 下也不会糊成一团。
            const float gap = std::round(2.0f * s);
            const float box = half * 2.0f - gap;
            painter.fill_rect(Rect{px - half + gap, py - half, box, t}, color);
            painter.fill_rect(Rect{px + half - t, py - half, t, box}, color);
            painter.stroke_round_rect(Rect{px - half, py - half + gap, box, box}, radius, t, color);
            break;
        }
        case Caption::Close:
            painter.stroke_line(px - half, py - half, px + half, py + half, t, color);
            painter.stroke_line(px + half, py - half, px - half, py + half, t, color);
            break;
    }
}

/// 标题前的默认图标（未传图标时回退，沿用 Fluent 图标表，只解析一次）。
int sparkle_icon() {
    static const int index = icons::index_of("SPARKLE");
    return index;
}

}  // namespace

void TitleBar::layout(float x, float y, float w, float s) {
    bar = Rect{x, y, w, kHeight * s};
    btn_close = Rect{x + w - kCaptionWidth * s, y, kCaptionWidth * s, kHeight * s};
    btn_maximize = Rect{btn_close.x - kCaptionWidth * s, y, kCaptionWidth * s, kHeight * s};
    btn_minimize = Rect{btn_maximize.x - kCaptionWidth * s, y, kCaptionWidth * s, kHeight * s};
}

bool TitleBar::handle_event(const Event& event, const UiContext& ui) {
    switch (event.type) {
        case Event::Type::MouseMove:
            hover_minimize = btn_minimize.contains(event.x, event.y);
            hover_maximize = btn_maximize.contains(event.x, event.y);
            hover_close = btn_close.contains(event.x, event.y);
            return false;
        case Event::Type::MouseDown: {
            if (event.key != 1) {
                return false;
            }
            WidgetHost* host = ui.host;
            if (btn_close.contains(event.x, event.y)) {
                if (host != nullptr) {
                    host->request_close();
                }
                return true;
            }
            if (btn_maximize.contains(event.x, event.y)) {
                if (host != nullptr) {
                    host->toggle_maximize();
                    maximized = host->maximized();
                }
                return true;
            }
            if (btn_minimize.contains(event.x, event.y)) {
                if (host != nullptr) {
                    host->minimize();
                }
                return true;
            }
            if (!bar.contains(event.x, event.y)) {
                return false;
            }
            // 双击标题栏切换最大化，否则开始拖动窗口。
            const bool double_click = last_click_ >= 0.0 && ui.now - last_click_ < 0.5 &&
                                      std::abs(event.x - last_click_x_) < 6.0f * ui.s;
            if (double_click) {
                if (host != nullptr) {
                    host->toggle_maximize();
                    maximized = host->maximized();
                }
                last_click_ = -1.0;
            } else {
                if (host != nullptr) {
                    host->begin_drag();
                }
                last_click_ = ui.now;
                last_click_x_ = event.x;
            }
            return true;
        }
        default:
            return false;
    }
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

    // 标题：强调色图标（与窗口图标同源）+ 12pt 次级文字。
    const float icon_size = std::round(16.0f * s);
    const float caption_line = theme.caption.line_height * s;
    const float text_x = bar.x + 12.0f * s + icon_size + 8.0f * s;
    const float icon_y = std::round(bar.y + (bar.h - icon_size) * 0.5f);
    if (icon.valid()) {
        icon.draw(painter, bar.x + 12.0f * s, icon_y, icon_size, 1.0f, theme.accent_fill_default,
                  ctx.dark);
    } else {
        icons::draw(painter, sparkle_icon(), bar.x + 12.0f * s, icon_y, icon_size,
                    theme.accent_fill_default, 1.0f, icons::variant_for(ctx.dark));
    }
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

    // 图标用主文本色，保证深色下发白、浅色下发黑，不会灰蒙蒙。
    draw_caption_glyph(painter, Caption::Minimize, btn_minimize, theme.text_primary, s);
    draw_caption_glyph(painter, maximized ? Caption::Restore : Caption::Maximize, btn_maximize,
                       theme.text_primary, s);
    draw_caption_glyph(painter, Caption::Close, btn_close,
                       hover_close ? color_hex(0xFFFFFF) : theme.text_primary, s);
}

}  // namespace aether
