//! NavigationView：可折叠侧边导航栏。
//! 导航视图：
//! 收起 56px / 展开 240px、汉堡横线按压形变、选中指示条 500ms 冲刺 + Squash & Stretch。

#pragma once

#include <string>
#include <vector>

#include "ui/widget.h"

namespace aether {

struct NavItem {
    std::string label;
    int icon = -1;
    bool hovered = false;
    bool selected = false;
};

struct NavigationView {
    std::vector<NavItem> items;
    std::vector<NavItem> footer;
    int selected = 0;

    bool expanded = true;
    Anim width = Anim::at(240.0f);
    Anim hamburger = Anim::at(1.0f);
    bool hamburger_hovered = false;
    Anim bar = Anim::at(22.0f);
    bool scrollbar = true;
    float corner_radius = 0.0f;

    /// 整个侧栏、汉堡按钮、条目区域。
    Rect rect;
    Rect hamburger_rect;
    Rect region;

    float bar_rendered = 0.0f;
    bool bar_ready = false;
    /// 列表中最近一次的滚动偏移（此处列表不产生滚动，保留接口）。
    float scroll_offset = 0.0f;

    NavigationView();

    void set_expanded(bool value, double now, const Theme& theme);
    void toggle_expanded(double now, const Theme& theme);
    void set_selected(int index, double now, float s);
    void press_feedback(double now);
    void pointer_up(double now);
    bool animating(double now, const Theme& theme) const;

    void layout(float x, float y, float w, float h, float s);
    Rect item_rect(int index, float s) const;

    /// 命中条目下标（含底部条目），未命中返回 -1。
    int item_at(float x, float y, float s) const;
    bool hamburger_at(float x, float y) const { return hamburger_rect.contains(x, y); }

    void draw(const Context& ctx) const;

private:
    float nav_width(double now, const Theme& theme) const;
    float footer_height(float s) const;
    float main_bottom(float s) const;
    float footer_top(float region_h, float s) const;
    float bar_target(float region_h, float s) const;
    void start_bar_flow(double now, float s);
};

}  // namespace aether
