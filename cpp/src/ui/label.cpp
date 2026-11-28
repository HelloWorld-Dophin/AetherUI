#include "ui/label.h"

#include <algorithm>

namespace aether {
namespace {

constexpr float kTitleGap = 8.0f;

}  // namespace

void Label::layout(TextRenderer& text_renderer, const Theme& theme, float s, float x, float y,
                   float width) {
    const TypeStyle& ts = type_style(theme, style);
    const float font_size = (size > 0.0f ? size : ts.size) * s;
    const float line_height_px = (line_height > 0.0f ? line_height : ts.line_height) * s;
    const TextLayout measured = make_text(text_renderer, text.c_str(), face, font_size,
                                          line_height_px, wrap ? width : 0.0f);
    rect.x = x;
    rect.y = y;
    rect.w = (wrap && width > 0.0f) ? width : measured.width;
    rect.h = std::max(measured.height, line_height_px);
}

void Label::draw(const Context& ctx) const {
    if (text.empty()) {
        return;
    }
    const TypeStyle& ts = type_style(ctx.theme, style);
    const float font_size = (size > 0.0f ? size : ts.size) * ctx.s;
    const float line_height_px = (line_height > 0.0f ? line_height : ts.line_height) * ctx.s;
    const TextLayout layout = make_text(ctx.text, text.c_str(), face, font_size, line_height_px,
                                        wrap ? rect.w : 0.0f);
    const float x = center ? rect.x + (rect.w - layout.width) * 0.5f : rect.x;
    const float y = rect.y + (rect.h - layout.height) * 0.5f;
    ctx.text.draw(ctx.painter, layout, x, y, role_color(ctx.theme, role));
}

void PageHeader::set(const std::string& title_text, const std::string& caption_text) {
    title.text = title_text;
    title.face = kFaceSemiBold;
    title.style = TextStyle::Title;
    title.role = TextRole::Primary;
    caption.text = caption_text;
    caption.face = kFaceRegular;
    caption.style = TextStyle::Caption;
    caption.role = TextRole::Secondary;
}

void PageHeader::layout(TextRenderer& text_renderer, const Theme& theme, float s, float x, float y,
                        float width) {
    title.layout(text_renderer, theme, s, x, y, width);
    caption.layout(text_renderer, theme, s, x, title.rect.bottom() + kTitleGap * s, width);
}

void PageHeader::draw(const Context& ctx) const {
    title.draw(ctx);
    caption.draw(ctx);
}

}  // namespace aether
