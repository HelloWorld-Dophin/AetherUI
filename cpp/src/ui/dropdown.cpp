#include "ui/dropdown.h"

#include <algorithm>
#include <cmath>

#include "ui/icons.h"

namespace aether {
namespace {

constexpr float kFieldHeight = 44.0f;
constexpr float kChromePadding = 4.0f;
constexpr float kPaddingHorizontal = 10.0f;
constexpr float kIconSize = 16.0f;
constexpr float kChevronSize = 12.0f;
constexpr float kFieldMinWidth = 140.0f;
constexpr float kFieldMaxWidth = 280.0f;
constexpr float kItemPaddingVertical = 6.0f;
constexpr float kItemMinHeight = 28.0f;
constexpr float kItemGap = 2.0f;
constexpr float kPanelPadding = 4.0f;
constexpr float kBarWidth = 3.0f;
constexpr float kPanelGap = 6.0f;
constexpr float kPanelDuration = 0.11f;

float item_height(const Theme& theme, float s) {
    const float content = std::max(theme.body.line_height * s, kIconSize * s);
    return std::max(content + kItemPaddingVertical * 2.0f * s, kItemMinHeight * s);
}

float progress_of(const Dropdown& d, double now, const Theme& theme) {
    return d.panel.value(now, kPanelDuration, Theme::focus_underline());
}

float estimated_text_width(TextRenderer& text, const std::string& value, float size) {
    return text_width(text, value.c_str(), kFaceRegular, size);
}

void draw_chevron(Painter& painter, const Rect& area, Color color, float s) {
    const float center_x = area.right() - (kPaddingHorizontal + kChevronSize * 0.5f) * s;
    const float center_y = area.y + area.h * 0.5f;
    const float wing = kChevronSize * 0.22f * s;
    const float reach = kChevronSize * 0.34f * s;
    const float brush = 1.2f * s;
    painter.stroke_line(center_x - reach, center_y - wing, center_x, center_y + wing, brush, color);
    painter.stroke_line(center_x, center_y + wing, center_x + reach, center_y - wing, brush, color);
}

}  // namespace

void Dropdown::measure(TextRenderer& text, const Theme& theme, float s) {
    field.h = (kFieldHeight + kChromePadding * 2.0f) * s;
    if (width > 0.0f) {
        field.w = (std::max(width, kFieldMinWidth) + kChromePadding * 2.0f) * s;
        return;
    }
    const DropdownOption* item = selected_item();
    const std::string label = item != nullptr ? item->label : std::string("请选择");
    const float content =
        estimated_text_width(text, label, theme.body.size * s) + 8.0f * s + (kChevronSize + 4.0f) * s;
    const float computed =
        (kPaddingHorizontal * 2.0f + kChromePadding * 2.0f) * s + content;
    field.w = std::clamp(computed, kFieldMinWidth * s, kFieldMaxWidth * s);
}

void Dropdown::place(float x, float y) {
    field.x = x;
    field.y = y;
    last_field_top = y;
}

void Dropdown::toggle(double now, const Theme& theme) {
    if (disabled) {
        return;
    }
    if (open) {
        close(now, theme);
        return;
    }
    open = true;
    hover_item = -1;
    panel.retarget(1.0f, now, kPanelDuration, Theme::focus_underline());
}

void Dropdown::close(double now, const Theme& theme) {
    if (!open) {
        return;
    }
    open = false;
    hover_item = -1;
    panel.retarget(0.0f, now, kPanelDuration, Theme::focus_underline());
}

void Dropdown::select(int index, double now, const Theme& theme) {
    if (index >= 0 && index < static_cast<int>(items.size()) && !items[static_cast<size_t>(index)].disabled) {
        selected = index;
    }
    close(now, theme);
}

bool Dropdown::panel_visible(double now) const { return open || panel.running(now, kPanelDuration); }

bool Dropdown::animating(double now) const { return panel.running(now, kPanelDuration); }

float Dropdown::panel_height(const Theme& theme, float s) const {
    const float h = item_height(theme, s);
    const float count = static_cast<float>(items.size());
    if (count <= 0.0f) {
        return kPanelPadding * 2.0f * s;
    }
    return count * h + (count - 1.0f) * kItemGap * s + kPanelPadding * 2.0f * s;
}

float Dropdown::panel_top(float field_top, const Theme& theme, float s) const {
    const float field_height = field.h;
    const float height = panel_height(theme, s);
    switch (placement) {
        case PanelPlacement::Above:
            return field_top - height - kPanelGap * s;
        case PanelPlacement::Below:
            return field_top + field_height + kPanelGap * s;
        default:
            return field_top + field_height * 0.5f - height * 0.5f;
    }
}

Rect Dropdown::panel_rect(const Theme& theme, float s) const {
    return Rect{field.x, panel_top(field.y, theme, s), chrome_width(), panel_height(theme, s)};
}

Rect Dropdown::item_rect(size_t index, const Theme& theme, float s) const {
    const Rect panel = panel_rect(theme, s);
    const float row_h = item_height(theme, s);
    return Rect{panel.x + kPanelPadding * s,
                panel.y + kPanelPadding * s +
                    static_cast<float>(index) * (row_h + kItemGap * s),
                panel.w - kPanelPadding * 2.0f * s, row_h};
}

int Dropdown::item_at(float x, float y, const Theme& theme, float s) const {
    for (size_t i = 0; i < items.size(); ++i) {
        if (items[i].disabled) {
            continue;
        }
        if (item_rect(i, theme, s).contains(x, y)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool Dropdown::handle_event(const Event& event, const UiContext& ui) {
    if (event.type == Event::Type::MouseMove) {
        hover_item = open ? item_at(event.x, event.y, ui.theme, ui.s) : -1;
    }
    return false;
}

bool DropdownGroup::handle_event(const Event& event, const UiContext& ui) {
    if (event.type == Event::Type::MouseMove) {
        for (Dropdown* dropdown : items_) {
            dropdown->handle_event(event, ui);
        }
        return false;
    }
    if (event.type != Event::Type::MouseDown || event.key != 1) {
        return false;
    }

    // 后加入的面板在上层，先判面板，再判闭合框。
    for (auto it = items_.rbegin(); it != items_.rend(); ++it) {
        Dropdown& dropdown = **it;
        if (!dropdown.open || !dropdown.panel_rect(ui.theme, ui.s).contains(event.x, event.y)) {
            continue;
        }
        const int index = dropdown.item_at(event.x, event.y, ui.theme, ui.s);
        if (index >= 0) {
            dropdown.select(index, ui.now, ui.theme);
            if (dropdown.on_select) {
                dropdown.on_select(index);
            }
        }
        return true;
    }
    for (auto it = items_.rbegin(); it != items_.rend(); ++it) {
        Dropdown& dropdown = **it;
        if (!dropdown.field.contains(event.x, event.y)) {
            continue;
        }
        close_all(ui.now, ui.theme);
        dropdown.toggle(ui.now, ui.theme);
        return true;
    }
    // 点击面板与闭合框之外：全部收起。
    close_all(ui.now, ui.theme);
    return false;
}

void DropdownGroup::close_all(double now, const Theme& theme) {
    for (Dropdown* dropdown : items_) {
        dropdown->close(now, theme);
    }
}

void DropdownGroup::draw_panels(const Context& ctx) const {
    for (const Dropdown* dropdown : items_) {
        if (!dropdown->panel_visible(ctx.now)) {
            continue;
        }
        dropdown->draw_panel(ctx, dropdown->panel_top(dropdown->field.y, ctx.theme, ctx.s));
    }
}

bool DropdownGroup::animating(double now) const {
    for (const Dropdown* dropdown : items_) {
        if (dropdown->animating(now)) {
            return true;
        }
    }
    return false;
}

const DropdownOption* Dropdown::selected_item() const {
    if (selected < 0 || selected >= static_cast<int>(items.size())) {
        return nullptr;
    }
    return &items[static_cast<size_t>(selected)];
}

void Dropdown::draw_field(const Context& ctx) const {
    const Theme& theme = ctx.theme;
    const float s = ctx.s;
    Painter& painter = ctx.painter;

    const float radius = theme.radius_text_box * s;
    // `field` 是含 4px 底盘边距的布局盒，可视区域在其内部。
    const Rect area{field.x + kChromePadding * s, field.y + kChromePadding * s,
                    field.w - kChromePadding * 2.0f * s, kFieldHeight * s};

    const Rect baseline{area.x + radius, area.bottom() - s, area.w - radius * 2.0f, s};
    const Recti saved_clip = painter.clip();
    painter.set_clip(intersect(saved_clip, cover(Rect{area.x, area.bottom() - 0.5f * s, area.w,
                                                      kChromePadding * s})));
    painter.draw_shadow(baseline, 0.0f, theme.shadow_text_box.color, theme.shadow_text_box.sigma() * s,
                        0.0f, theme.shadow_text_box.dy * s);
    painter.set_clip(saved_clip);

    painter.fill_round_rect(area, radius, theme.neutral_background_1);
    painter.stroke_round_rect(area, radius, theme.stroke_thickness * s,
                              theme.control_stroke_default);
    painter.fill_rect(baseline, theme.control_stroke_secondary);

    draw_chevron(painter, area, theme.text_secondary, s);

    const DropdownOption* item = selected_item();
    const char* label = item != nullptr ? item->label.c_str() : "请选择";
    const Color ink = item != nullptr ? theme.text_primary : theme.text_secondary;
    const float line_height = theme.body.line_height * s;
    draw_text(painter, ctx.text, label, kFaceRegular, theme.body.size * s, line_height,
              area.x + (kPaddingHorizontal + kChromePadding) * s,
              area.y + (area.h - line_height) * 0.5f, ink);
}

void Dropdown::draw_panel(const Context& ctx, float top) const {
    const Theme& theme = ctx.theme;
    const float s = ctx.s;
    const double now = ctx.now;
    Painter& painter = ctx.painter;

    const float progress = progress_of(*this, now, theme);
    if (progress <= 0.001f) {
        return;
    }

    const float full_height = panel_height(theme, s);
    const float height = full_height * progress;
    const float panel_width = chrome_width();
    float y = top;

    // 上方展开时贴底、居中时向两侧展开、下方时贴顶。
    if (placement == PanelPlacement::Above) {
        y = top + (full_height - height);
    } else if (placement == PanelPlacement::Center) {
        y = top + (full_height - height) * 0.5f;
    }

    const Rect panel{field.x, y, panel_width, height};
    const float radius = theme.radius_overlay * s;
    painter.fill_round_rect(panel, radius, theme.neutral_background_1);
    painter.stroke_round_rect(panel, radius, theme.stroke_thickness * s,
                              ctx.dark ? color_rgba8(0x00, 0x00, 0x00, 0.50f)
                                       : color_rgba8(0x00, 0x00, 0x00, 0.10f));

    const Recti saved_clip = painter.clip();
    painter.set_clip(intersect(saved_clip, cover(panel)));

    const float row_h = item_height(theme, s);
    const float line_height = theme.body.line_height * s;
    const float bar_height = row_h * 0.5f;
    for (size_t i = 0; i < items.size(); ++i) {
        const DropdownOption& item = items[i];
        const float item_y = panel.y + kPanelPadding * s + static_cast<float>(i) * (row_h + kItemGap * s);
        const Rect row{panel.x + kPanelPadding * s, item_y, panel.w - kPanelPadding * 2.0f * s, row_h};
        const bool hovered = !item.disabled && hover_item == static_cast<int>(i);
        const bool is_selected = selected == static_cast<int>(i);
        if (hovered) {
            painter.fill_round_rect(row, theme.radius_control * s, theme.neutral_background_2);
        }
        if (is_selected) {
            painter.fill_round_rect(
                Rect{row.x + 6.0f * s, row.y + (row.h - bar_height) * 0.5f, kBarWidth * s,
                     bar_height},
                kBarWidth * 0.5f * s, theme.accent_fill_default);
        }

        float content_x = row.x + 6.0f * s + kBarWidth * s + 9.0f * s;
        if (item.icon >= 0) {
            icons::draw(painter, item.icon, content_x, row.y + (row.h - kIconSize * s) * 0.5f,
                        kIconSize * s, item.disabled ? theme.text_disabled : theme.text_primary,
                        s, icons::variant_for(ctx.dark));
            content_x += (kIconSize + 8.0f) * s;
        }
        draw_text(painter, ctx.text, item.label.c_str(), kFaceRegular, theme.body.size * s,
                  line_height, content_x, row.y + (row.h - line_height) * 0.5f,
                  item.disabled ? theme.text_disabled : theme.text_primary);
    }

    painter.set_clip(saved_clip);
}

}  // namespace aether
