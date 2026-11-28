#include "ui/scroll_bar.h"

#include <algorithm>

namespace aether {
namespace {

constexpr float kWidgetThickness = 12.0f;
constexpr float kBarThickness = 6.0f;
constexpr float kTrackInset = 2.0f;
constexpr float kMinThumb = 16.0f;

}  // namespace

void ScrollBar::layout(float x, float y, float w, float h, float s) {
    rect = Rect{x, y, w, h};
    scale = s;
}

void ScrollBar::set_range(double lo, double hi) {
    min = std::min(lo, hi);
    max = std::max(lo, hi);
    value = std::clamp(value, min, max);
}

void ScrollBar::set_page_size(double size) { page_size = std::max(size, 0.0); }

void ScrollBar::set_value(double v) { value = std::clamp(v, min, max); }

float ScrollBar::track_length() const {
    return std::max(rect.h - kTrackInset * 2.0f * scale, 0.0f);
}

float ScrollBar::thumb_length() const {
    const float track = track_length();
    const double content = (max - min) + page_size;
    const double ratio = content <= 0.0 ? 1.0 : std::clamp(page_size / content, 0.0, 1.0);
    return std::clamp(track * static_cast<float>(ratio), std::min(kMinThumb * scale, track), track);
}

float ScrollBar::thumb_start() const {
    const float movable = std::max(track_length() - thumb_length(), 0.0f);
    const double span = max - min;
    const float progress =
        span <= 0.0 ? 0.0f : static_cast<float>((value - min) / span);
    return (kTrackInset + std::clamp(progress, 0.0f, 1.0f) * movable / scale) * scale;
}

void ScrollBar::press_at(float position, double now, const Theme& theme) {
    const float start = thumb_start();
    const float length = thumb_length();
    grab = (position >= start && position <= start + length) ? position - start : length * 0.5f;
    dragging = true;
    press.retarget(1.0f, now, theme.motion_fast, theme.easing);
    drag_to(position);
}

void ScrollBar::drag_to(float position) {
    if (!dragging) {
        return;
    }
    const float movable = std::max(track_length() - thumb_length(), 0.0f);
    if (movable <= 0.0f) {
        return;
    }
    const float offset = std::clamp(position - grab - kTrackInset, 0.0f, movable);
    apply(min + static_cast<double>(offset / movable) * (max - min));
}

void ScrollBar::release(double now, const Theme& theme) {
    dragging = false;
    press.retarget(0.0f, now, theme.motion_fast, theme.easing);
}

void ScrollBar::wheel(float delta) { apply(value - static_cast<double>(delta)); }

bool ScrollBar::handle_event(const Event& event, const UiContext& ui) {
    switch (event.type) {
        case Event::Type::MouseMove: {
            const bool inside = rect.contains(event.x, event.y);
            if (inside != hovered) {
                hovered = inside;
                hover.retarget(inside ? 1.0f : 0.0f, ui.now, ui.theme.motion_fast, ui.theme.easing);
            }
            if (dragging) {
                drag_to(event.y - rect.y);
            }
            return false;
        }
        case Event::Type::MouseDown:
            if (event.key != 1 || !rect.contains(event.x, event.y)) {
                return false;
            }
            press_at(event.y - rect.y, ui.now, ui.theme);
            return true;
        case Event::Type::MouseUp:
            if (event.key != 1 || !dragging) {
                return false;
            }
            release(ui.now, ui.theme);
            return true;
        default:
            return false;
    }
}

bool ScrollBar::animating(double now) const {
    return hover.running(now, 0.15f) || press.running(now, 0.15f);
}

void ScrollBar::apply(double v) { value = std::clamp(v, min, max); }

void ScrollBar::draw(const Context& ctx) const {
    const Theme& theme = ctx.theme;
    const float s = ctx.s;
    const double now = ctx.now;
    Painter& painter = ctx.painter;

    const float hover_value = hover.value(now, theme.motion_fast, theme.easing);
    const float press_value = press.value(now, theme.motion_fast, theme.easing);
    const float active = std::max(hover_value, press_value);

    const float bar_w = kBarThickness * s;
    const float radius = bar_w * 0.5f;
    const float track_start = rect.y + kTrackInset * s;
    const float track_len = std::max(rect.h - kTrackInset * 2.0f * s, 0.0f);

    if (hover_value > 0.001f) {
        painter.fill_round_rect(Rect{rect.x + (rect.w - bar_w) * 0.5f, track_start, bar_w, track_len},
                                radius, with_alpha(theme.scrollbar_track, theme.scrollbar_track.a * active));
    }

    const float length = thumb_length();
    const float start = rect.y + thumb_start();
    const Rect thumb{rect.x + (rect.w - bar_w) * 0.5f, start, bar_w, length};

    Color base = disabled ? with_alpha(theme.scrollbar_thumb, theme.scrollbar_thumb.a * 0.4f)
                          : mix(theme.scrollbar_thumb, theme.scrollbar_thumb_hover, active);
    painter.fill_round_rect(thumb, radius, base);
}

}  // namespace aether
