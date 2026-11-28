//! 控件公共设施：控件基类、事件转发、数值动画、交互状态机、文本绘制助手。
//!
//! 所有交互控件都从 `Widget` 派生：自己命中、自己处理事件，应用层只负责
//! 「摆放 + 转发 + 绘制」。颜色一律从主题 Token 读取。

#pragma once

#include <iterator>
#include <string>

#include "gfx/geometry.h"
#include "gfx/painter.h"
#include "text/font.h"
#include "ui/input.h"
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

/// 主题里的排版等级（字号 / 行高）。
enum class TextStyle { Caption, Body, BodyStrong, BodyLarge, Subtitle, Title, TitleLarge };

/// 文本颜色角色。
enum class TextRole { Primary, Secondary, Tertiary, Accent, Disabled };

const TypeStyle& type_style(const Theme& theme, TextStyle style);
Color role_color(const Theme& theme, TextRole role);

/// 控件基类：命中测试 + 事件处理 + 焦点。
class Widget {
public:
    virtual ~Widget() = default;

    virtual bool hit_test(float x, float y) const {
        (void)x;
        (void)y;
        return false;
    }
    /// 处理一条事件；返回 true 表示已消费（不再传给下层控件）。
    virtual bool handle_event(const Event& event, const UiContext& ui) {
        (void)event;
        (void)ui;
        return false;
    }
    /// 是否参与焦点轮转（Tab 键），不可用状态下返回 false。
    virtual bool focusable() const { return false; }
    virtual bool has_focus() const { return false; }
    virtual void set_focus(bool value) { (void)value; }
    /// 把光标位置上报给输入法，只有文本类控件需要实现。
    virtual void sync_ime(const UiContext& ui) const { (void)ui; }
};

/// 指针类事件（键盘 / 文本 / 组字由焦点管理器独占，不参与批量转发）。
inline bool is_pointer_event(const Event& event) {
    switch (event.type) {
        case Event::Type::MouseMove:
        case Event::Type::MouseDown:
        case Event::Type::MouseUp:
        case Event::Type::Wheel:
            return true;
        default:
            return false;
    }
}

/// 把事件转发给一组控件：移动事件广播，其余自后向前、首个消费即止。
template <typename Collection>
bool forward_events(Collection& widgets, const Event& event, const UiContext& ui) {
    if (event.type == Event::Type::MouseMove) {
        for (auto& widget : widgets) {
            widget.handle_event(event, ui);
        }
        return false;
    }
    for (auto it = std::rbegin(widgets); it != std::rend(widgets); ++it) {
        if (it->handle_event(event, ui)) {
            return true;
        }
    }
    return false;
}

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
