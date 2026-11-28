//! ToggleSwitch：轨道 + 史莱姆滑块，支持点击切换、按下拉伸、拖拽跟随与中点吸附。
//! 视觉与交互遵循 Fluent 开关规范。

#pragma once

#include <string>

#include "ui/widget.h"

namespace aether {

struct ToggleSwitch : Widget {
    std::string label;
    bool on = false;
    bool disabled = false;

    bool hovered = false;
    bool pressed = false;
    bool dragging = false;
    float press_x = 0.0f;
    float press_fraction = 0.0f;
    bool hover_suppressed = false;

    Anim pos = Anim::at(0.0f);
    Anim grow = Anim::at(0.0f);
    Anim hover = Anim::at(0.0f);

    /// 整个控件（轨道图片 + 标签）与轨道图片区域。
    Rect rect;
    Rect track;

    void set_default_on(bool value);

    /// 计算尺寸；`label_width` 由调用方用文本度量给出。
    void measure(float label_width, const Theme& theme, float s);
    void place(float x, float y, float s);

    // ---- 交互 ----
    void update_hover(bool inside, double now, const Theme& theme);
    void update_press(float x, double now, const Theme& theme);
    void update_drag(float x, double now, const Theme& theme);
    void update_release(double now, const Theme& theme);

    bool animating(double now) const;
    void draw(const Context& ctx) const;

    bool hit_test(float x, float y) const override { return !disabled && rect.contains(x, y); }
    bool handle_event(const Event& event, const UiContext& ui) override;
};

}  // namespace aether
