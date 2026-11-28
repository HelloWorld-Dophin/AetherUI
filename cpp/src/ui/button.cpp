#include "ui/button.h"

#include <algorithm>

#include "ui/icons.h"

namespace aether {
namespace {

constexpr float kIconSize = 16.0f;
constexpr float kIconGap = 8.0f;
constexpr float kMinWidth = 96.0f;
constexpr float kHeight = 32.0f;
constexpr float kPaddingH = 11.0f;

Color fill_color(const Theme& t, const Button& b, ControlState state) {
    if (state == ControlState::Disabled) {
        return t.control_fill_disabled;
    }
    if (b.primary) {
        switch (state) {
            case ControlState::PointerOver:
                return t.accent_fill_secondary;
            case ControlState::Pressed:
                return t.accent_fill_tertiary;
            default:
                return t.accent_fill_default;
        }
    }
    switch (state) {
        case ControlState::PointerOver:
            return t.control_fill_secondary;
        case ControlState::Pressed:
            return t.control_fill_tertiary;
        default:
            return t.control_fill_default;
    }
}

Color text_color(const Theme& t, const Button& b, ControlState state) {
    if (b.disabled) {
        return t.text_disabled;
    }
    if (b.primary) {
        return t.text_on_accent_primary;
    }
    if (state == ControlState::Pressed) {
        return t.text_secondary;
    }
    return t.text_primary;
}

}  // namespace

void Button::measure(TextRenderer& text, const Theme& theme, float s) {
    const float height = kHeight * s;
    const float line_height = theme.body_strong.line_height * s;
    const float label_width =
        label.empty() ? 0.0f : text_width(text, label.c_str(), theme.dark ? kFaceSemiBold : kFaceRegular,
                                          theme.body_strong.size * s);
    const float icon_width = icon >= 0 ? kIconSize * s : 0.0f;
    const float gap = (icon >= 0 && !label.empty()) ? kIconGap * s : 0.0f;

    rect.h = compact ? height : std::max(line_height, height);
    rect.w = compact ? rect.h
                     : std::max(label_width + icon_width + gap + kPaddingH * 2.0f, kMinWidth * s);
}

void Button::place(float x, float y) { rect = Rect{x, y, rect.w, rect.h}; }

void Button::draw(const Context& ctx) const {
    const Theme& theme = ctx.theme;
    const float s = ctx.s;
    const bool dark = ctx.dark;
    Painter& painter = ctx.painter;
    TextRenderer& text = ctx.text;

    ControlState state = ControlState::Normal;
    if (disabled) {
        state = ControlState::Disabled;
    } else if (pressed) {
        state = ControlState::Pressed;
    } else if (hovered) {
        state = ControlState::PointerOver;
    }

    const float radius = theme.radius_control * s;
    painter.fill_round_rect(rect, radius, fill_color(theme, *this, state));
    painter.stroke_round_rect(rect, radius, theme.stroke_thickness * s,
                              disabled ? theme.control_stroke_disabled : theme.control_stroke_default);

    const Color ink = text_color(theme, *this, state);
    const float face_size = theme.body_strong.size * s;
    const float line_height = theme.body_strong.line_height * s;
    const float label_width =
        label.empty() ? 0.0f : text_width(text, label.c_str(), dark ? kFaceSemiBold : kFaceRegular,
                                          face_size);
    const float icon_width = icon >= 0 ? kIconSize * s : 0.0f;
    const float gap = (icon >= 0 && !label.empty()) ? kIconGap * s : 0.0f;
    const float total = icon_width + gap + label_width;
    const float content_x = rect.x + (rect.w - total) * 0.5f;

    if (icon >= 0) {
        icons::draw(painter, icon, content_x, rect.y + (rect.h - icon_width) * 0.5f, kIconSize * s,
                    ink, s, icons::variant_for(dark));
    }
    if (!label.empty()) {
        const float text_y = rect.y + rect.h * 0.5f - line_height * 0.5f;
        draw_text(painter, text, label.c_str(), dark ? kFaceSemiBold : kFaceRegular, face_size,
                  line_height, content_x + icon_width + gap, text_y, ink);
    }
}

}  // namespace aether
