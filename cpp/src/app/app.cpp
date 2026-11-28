#include "app/app.h"

#include <algorithm>
#include <vector>

#include <SDL3/SDL.h>

#include "ui/theme.h"

namespace aether {
namespace {

/// 原生窗口图标边长（任务栏会自动缩放）。
constexpr int kWindowIconSize = 64;

double now_seconds() {
    static const double freq = static_cast<double>(SDL_GetPerformanceFrequency());
    return static_cast<double>(SDL_GetPerformanceCounter()) / freq;
}

}  // namespace

AppIcon App::resolve_icon(const char* icon) {
    const std::string source = icon != nullptr ? icon : "";
    std::string resolved = find_asset(source);
    if (resolved.empty()) {
        resolved = find_asset("icons/" + source);
    }
    return AppIcon(resolved.empty() ? source : resolved);
}

bool App::create(const char* title, const char* icon, int width, int height) {
    title_ = title != nullptr ? title : "";
    if (!window_.create(title_.c_str(), width, height)) {
        return false;
    }
    icon_ = resolve_icon(icon);

    // 任务栏 / Alt-Tab 图标：与标题栏 logo 同源，颜色取主题强调色。
    const Theme theme = window_.system_dark() ? Theme::dark_theme() : Theme::light();
    std::vector<uint32_t> argb;
    if (icon_.rasterize(kWindowIconSize, theme.accent_fill_default, argb)) {
        window_.set_icon(argb.data(), kWindowIconSize, kWindowIconSize);
    }
    return true;
}

void App::set_frame_interval(double seconds) { frame_interval_ = std::max(seconds, 0.0); }

int App::run() {
    bool running = true;
    double clock = 0.0;
    double last_frame = now_seconds();

    while (running) {
        const double frame_begin = now_seconds();
        // 拖动窗口 / 断点暂停后时间差可能是几秒，卡住上限避免动画瞬移。
        const double dt = std::clamp(frame_begin - last_frame, 0.0, 0.1);
        last_frame = frame_begin;
        clock += dt;

        Event event;
        while (window_.poll(event)) {
            if (event.type == Event::Type::Quit) {
                running = false;
                break;
            }
            if (on_event) {
                on_event(event, window_, clock);
            }
        }
        if (!running) {
            break;
        }

        if (on_tick) {
            on_tick(clock);
        }

        Surface surface = window_.begin_frame();
        if (!surface.valid()) {
            // 最小化等情况下没有可写的表面，让出 CPU 等下一帧。
            SDL_Delay(4);
            continue;
        }
        if (on_draw) {
            on_draw(surface, window_, clock);
        }
        window_.end_frame();

        if (frame_interval_ > 0.0) {
            const double spent = now_seconds() - frame_begin;
            if (spent < frame_interval_) {
                SDL_Delay(static_cast<uint32_t>((frame_interval_ - spent) * 1000.0));
            }
        }
    }
    return 0;
}

}  // namespace aether
