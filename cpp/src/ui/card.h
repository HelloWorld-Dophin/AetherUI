//! Card：圆角卡片底盘（填充 + 1px 描边），内容由调用方组合。

#pragma once

#include "ui/widget.h"

namespace aether {

struct Card {
    Rect rect;

    void place(float x, float y, float w, float h);
    void draw(const Context& ctx) const;
};

}  // namespace aether
