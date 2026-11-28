//! Fluent Design Token（C++ 版）。
//!
//! 颜色取 Fluent 亮色 / 暗色默认值，
//! 圆角对齐 `ControlCornerRadius` / `OverlayCornerRadius`，动效对齐 Fluent 标准曲线。
//! 控件只允许读取这里，禁止硬编码色值和尺寸。

#pragma once

#include <cstdint>

#include "gfx/geometry.h"

namespace aether {

/// 三次贝塞尔缓动（与 `CubicBezierEasingFunction` 对应）。
struct CubicBezier {
    float x1 = 0.0f;
    float y1 = 0.0f;
    float x2 = 0.0f;
    float y2 = 0.0f;

    static constexpr CubicBezier standard() { return CubicBezier{0.8f, 0.0f, 0.2f, 1.0f}; }
    static constexpr CubicBezier decelerate() { return CubicBezier{0.1f, 0.9f, 0.2f, 1.0f}; }
    static constexpr CubicBezier accelerate() { return CubicBezier{0.7f, 0.0f, 1.0f, 0.5f}; }

    /// 输入进度 0..1，返回缓动后的进度（x 方向二分求解）。
    float solve(float t) const;
};

struct ShadowSpec {
    Color color;
    float blur = 0.0f;
    float dx = 0.0f;
    float dy = 0.0f;

    /// Fluent 的 blur_radius 约等于 2 倍高斯标准差。
    float sigma() const { return blur * 0.5f; }
};

struct TypeStyle {
    float size = 14.0f;
    float line_height = 20.0f;
};

struct Theme {
    bool dark = true;

    // 强调色
    Color accent_fill_default;
    Color accent_fill_secondary;
    Color accent_fill_tertiary;
    Color text_on_accent;
    Color text_on_accent_primary;

    // 文本
    Color text_primary;
    Color text_secondary;
    Color text_tertiary;
    /// 文本选区底色。
    Color text_selection;
    Color text_disabled;

    // 控件填充
    Color control_fill_default;
    Color control_fill_secondary;
    Color control_fill_tertiary;
    Color control_fill_disabled;

    // 轻量填充（悬停 / 选中）
    Color subtle_fill_secondary;
    Color subtle_fill_tertiary;
    Color selected_background;

    // 描边
    Color control_stroke_default;
    Color control_stroke_secondary;
    Color control_stroke_disabled;
    Color neutral_stroke_subtle;
    Color card_stroke_default;

    // 背景
    Color card_background;
    Color card_background_secondary;
    Color layer_fill;
    Color solid_background_base;
    Color solid_background_secondary;
    Color neutral_background_1;
    Color neutral_background_2;

    // 开关
    Color toggle_thumb_stroke_off;
    Color toggle_thumb_fill_off;
    Color toggle_thumb_fill_on;
    Color toggle_track_on;
    Color toggle_track_on_focused;
    Color toggle_track_off;
    Color toggle_disabled;

    // 滚动条
    Color scrollbar_thumb;
    Color scrollbar_thumb_hover;
    Color scrollbar_track;

    // 焦点
    Color focus_stroke_outer;

    // 标题栏按钮（最小化 / 最大化 / 关闭）
    Color caption_hover;
    Color caption_pressed;
    Color close_hover;
    Color close_pressed;

    // 圆角
    float radius_control = 6.0f;
    float radius_card = 10.0f;
    float radius_overlay = 10.0f;
    float radius_small = 4.0f;
    float radius_window = 8.0f;
    float radius_text_box = 6.0f;
    float stroke_thickness = 1.0f;
    float focus_thickness = 2.0f;

    // 阴影
    ShadowSpec shadow_card;
    ShadowSpec shadow_control_rest;
    ShadowSpec shadow_text_box;

    // 动效（秒）
    float motion_fast = 0.15f;
    float motion_normal = 0.30f;
    float motion_slow = 0.50f;
    float motion_text_box_focus = 0.167f;
    CubicBezier easing = CubicBezier::standard();
    CubicBezier easing_decelerate = CubicBezier::decelerate();
    CubicBezier easing_accelerate = CubicBezier::accelerate();

    /// 输入框焦点下划线 / 下拉面板展开曲线 cubic-bezier(0, 0, 0, 1)。
    static constexpr CubicBezier focus_underline() { return CubicBezier{0.0f, 0.0f, 0.0f, 1.0f}; }

    /// 导航指示条曲线 cubic-bezier(0.9, 0, 0.1, 1)。
    static constexpr CubicBezier navigation() { return CubicBezier{0.9f, 0.0f, 0.1f, 1.0f}; }

    // 排版
    TypeStyle caption;
    TypeStyle body;
    TypeStyle body_strong;
    TypeStyle body_large;
    TypeStyle subtitle;
    TypeStyle title;
    TypeStyle title_large;

    // 材质（模拟 Mica）
    Color mica_base;
    Color mica_tint_a;
    Color mica_tint_b;

    static Theme light();
    static Theme dark_theme();
};

}  // namespace aether
