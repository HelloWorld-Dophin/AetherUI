//! Dropdown：闭合框复用输入框底盘 + 右侧箭头，悬浮面板可上下 / 居中展开。
//! 尺寸与配色遵循 Fluent 下拉框规范。

#pragma once

#include <string>
#include <vector>

#include "ui/widget.h"

namespace aether {

struct DropdownOption {
    int icon = -1;
    std::string label;
    bool disabled = false;
};

enum class PanelPlacement { Above, Center, Below };

struct Dropdown {
    std::vector<DropdownOption> items;
    int selected = -1;
    bool disabled = false;
    bool open = false;
    Anim panel = Anim::at(0.0f);
    int hover_item = -1;
    PanelPlacement placement = PanelPlacement::Center;

    /// 可视宽度；0 表示按选中文本自动适应。
    float width = 280.0f;
    Rect field;

    /// 最近一次布局的闭合框顶部，用于外部点击判定。
    float last_field_top = -1.0f;

    void measure(TextRenderer& text, const Theme& theme, float s);
    void place(float x, float y);
    void toggle(double now, const Theme& theme);
    void close(double now, const Theme& theme);
    void select(int index, double now, const Theme& theme);
    bool panel_visible(double now) const;
    bool animating(double now) const;

    float panel_height(const Theme& theme, float s) const;
    float panel_top(float field_top, const Theme& theme, float s) const;
    /// 闭合框布局盒宽度（含左右各 4px 底盘边距），面板与之等宽。
    float chrome_width() const { return field.w; }

    const DropdownOption* selected_item() const;

    void draw_field(const Context& ctx) const;
    /// 绘制悬浮面板，`top` 为面板顶部（内容区坐标）。
    void draw_panel(const Context& ctx, float top) const;
};

}  // namespace aether
