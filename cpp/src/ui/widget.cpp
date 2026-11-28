#include "ui/widget.h"

#include <algorithm>

namespace aether {

Anim Anim::at(float value) {
    Anim anim;
    anim.from = value;
    anim.to = value;
    anim.started = -1.0;
    return anim;
}

float Anim::value(double now, float duration, const CubicBezier& curve) const {
    if (started < 0.0) {
        return to;
    }
    const float total = std::max(duration, 1e-5f);
    const float t = static_cast<float>((now - started) / total);
    if (t >= 1.0f) {
        return to;
    }
    return from + (to - from) * curve.solve(std::max(t, 0.0f));
}

void Anim::retarget(float target, double now, float duration, const CubicBezier& curve) {
    const float current = value(now, duration, curve);
    if (std::abs(current - target) < 1e-6f && started < 0.0) {
        return;
    }
    from = current;
    to = target;
    started = now;
}

void Anim::snap(float target) {
    from = target;
    to = target;
    started = -1.0;
}

bool Anim::running(double now, float duration) const {
    return started >= 0.0 && (now - started) < duration;
}

float Anim::linear_progress(double now, float duration) const {
    if (started < 0.0) {
        return 1.0f;
    }
    return std::clamp(static_cast<float>((now - started) / std::max(duration, 1e-5f)), 0.0f, 1.0f);
}

bool Interaction::hover(float x, float y, Rect bounds) {
    if (!enabled) {
        return false;
    }
    hovered = bounds.contains(x, y);
    if (!hovered) {
        pressed = false;
    }
    return hovered;
}

void Interaction::press(bool down) { pressed = down && enabled && hovered; }

void Interaction::leave() {
    hovered = false;
    pressed = false;
}

ControlState Interaction::state() const {
    if (!enabled) {
        return ControlState::Disabled;
    }
    if (pressed) {
        return ControlState::Pressed;
    }
    if (hovered) {
        return ControlState::PointerOver;
    }
    return ControlState::Normal;
}

TextLayout make_text(TextRenderer& text, const char* utf8, int face, float size, float line_height,
                     float max_width, int align) {
    TextFormat fmt;
    fmt.face = face;
    fmt.size = size;
    fmt.line_height = line_height;
    fmt.max_width = max_width;
    fmt.align = align;
    return text.layout(utf8, fmt);
}

float draw_text(Painter& painter, TextRenderer& text, const char* utf8, int face, float size,
                float line_height, float x, float y, Color color) {
    const TextLayout layout = make_text(text, utf8, face, size, line_height);
    text.draw(painter, layout, x, y, color);
    return layout.width;
}

float text_width(TextRenderer& text, const char* utf8, int face, float size) {
    return text.measure(utf8, face, size);
}

}  // namespace aether
