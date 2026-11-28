//! 输入事件与控件宿主：控件层只依赖这里的结构，不直接接触平台窗口。
//!
//! `Event` 是平台送来的一条原始输入；`WidgetHost` 是控件能反向请求的窗口级
//! 能力（拖动 / 最大化 / 输入法锚点），由应用层实现并注入。

#pragma once

#include <string>

#include "text/font.h"
#include "ui/theme.h"

namespace aether {

struct Event {
    enum class Type {
        None,
        Quit,
        Resize,
        MouseMove,
        MouseDown,
        MouseUp,
        Wheel,
        KeyDown,
        Text,
        /// 输入法组字过程中的实时串（为空表示组字结束）。
        Editing,
        /// 系统深浅色切换（Windows 个性化 / Linux portal / macOS 外观）。
        SystemTheme,
        /// 窗口最大化状态变化（窗口按钮之外的途径，如系统贴边吸附）。
        WindowState
    };

    Type type = Type::None;
    /// 鼠标位置，已换算成像素坐标。
    float x = 0.0f;
    float y = 0.0f;
    /// 滚轮增量（像素，正数向上）。
    float wheel = 0.0f;
    int width = 0;
    int height = 0;
    int key = 0;
    /// 修饰键状态（SDL_KMOD_* 位组合：Ctrl / Shift / Alt）。
    int mods = 0;
    std::string text;
    /// 系统当前是否为深色（仅 `SystemTheme` 事件有效）。
    bool dark = false;
    /// 窗口是否最大化（仅 `WindowState` 事件有效）。
    bool maximized = false;
};

/// 控件向宿主请求的窗口级能力；离屏截图模式下可以不提供（指针为空）。
class WidgetHost {
public:
    virtual ~WidgetHost() = default;
    virtual void request_close() {}
    virtual void minimize() {}
    virtual void toggle_maximize() {}
    virtual bool maximized() const { return false; }
    virtual void begin_drag() {}
    virtual void set_text_input_area(float x, float y, float w, float h) {
        (void)x;
        (void)y;
        (void)w;
        (void)h;
    }
    virtual void clear_text_input_area() {}
    virtual bool system_dark() const { return true; }
};

/// 事件处理上下文：一次输入所需的全部环境。
struct UiContext {
    TextRenderer& text;
    const Theme& theme;
    float s = 1.0f;
    double now = 0.0;
    bool dark = true;
    WidgetHost* host = nullptr;
};

}  // namespace aether
