//! ThemeManager：跟随系统 / 强制浅色 / 强制深色，明暗与 token 的唯一来源。

#pragma once

#include "ui/input.h"
#include "ui/theme.h"

namespace aether {

class ThemeManager {
public:
    /// 0 跟随系统 / 1 浅色 / 2 深色。
    static constexpr int kFollowSystem = 0;
    static constexpr int kLight = 1;
    static constexpr int kDark = 2;

    void set_system_dark(bool dark);
    void set_preference(int preference);
    int preference() const { return preference_; }
    /// 明暗互换并写回显式偏好。
    void toggle();

    const Theme& theme() const { return theme_; }
    bool dark() const { return theme_.dark; }
    bool follow_system() const { return preference_ == kFollowSystem; }

    /// 处理系统深浅色事件；返回主题是否发生变化。
    bool handle_event(const Event& event);

private:
    void apply();

    Theme theme_ = Theme::dark_theme();
    bool system_dark_ = true;
    int preference_ = kFollowSystem;
};

}  // namespace aether
