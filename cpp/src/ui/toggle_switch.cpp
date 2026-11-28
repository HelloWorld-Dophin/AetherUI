#include "ui/toggle_switch.h"

#include <algorithm>

namespace aether {
namespace {

constexpr float kTrackWidth = 40.0f;
constexpr float kTrackHeight = 20.0f;
constexpr float kKnobRest = 12.0f;
constexpr float kKnobHover = 14.0f;
constexpr float kKnobStretch = 8.0f;
constexpr float kKnobMargin = 4.0f;
constexpr float kChromePadding = 6.0f;
constexpr float kTravel = kTrackWidth - kKnobRest - kKnobMargin * 2.0f;
constexpr float kLabelGap = 12.0f;
constexpr float kMinHeight = 40.0f;
constexpr float kDragThreshold = 3.0f;
constexpr float kHoverDuration = 0.18f;
constexpr float kKnobDuration = 0.20f;
constexpr float kSnapDuration = 0.22f;
constexpr float kDisabledOpacity = 0.55f;

Color dim(Color c, float factor) { return with_alpha(c, c.a * factor); }

Color track_fill(const ToggleSwitch& t, const Theme& theme) {
    if (t.disabled) {
        return theme.toggle_disabled;
    }
    const bool active = t.pressed || t.dragging;
    if (t.on) {
        return (active || t.hovered) ? theme.toggle_track_on_focused : theme.toggle_track_on;
    }
    if (active) {
        return theme.control_fill_tertiary;
    }
    if (t.hovered) {
        return theme.control_fill_secondary;
    }
    return theme.toggle_track_off;
}

Color track_stroke(const ToggleSwitch& t, const Theme& theme) {
    if (t.disabled) {
        return theme.toggle_disabled;
    }
    return t.on ? theme.neutral_stroke_subtle : theme.toggle_thumb_stroke_off;
}

Color thumb_fill(const ToggleSwitch& t, const Theme& theme) {
    if (t.disabled) {
        return theme.neutral_background_1;
    }
    return t.on ? theme.toggle_thumb_fill_on : theme.toggle_thumb_fill_off;
}

Color thumb_stroke(const ToggleSwitch& t, const Theme& theme) {
    return t.disabled ? theme.toggle_disabled : theme.toggle_thumb_stroke_off;
}

}  // namespace

void ToggleSwitch::set_default_on(bool value) {
    on = value;
    pos.snap(value ? 1.0f : 0.0f);
    grow.snap(0.0f);
    hover.snap(0.0f);
}

void ToggleSwitch::measure(float label_width, const Theme& theme, float s) {
    const float image_w = (kTrackWidth + kChromePadding * 2.0f) * s;
    const float image_h = (kTrackHeight + kChromePadding * 2.0f) * s;
    const float content_h = std::max(image_h, theme.body.line_height * s);
    const float padding = std::max((kMinHeight * s - content_h) * 0.5f, 0.0f);
    rect.h = content_h + padding * 2.0f;
    rect.w = image_w + (label.empty() ? 0.0f : kLabelGap * s + label_width);
}

void ToggleSwitch::place(float x, float y, float s) {
    rect.x = x;
    rect.y = y;
    const float image_h = (kTrackHeight + kChromePadding * 2.0f) * s;
    track = Rect{x, y + (rect.h - image_h) * 0.5f, (kTrackWidth + kChromePadding * 2.0f) * s,
                 image_h};
}

void ToggleSwitch::update_hover(bool inside, double now, const Theme& theme) {
    if (disabled || hovered == inside) {
        return;
    }
    hovered = inside;
    if (pressed) {
        return;
    }
    if (!inside) {
        hover_suppressed = false;
    }
    hover.retarget((inside && !hover_suppressed) ? 1.0f : 0.0f, now, kHoverDuration, theme.easing);
}

void ToggleSwitch::update_press(float x, double now, const Theme& theme) {
    if (disabled) {
        return;
    }
    pressed = true;
    dragging = false;
    press_x = x;
    press_fraction = pos.value(now, kSnapDuration, theme.easing_decelerate);
    grow.retarget(1.0f, now, kKnobDuration, theme.easing);
    hover.retarget(1.0f, now, kHoverDuration, theme.easing);
}

void ToggleSwitch::update_drag(float x, double now, const Theme& theme) {
    if (!pressed) {
        return;
    }
    const float delta = x - press_x;
    if (!dragging && std::abs(delta) >= kDragThreshold * 1.0f) {
        dragging = true;
    }
    if (dragging) {
        const float fraction = std::clamp(press_fraction + delta / kTravel, 0.0f, 1.0f);
        pos.snap(fraction);
    }
}

void ToggleSwitch::update_release(double now, const Theme& theme) {
    if (!pressed) {
        return;
    }
    pressed = false;
    const bool was_dragging = dragging;
    dragging = false;

    if (was_dragging) {
        on = pos.value(now, kSnapDuration, theme.easing_decelerate) > 0.5f;
    } else {
        on = !on;
    }

    pos.retarget(on ? 1.0f : 0.0f, now, kSnapDuration, theme.easing_decelerate);
    grow.retarget(0.0f, now, kKnobDuration, theme.easing);
    hover_suppressed = true;
    hover.retarget(0.0f, now, kHoverDuration, theme.easing);
}

bool ToggleSwitch::animating(double now) const {
    return pos.running(now, kSnapDuration) || grow.running(now, kKnobDuration) ||
           hover.running(now, kHoverDuration);
}

bool ToggleSwitch::handle_event(const Event& event, const UiContext& ui) {
    switch (event.type) {
        case Event::Type::MouseMove:
            update_hover(rect.contains(event.x, event.y) && !disabled, ui.now, ui.theme);
            if (pressed) {
                update_drag(event.x, ui.now, ui.theme);
            }
            return false;
        case Event::Type::MouseDown:
            if (event.key != 1 || !hit_test(event.x, event.y)) {
                return false;
            }
            update_press(event.x, ui.now, ui.theme);
            return true;
        case Event::Type::MouseUp:
            if (event.key != 1 || !pressed) {
                return false;
            }
            update_release(ui.now, ui.theme);
            return true;
        default:
            return false;
    }
}

void ToggleSwitch::draw(const Context& ctx) const {
    const Theme& theme = ctx.theme;
    const float s = ctx.s;
    const double now = ctx.now;
    Painter& painter = ctx.painter;

    const float pos_value = pos.value(now, kSnapDuration, theme.easing_decelerate);
    const float grow_value = grow.value(now, kKnobDuration, theme.easing);
    const float hover_value = hover.value(now, kHoverDuration, theme.easing);
    const float opacity = disabled ? kDisabledOpacity : 1.0f;

    const Rect box{track.x + kChromePadding * s, track.y + kChromePadding * s, kTrackWidth * s,
                   kTrackHeight * s};
    const float radius = kTrackHeight * 0.5f * s;

    painter.fill_round_rect(box, radius, dim(track_fill(*this, theme), opacity));
    painter.stroke_round_rect(box, radius, theme.stroke_thickness * s,
                              dim(track_stroke(*this, theme), opacity));

    const float stroke_w = theme.stroke_thickness * s;
    const bool outlined = disabled || !on;
    const float adjust = outlined ? stroke_w : 0.0f;
    const float diameter = (kKnobRest + (kKnobHover - kKnobRest) * hover_value) * s;
    const float knob_h = diameter - adjust;
    const float knob_w = diameter + kKnobStretch * grow_value * s - adjust;
    const float knob_radius = knob_h * 0.5f;

    const float raw_center_x =
        box.x + kKnobMargin * s + kKnobRest * 0.5f * s + kTravel * s * pos_value;
    const float stretch_half = kKnobStretch * grow_value * s * 0.5f;
    const float center_x =
        std::clamp(raw_center_x, box.x + kKnobMargin * s + kKnobRest * 0.5f * s + stretch_half,
                   box.right() - kKnobMargin * s - kKnobRest * 0.5f * s - stretch_half);
    const float center_y = box.y + box.h * 0.5f;
    const Rect knob{center_x - knob_w * 0.5f, center_y - knob_h * 0.5f, knob_w, knob_h};

    painter.fill_round_rect(knob, knob_radius, dim(thumb_fill(*this, theme), opacity));
    if (outlined) {
        painter.stroke_round_rect(knob, knob_radius, stroke_w,
                                  dim(thumb_stroke(*this, theme), opacity));
    }

    if (!label.empty()) {
        const Color ink = disabled ? theme.text_disabled : theme.text_primary;
        const float line_height = theme.body.line_height * s;
        const float label_x = track.right() + kLabelGap * s;
        draw_text(painter, ctx.text, label.c_str(), kFaceRegular, theme.body.size * s, line_height,
                  label_x, rect.y + (rect.h - line_height) * 0.5f, ink);
    }
}

}  // namespace aether
