#include "ui/card.h"

namespace aether {

void Card::place(float x, float y, float w, float h) { rect = Rect{x, y, w, h}; }

void Card::draw(const Context& ctx) const {
    const Theme& theme = ctx.theme;
    const float s = ctx.s;
    ctx.painter.fill_round_rect(rect, theme.radius_card * s, theme.neutral_background_1);
    ctx.painter.stroke_round_rect(rect, theme.radius_card * s, theme.stroke_thickness * s,
                                  theme.card_stroke_default);
}

}  // namespace aether
