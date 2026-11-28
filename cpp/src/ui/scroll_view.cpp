#include "ui/scroll_view.h"

#include <algorithm>

namespace aether {
namespace {

constexpr float kBarWidth = 12.0f;
constexpr float kBarSpace = 20.0f;

}  // namespace

void ScrollView::layout(float x, float y, float w, float h, float content_h, float s) {
    scale = s;
    rect = Rect{x, y, w, h};
    bar.layout(x + w - kBarWidth * s, y, kBarWidth * s, h, s);
    bar.set_page_size(h);
    bar.set_range(0.0, std::max(content_h - h, 0.0f));
}

Rect ScrollView::content_rect() const {
    return Rect{rect.x, rect.y, std::max(rect.w - kBarSpace * scale, 0.0f), rect.h};
}

bool ScrollView::hit_test(float x, float y) const {
    return rect.contains(x, y) || bar.rect.contains(x, y);
}

bool ScrollView::handle_event(const Event& event, const UiContext& ui) {
    switch (event.type) {
        case Event::Type::MouseMove:
            bar.handle_event(event, ui);
            return false;
        case Event::Type::MouseDown:
            if (bar.rect.contains(event.x, event.y)) {
                return bar.handle_event(event, ui);
            }
            return false;
        case Event::Type::MouseUp:
            return bar.handle_event(event, ui);
        case Event::Type::Wheel:
            if (!hit_test(event.x, event.y)) {
                return false;
            }
            bar.wheel(event.wheel);
            return true;
        default:
            return false;
    }
}

Recti ScrollView::begin_clip(const Context& ctx) const {
    const Recti saved = ctx.painter.clip();
    ctx.painter.set_clip(intersect(saved, cover(content_rect())));
    return saved;
}

void ScrollView::end_clip(const Context& ctx, Recti saved) const { ctx.painter.set_clip(saved); }

}  // namespace aether
