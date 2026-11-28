//! ScrollView：可滚动视口，内部持有滚动条，负责裁剪、滚轮与拖动。

#pragma once

#include "ui/scroll_bar.h"
#include "ui/widget.h"

namespace aether {

class ScrollView {
public:
    Rect rect;
    ScrollBar bar;
    float scale = 1.0f;

    /// `content_h` 为内容总高度（像素）；不需要滚动时滚动条自动退化。
    void layout(float x, float y, float w, float h, float content_h, float s);

    /// 内容区（去掉右侧滚动条占位）。
    Rect content_rect() const;
    /// 当前滚动偏移（像素）。
    float offset() const { return static_cast<float>(bar.value); }

    bool hit_test(float x, float y) const;
    bool handle_event(const Event& event, const UiContext& ui);
    bool animating(double now) const { return bar.animating(now); }

    /// 设置内容裁剪，绘制完内容后用返回值恢复。
    Recti begin_clip(const Context& ctx) const;
    void end_clip(const Context& ctx, Recti saved) const;
    void draw_bar(const Context& ctx) const { bar.draw(ctx); }
};

}  // namespace aether
