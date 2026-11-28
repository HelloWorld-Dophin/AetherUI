//! 自绘标题栏：应用图标 + 标题 + 最小化 / 最大化 / 关闭。
//! 高度 40、按钮宽 56（均按 DPI 缩放），关闭按钮悬停为红色。

#pragma once

#include <string>

#include "ui/app_icon.h"
#include "ui/widget.h"

namespace aether {

struct TitleBar : Widget {
    std::string title;
    /// 左侧 logo：与窗口图标同源（内置图标名或 SVG）；为空时退回内置 SPARKLE。
    AppIcon icon;
    bool maximized = false;
    float corner_radius = 0.0f;

    bool hover_minimize = false;
    bool hover_maximize = false;
    bool hover_close = false;

    Rect bar;
    Rect btn_minimize;
    Rect btn_maximize;
    Rect btn_close;

    void layout(float x, float y, float w, float s);
    void draw(const Context& ctx) const;

    bool hit_test(float x, float y) const override { return bar.contains(x, y); }
    /// 三个窗口按钮 + 双击最大化 + 拖动起手。
    bool handle_event(const Event& event, const UiContext& ui) override;

private:
    double last_click_ = -1.0;
    float last_click_x_ = 0.0f;
};

}  // namespace aether
