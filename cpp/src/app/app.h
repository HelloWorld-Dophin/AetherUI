//! App：窗口 + 主循环 + 帧节流。
//!
//! 界面只需要提供三个回调：事件、推进、绘制。窗口拥有权、事件轮询、
//! 动画时钟与每帧限速都在这里，演示程序不再自己写 `while` 循环。
//! 窗口的标题与图标只在这里传一次，标题栏与任务栏图标都从同一来源取。

#pragma once

#include <functional>
#include <string>

#include "platform/window.h"
#include "ui/app_icon.h"

namespace aether {

class App {
public:
    /// 创建窗口：`title` 为窗口标题；`icon` 为内置 Fluent 图标名（如 `"SPARKLE"`）
    /// 或 SVG 文件（绝对路径，或 `assets/icons/` 下的相对名如 `"sparkle.svg"`）。
    bool create(const char* title, const char* icon, int width, int height);

    Window& window() { return window_; }
    const std::string& title() const { return title_; }
    const AppIcon& icon() const { return icon_; }

    /// 解析图标来源：先在 `assets/` 与 `assets/icons/` 下找同名文件（SVG），
    /// 找不到再把整串当内置图标名。离屏截图等不开窗口的入口也走这里。
    static AppIcon resolve_icon(const char* icon);

    /// 帧间隔（秒）；0 表示不限帧。
    void set_frame_interval(double seconds);

    /// 一条输入事件（`Event::Type::Quit` 由 App 自己处理）。
    std::function<void(const Event&, Window&, double)> on_event;
    /// 每帧推进（可选），在绘制之前调用。
    std::function<void(double)> on_tick;
    /// 绘制一帧。
    std::function<void(Surface&, Window&, double)> on_draw;

    /// 进入主循环，窗口关闭后返回。
    int run();

private:
    Window window_;
    std::string title_;
    AppIcon icon_;
    double frame_interval_ = 1.0 / 60.0;
};

}  // namespace aether
