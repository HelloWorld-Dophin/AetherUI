#include "ui/icon_tile.h"

#include "ui/icons.h"
#include "ui/label.h"

namespace aether {
namespace {

constexpr float kIconSize = 28.0f;
constexpr float kPadding = 10.0f;
constexpr float kActionSize = 32.0f;

}  // namespace

void IconTile::place(float x, float y, float w, float h, float s) {
    rect = Rect{x, y, w, h};
    const float size = kActionSize * s;
    action.rect = Rect{rect.right() - kPadding * s - size, rect.bottom() - kPadding * s - size,
                       size, size};
}

void IconTile::draw(const Context& ctx) const {
    const Theme& theme = ctx.theme;
    const float s = ctx.s;

    ctx.painter.fill_round_rect(rect, theme.radius_card * s, theme.neutral_background_1);
    ctx.painter.stroke_round_rect(rect, theme.radius_card * s, theme.stroke_thickness * s,
                                  theme.card_stroke_default);

    if (icon >= 0) {
        const float size = kIconSize * s;
        icons::draw(ctx.painter, icon, rect.x + (rect.w - size) * 0.5f, rect.y + kPadding * s,
                    size, theme.text_primary, s, icons::variant_for(ctx.dark));
    }
    if (!name.empty()) {
        const TypeStyle& ts = type_style(theme, TextStyle::Caption);
        const float line_height = ts.line_height * s;
        Label label;
        label.text = name;
        label.face = kFaceMono;
        label.style = TextStyle::Caption;
        label.role = TextRole::Secondary;
        label.layout(ctx.text, theme, s, rect.x + kPadding * s,
                     action.rect.y + (action.rect.h - line_height) * 0.5f, 0.0f);
        label.draw(ctx);
    }
    action.draw(ctx);
}

}  // namespace aether
