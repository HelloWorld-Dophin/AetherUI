//! Label：按主题排版等级渲染的单行 / 自动换行文本；PageHeader 是「标题 + 说明」组合。

#pragma once

#include <string>

#include "ui/widget.h"

namespace aether {

struct Label {
    std::string text;
    int face = kFaceRegular;
    TextStyle style = TextStyle::Body;
    TextRole role = TextRole::Primary;
    /// 覆盖排版等级的字号 / 行高（逻辑点，> 0 时生效）。
    float size = 0.0f;
    float line_height = 0.0f;
    /// 按 `layout` 给定的宽度自动换行。
    bool wrap = false;
    /// 在盒内水平居中（否则左对齐）。
    bool center = false;

    Rect rect;

    /// 计算盒子的位置与尺寸；`width` 为 0 时按文本宽度收缩。
    void layout(TextRenderer& text_renderer, const Theme& theme, float s, float x, float y,
                float width);
    void draw(const Context& ctx) const;
};

/// 页头：标题 + 一行说明。
struct PageHeader {
    Label title;
    Label caption;

    void set(const std::string& title_text, const std::string& caption_text);
    void layout(TextRenderer& text_renderer, const Theme& theme, float s, float x, float y,
                float width);
    float bottom() const { return caption.rect.bottom(); }
    void draw(const Context& ctx) const;
};

}  // namespace aether
