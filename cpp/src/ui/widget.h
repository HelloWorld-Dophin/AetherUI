//! 控件公共设施：数值动画、交互状态机、文本绘制助手。
//!
//! 动画与状态机：
//! 动画以「起点 / 终点 / 起始时刻」描述，状态机只做命中判定与状态推导，
//! 颜色一律由调用方从主题 Token 读取。

#pragma once

#include <string>

#include "gfx/geometry.h"
#include "gfx/painter.h"
#include "text/font.h"
#include "ui/theme.h"

namespace aether {

/// 字体栈下标（与 main.cpp 加载顺序一致）。
enum FaceIndex {
    kFaceRegular = 0,
    kFaceLight = 1,
    kFaceSemiBold = 2,
    kFaceSymbols = 3,
    kFaceEmoji = 4,
    kFaceMono = 5,
    kFaceMonoSemiBold = 6,
};

/// 简单数值动画（167ms、cubic-bezier(0, 0, 0, 1) 为默认观感）。
struct Anim {
    float from = 0.0f;
    float to = 0.0f;
    double started = -1.0;

    static Anim at(float value);
    float value(double now, float duration, const CubicBezier& curve) const;
    void retarget(float target, double now, float duration, const CubicBezier& curve);
    void snap(float target);
    bool running(double now, float duration) const;
    float linear_progress(double now, float duration) const;
};

/// 控件视觉状态。
enum class ControlState { Normal, PointerOver, Focused, Pressed, Disabled };

/// 简易交互状态机：只负责命中 / 按下，不持有绘制数据。
struct Interaction {
    bool enabled = true;
    bool hovered = false;
    bool pressed = false;

    bool hover(float x, float y, Rect bounds);
    void press(bool down);
    void leave();
    ControlState state() const;
};

/// 文本排版助手：按字号 / 行高 / 换行宽度生成布局。
TextLayout make_text(TextRenderer& text, const char* utf8, int face, float size, float line_height,
                     float max_width = 0.0f, int align = 0);

/// 单行文本的便捷绘制（返回实际宽度）。
float draw_text(Painter& painter, TextRenderer& text, const char* utf8, int face, float size,
                float line_height, float x, float y, Color color);

/// 单行文本宽度。
float text_width(TextRenderer& text, const char* utf8, int face, float size);

/// 绘制上下文：一次绘制所需的全部环境。
struct Context {
    Painter& painter;
    TextRenderer& text;
    const Theme& theme;
    float s = 1.0f;
    double now = 0.0;
    bool dark = true;
};

}  // namespace aether
