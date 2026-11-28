//! ScrollBar：只负责「数值 + 外观 + 交互」的滚动条。
//! 12px 占位厚度、6px 可视厚度、悬浮渐显轨道。

#pragma once

#include "ui/widget.h"

namespace aether {

struct ScrollBar {
    double min = 0.0;
    double max = 1.0;
    double value = 0.0;
    double page_size = 0.0;

    bool disabled = false;
    bool hovered = false;
    bool dragging = false;
    float grab = 0.0f;
    float extent = 0.0f;

    Anim hover = Anim::at(0.0f);
    Anim press = Anim::at(0.0f);

    Rect rect;
    float scale = 1.0f;

    void layout(float x, float y, float w, float h, float s);
    void set_range(double lo, double hi);
    void set_page_size(double size);
    void set_value(double v);

    void press_at(float position, double now, const Theme& theme);
    void drag_to(float position);
    void release(double now, const Theme& theme);
    void wheel(float delta);
    /// 悬停 / 按下 / 拖动 / 抬起（`event.y` 为窗口像素坐标）。
    bool handle_event(const Event& event, const UiContext& ui);
    bool animating(double now) const;

    void draw(const Context& ctx) const;

private:
    float track_length() const;
    float thumb_length() const;
    float thumb_start() const;
    void apply(double v);
};

}  // namespace aether
