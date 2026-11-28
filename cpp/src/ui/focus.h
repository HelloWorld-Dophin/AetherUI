//! FocusManager：同一时刻至多一个控件持有焦点；点击空白清除，Tab 依次轮转。

#pragma once

#include <vector>

#include "ui/widget.h"

namespace aether {

class FocusManager {
public:
    void add(Widget* widget);
    void clear();

    /// 鼠标按下：命中可聚焦控件时交接焦点，否则清除焦点。
    void press(const Event& event);
    /// 键盘 / 文本 / 组字事件：只交给当前焦点控件；Tab 在控件间轮转。
    bool handle_event(const Event& event, const UiContext& ui);
    /// 把光标位置上报给输入法（没有焦点控件时清空锚点）。
    void sync_ime(const UiContext& ui) const;

    Widget* current() const { return current_; }

private:
    void focus_on(Widget* widget);
    void cycle(int direction);

    std::vector<Widget*> widgets_;
    Widget* current_ = nullptr;
};

}  // namespace aether
