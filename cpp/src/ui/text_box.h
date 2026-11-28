//! TextBox：圆角 6 / 内边距 12·10 / 底边基线与焦点高亮的输入框。
//! 底盘与输入行为都在本文件内维护。

#pragma once

#include <string>
#include <vector>

#include "ui/widget.h"

namespace aether {

struct TextBox {
    std::string text;
    std::string placeholder;
    bool disabled = false;
    /// 仅允许 0-9 与开头一个负号。
    bool numeric_only = false;
    /// 右侧图标下标：默认 DISMISS，可换成自定义图标。
    int right_icon = -1;
    bool right_icon_custom = false;

    bool hovered = false;
    bool focused = false;
    bool pressed = false;
    bool right_hovered = false;

    /// 布局盒（含底盘 4px 阴影边距），可视区域在其中内缩 4px。
    Rect field;
    float width = 428.0f;
    float height = 52.0f;

    /// 光标位置（字节偏移）。
    int caret = 0;
    /// 选区锚点：与 caret 不同即表示存在选区（鼠标拖拽时固定不动）。
    int anchor = 0;
    /// 自定义右侧图标点击次数（演示回调）。
    int right_clicks = 0;

    void measure(float s);
    void place(float x, float y);
    Rect right_icon_rect(float s) const;
    /// 文本绘制起点（未缩放坐标需乘 s）。
    float text_left(float s) const;
    /// 水平滚动偏移（像素）：内容超出可视宽度时由 `draw` 按光标位置自动调整。
    mutable float scroll = 0.0f;

    bool accepts(const std::string& value) const;
    void insert_text(const std::string& utf8);
    void backspace();
    void erase_forward();
    void move_caret(int direction);
    void clear();

    // ---- 选区 ----
    bool has_selection() const { return anchor != caret; }
    int selection_begin() const;
    int selection_end() const;
    std::string selected_text() const;
    /// 删除选区内容并把光标落到选区起点。
    void delete_selection();
    void select_all();
    /// 按鼠标横坐标（屏幕像素）换算光标字节偏移。
    int index_at_x(TextRenderer& text, float x, float s, float font_size) const;
    /// 点击定位：锚点与光标同时落到 index。
    void place_caret_at(TextRenderer& text, float x, float s, float font_size);

    // ---- 键盘导航（Shift 扩选 / Ctrl 按词）----
    void move_caret_extend(int direction);
    void move_word(int direction, bool extend);
    void move_to_edge(bool to_end, bool extend);
    void delete_word_backward();
    void delete_word_forward();

    // ---- 撤销 / 重做 ----
    void undo();
    void redo();
    void push_undo();
    /// 剪切选区：返回被剪切的内容（可写入剪贴板）。
    std::string cut_selection();

    struct Snapshot {
        std::string text;
        int caret = 0;
        int anchor = 0;
    };
    std::vector<Snapshot> undo_stack;
    std::vector<Snapshot> redo_stack;
    /// 连续输入/删除合并为一次撤销步骤。
    bool coalesce_undo = false;

    void draw(const Context& ctx) const;
};

}  // namespace aether
