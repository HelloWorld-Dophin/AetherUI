//! IconTile：图标 + 名称 + 右下角动作按钮的方形卡片（图标页 / 资源选择器通用）。

#pragma once

#include <string>

#include "ui/button.h"
#include "ui/widget.h"

namespace aether {

class IconTile : public Widget {
public:
    int icon = -1;
    std::string name;
    Button action;
    Rect rect;
    /// 是否在视口内可见（滚动出视口的卡片不参与命中）。
    bool visible = true;

    void place(float x, float y, float w, float h, float s);
    void draw(const Context& ctx) const;

    bool hit_test(float x, float y) const override { return visible && rect.contains(x, y); }
    bool handle_event(const Event& event, const UiContext& ui) override {
        if (!visible) {
            action.hovered = false;
            return false;
        }
        return action.handle_event(event, ui);
    }
};

}  // namespace aether
