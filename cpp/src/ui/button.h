//! Button：Primary / Standard 两种外观，可选前置图标，四态 + 自动内容尺寸。
//! 尺寸与配色遵循 Fluent 按钮规范。

#pragma once

#include <string>

#include "ui/widget.h"

namespace aether {

struct Button {
    std::string label;
    /// 图标下标（icons::index_of 的结果），-1 表示无图标。
    int icon = -1;
    /// true = Primary（AccentFill 背景），false = Standard。
    bool primary = false;
    bool disabled = false;
    /// 正方形纯图标按钮：宽高取同一值。
    bool compact = false;

    Rect rect;

    bool hovered = false;
    bool pressed = false;

    /// 计算尺寸（rect.w / rect.h）。
    void measure(TextRenderer& text, const Theme& theme, float s);
    void place(float x, float y);
    void draw(const Context& ctx) const;
};

}  // namespace aether
