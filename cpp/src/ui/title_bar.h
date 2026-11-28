//! 自绘标题栏：Flash 图标 + 标题 + 最小化 / 最大化 / 关闭。
//! 高度 40、按钮宽 46、关闭按钮悬停为红色。

#pragma once

#include <string>

#include "ui/widget.h"

namespace aether {

struct TitleBar {
    std::string title;
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
};

}  // namespace aether
