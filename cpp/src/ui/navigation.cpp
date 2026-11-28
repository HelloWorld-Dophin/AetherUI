#include "ui/navigation.h"

#include <algorithm>
#include <cmath>

#include "ui/icons.h"

namespace aether {
namespace {

constexpr float kCompactWidth = 56.0f;
constexpr float kExpandedWidth = 240.0f;
constexpr float kHeaderHeight = 52.0f;
constexpr float kListTop = 8.0f;
constexpr float kListPadding = 8.0f;
constexpr float kItemGap = 4.0f;
constexpr float kItemHeight = 44.0f;
constexpr float kIconSize = 18.0f;
constexpr float kIconLeftPadding = (kCompactWidth - kIconSize) * 0.5f - kListPadding;
constexpr float kLabelRevealThreshold = 0.55f;
constexpr float kHamburgerDuration = 0.24f;
constexpr float kBarMoveDuration = 0.50f;
constexpr float kBarHeight = 14.0f;
constexpr float kBarWidth = 3.0f;
constexpr float kBarStretchMax = 0.35f;
constexpr float kBarRestStretch = 0.4f;
constexpr float kBarThinMax = 0.25f;
constexpr float kNavWidthDuration = 0.167f;

}  // namespace

NavigationView::NavigationView() = default;

float NavigationView::nav_width(double now, const Theme& theme) const {
    return std::clamp(width.value(now, kNavWidthDuration, Theme::focus_underline()), kCompactWidth,
                      kExpandedWidth);
}

void NavigationView::set_expanded(bool value, double now, const Theme& theme) {
    expanded = value;
    width.retarget(value ? kExpandedWidth : kCompactWidth, now, kNavWidthDuration,
                   Theme::focus_underline());
}

void NavigationView::toggle_expanded(double now, const Theme& theme) {
    set_expanded(!expanded, now, theme);
}

void NavigationView::set_selected(int index, double now, float s) {
    const int total = static_cast<int>(items.size() + footer.size());
    if (total == 0) {
        return;
    }
    selected = std::clamp(index, 0, total - 1);
    for (size_t i = 0; i < items.size(); ++i) {
        items[i].selected = static_cast<int>(i) == selected;
    }
    const int main_count = static_cast<int>(items.size());
    for (size_t i = 0; i < footer.size(); ++i) {
        footer[i].selected = main_count + static_cast<int>(i) == selected;
    }
    start_bar_flow(now, s);
}

void NavigationView::press_feedback(double now) {
    hamburger.retarget(0.55f, now, kHamburgerDuration, Theme::navigation());
}

void NavigationView::pointer_up(double now) {
    hamburger.retarget(1.0f, now, kHamburgerDuration, Theme::navigation());
}

bool NavigationView::animating(double now, const Theme& theme) const {
    (void)theme;
    return width.running(now, kNavWidthDuration) || hamburger.running(now, kHamburgerDuration) ||
           bar.running(now, kBarMoveDuration);
}

bool NavigationView::handle_event(const Event& event, const UiContext& ui) {
    switch (event.type) {
        case Event::Type::MouseMove: {
            hamburger_hovered = hamburger_at(event.x, event.y);
            const int hovered = item_at(event.x, event.y, ui.s);
            for (size_t i = 0; i < items.size(); ++i) {
                items[i].hovered = hovered == static_cast<int>(i);
            }
            for (size_t i = 0; i < footer.size(); ++i) {
                footer[i].hovered = hovered == static_cast<int>(items.size() + i);
            }
            return false;
        }
        case Event::Type::MouseDown: {
            if (event.key != 1 || !rect.contains(event.x, event.y)) {
                return false;
            }
            if (hamburger_at(event.x, event.y)) {
                press_feedback(ui.now);
                toggle_expanded(ui.now, ui.theme);
                return true;
            }
            const int index = item_at(event.x, event.y, ui.s);
            if (index < 0) {
                return false;
            }
            set_selected(index, ui.now, ui.s);
            if (on_select) {
                on_select(index);
            }
            return true;
        }
        case Event::Type::MouseUp:
            if (event.key == 1) {
                pointer_up(ui.now);
            }
            return false;
        default:
            return false;
    }
}

void NavigationView::layout(float x, float y, float w, float h, float s) {
    rect = Rect{x, y, w, h};
    hamburger_rect = Rect{x + kListPadding * s, y + (kHeaderHeight * s - 40.0f * s) * 0.5f,
                          40.0f * s, 40.0f * s};
    if (hamburger_rect.y < y) {
        hamburger_rect.y = y;
    }
    region = Rect{x, y + kHeaderHeight * s, w, std::max(h - kHeaderHeight * s, 0.0f)};
    if (!bar_ready) {
        const float target = bar_target(region.h, s);
        if (target >= 0.0f) {
            bar.snap(target);
            bar_rendered = target;
            bar_ready = true;
        }
    }
    (void)h;
}

float NavigationView::footer_height(float s) const {
    if (footer.empty()) {
        return 0.0f;
    }
    return kListTop * 2.0f * s + static_cast<float>(footer.size()) * kItemHeight * s +
           (static_cast<float>(footer.size()) - 1.0f) * kItemGap * s;
}

float NavigationView::main_bottom(float s) const {
    if (items.empty()) {
        return kListTop * s;
    }
    return kListTop * s + static_cast<float>(items.size()) * kItemHeight * s +
           (static_cast<float>(items.size()) - 1.0f) * kItemGap * s;
}

float NavigationView::footer_top(float region_h, float s) const {
    return std::max(region_h - footer_height(s), main_bottom(s));
}

Rect NavigationView::item_rect(int index, float s) const {
    const int main_count = static_cast<int>(items.size());
    if (index >= 0 && index < main_count) {
        return Rect{region.x + kListPadding * s,
                    region.y + kListTop * s + static_cast<float>(index) * (kItemHeight + kItemGap) * s,
                    std::max(rect.w - kListPadding * 2.0f * s, 0.0f), kItemHeight * s};
    }
    const int local = index - main_count;
    if (local >= 0 && local < static_cast<int>(footer.size())) {
        return Rect{region.x + kListPadding * s,
                    region.y + footer_top(region.h, s) + kListTop * s +
                        static_cast<float>(local) * (kItemHeight + kItemGap) * s,
                    std::max(rect.w - kListPadding * 2.0f * s, 0.0f), kItemHeight * s};
    }
    return Rect{};
}

int NavigationView::item_at(float x, float y, float s) const {
    if (!region.contains(x, y)) {
        return -1;
    }
    const int total = static_cast<int>(items.size() + footer.size());
    for (int i = 0; i < total; ++i) {
        if (item_rect(i, s).contains(x, y)) {
            return i;
        }
    }
    return -1;
}

float NavigationView::bar_target(float region_h, float s) const {
    const int main_count = static_cast<int>(items.size());
    if (selected >= main_count && selected < main_count + static_cast<int>(footer.size())) {
        const int local = selected - main_count;
        return footer_top(region_h, s) + kListTop * s +
               static_cast<float>(local) * (kItemHeight + kItemGap) * s + kItemHeight * 0.5f * s;
    }
    if (selected < main_count) {
        return kListTop * s + static_cast<float>(selected) * (kItemHeight + kItemGap) * s +
               kItemHeight * 0.5f * s - scroll_offset;
    }
    return -1.0f;
}

void NavigationView::start_bar_flow(double now, float s) {
    const float target = bar_target(region.h, s);
    if (target < 0.0f) {
        return;
    }
    const bool running = bar.running(now, kBarMoveDuration);
    const float current =
        running ? bar.value(now, kBarMoveDuration, Theme::navigation())
                : (bar_rendered > 0.0f ? bar_rendered : bar.to);

    if (std::abs(current - target) < 0.001f) {
        bar = Anim::at(target);
        return;
    }
    if (running && std::abs(bar.to - target) < 0.001f) {
        return;
    }
    bar.from = current;
    bar.to = target;
    bar.started = now;
}

void NavigationView::draw(const Context& ctx) const {
    const Theme& theme = ctx.theme;
    const float s = ctx.s;
    const double now = ctx.now;
    Painter& painter = ctx.painter;

    const float nav_w = nav_width(now, theme) * s;
    const float reveal =
        std::clamp((nav_w / s - kCompactWidth) / (kExpandedWidth - kCompactWidth), 0.0f, 1.0f);
    const float radius = corner_radius * s;
    const Rect box{rect.x, rect.y, nav_w, rect.h};

    // 背景：仅左下角圆角（最大化时 corner_radius = 0）。
    painter.fill_round_rect(box, radius, theme.solid_background_secondary);
    if (radius > 0.0f) {
        painter.fill_rect(Rect{box.x, box.y, box.w, radius}, theme.solid_background_secondary);
        painter.fill_rect(Rect{box.right() - radius, box.y, radius, box.h - radius},
                          theme.solid_background_secondary);
    }

    // 汉堡按钮：40×40 的命中区域 + 三条 18×2 横线。
    const float line_scale = hamburger.value(now, kHamburgerDuration, Theme::navigation());
    if (hamburger_hovered) {
        painter.fill_round_rect(hamburger_rect, theme.radius_control * s,
                                theme.subtle_fill_secondary);
    }
    const float line_w = 18.0f * s * line_scale;
    const float line_h = 2.0f * s;
    const float gap = 4.0f * s;
    const float total_h = line_h * 3.0f + gap * 2.0f;
    const float line_x = hamburger_rect.x + (hamburger_rect.w - line_w) * 0.5f;
    float line_y = hamburger_rect.y + (hamburger_rect.h - total_h) * 0.5f;
    for (int i = 0; i < 3; ++i) {
        painter.fill_round_rect(Rect{line_x, line_y, line_w, line_h}, line_h * 0.5f,
                                theme.text_primary);
        line_y += line_h + gap;
    }

    // 条目列表（裁剪到条目区域）。
    const Recti saved_clip = painter.clip();
    painter.set_clip(intersect(saved_clip, cover(region)));

    const int total = static_cast<int>(items.size() + footer.size());
    const float label_alpha =
        reveal > kLabelRevealThreshold
            ? std::clamp((reveal - kLabelRevealThreshold) / (1.0f - kLabelRevealThreshold), 0.0f, 1.0f)
            : 0.0f;
    const int label_face = ctx.dark ? kFaceLight : kFaceSemiBold;
    const float label_size = theme.body.size * s;
    const float label_line = theme.body.line_height * s;

    for (int index = 0; index < total; ++index) {
        const NavItem& item =
            index < static_cast<int>(items.size()) ? items[static_cast<size_t>(index)]
                                                   : footer[static_cast<size_t>(index - items.size())];
        const Rect box_item = item_rect(index, s);
        if (box_item.w <= 0.0f) {
            continue;
        }
        if (item.selected) {
            painter.fill_round_rect(box_item, theme.radius_control * s, theme.selected_background);
        } else if (item.hovered) {
            painter.fill_round_rect(box_item, theme.radius_control * s, theme.subtle_fill_secondary);
        }

        const float icon_x = box_item.x + kIconLeftPadding * s;
        const float icon_y = box_item.y + (box_item.h - kIconSize * s) * 0.5f;
        icons::draw(painter, item.icon, icon_x, icon_y, kIconSize * s, theme.text_primary, s,
                    icons::variant_for(ctx.dark));

        if (label_alpha > 0.001f) {
            draw_text(painter, ctx.text, item.label.c_str(), label_face, label_size, label_line,
                      icon_x + (kIconSize + 12.0f) * s, box_item.y + (box_item.h - label_line) * 0.5f,
                      with_alpha(theme.text_primary, label_alpha));
        }
    }

    painter.set_clip(saved_clip);

    // 选中指示条：位置 + Squash & Stretch。
    const float target = bar_target(region.h, s);
    const bool running = bar.running(now, kBarMoveDuration);
    const float center =
        (running || target < 0.0f) ? bar.value(now, kBarMoveDuration, Theme::navigation()) : target;
    const float progress = running ? bar.linear_progress(now, kBarMoveDuration) : 0.0f;
    const_cast<NavigationView*>(this)->bar_rendered = center;

    const float stretch =
        kBarRestStretch +
        (1.0f - kBarRestStretch) * std::max(std::sin(3.14159265f * progress), 0.0f);
    const float bar_h = kBarHeight * s * (1.0f + kBarStretchMax * stretch);
    const float bar_w = kBarWidth * s * (1.0f - kBarThinMax * stretch);
    const float bar_x = rect.x + kListPadding * s;
    const float bar_y = region.y + std::round(center - bar_h * 0.5f);

    const Recti saved_clip2 = painter.clip();
    painter.set_clip(intersect(saved_clip2, cover(region)));
    painter.fill_round_rect(Rect{bar_x, bar_y, bar_w, bar_h}, bar_w * 0.45f,
                            theme.accent_fill_default);
    painter.set_clip(saved_clip2);
}

}  // namespace aether
