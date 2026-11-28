#include "ui/text_box.h"

#include <algorithm>
#include <cmath>

#include "ui/icons.h"

namespace aether {
namespace {

constexpr float kPaddingVertical = 12.0f;
constexpr float kPaddingHorizontal = 10.0f;
constexpr float kUnderlineThickness = 3.0f;
constexpr float kChromePadding = 4.0f;
constexpr float kClearPaddingRight = 8.0f;
constexpr float kRightIconSize = 12.0f;

bool is_continuation(unsigned char byte) { return (byte & 0xC0) == 0x80; }

size_t prev_boundary(const std::string& s, size_t index) {
    if (index == 0) {
        return 0;
    }
    size_t i = index - 1;
    while (i > 0 && is_continuation(static_cast<unsigned char>(s[i]))) {
        --i;
    }
    return i;
}

size_t next_boundary(const std::string& s, size_t index) {
    if (index >= s.size()) {
        return s.size();
    }
    ++index;
    while (index < s.size() && is_continuation(static_cast<unsigned char>(s[index]))) {
        ++index;
    }
    return index;
}

/// 按词跳转：跳过空白后，跳过一个连续的「非空白块」（中文整块视为一个词）。
size_t prev_word_start(const std::string& s, size_t index) {
    size_t i = std::min(index, s.size());
    while (i > 0 && (s[i - 1] == ' ' || s[i - 1] == '\t')) {
        --i;
    }
    while (i > 0 && s[i - 1] != ' ' && s[i - 1] != '\t') {
        --i;
    }
    return i;
}

size_t next_word_end(const std::string& s, size_t index) {
    size_t i = std::min(index, s.size());
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) {
        ++i;
    }
    while (i < s.size() && s[i] != ' ' && s[i] != '\t') {
        ++i;
    }
    return i;
}

bool valid_integer(const std::string& value) {
    if (value.empty()) {
        return true;
    }
    size_t i = 0;
    if (value[0] == '-') {
        i = 1;
        if (value.size() == 1) {
            return true;
        }
    }
    for (; i < value.size(); ++i) {
        if (value[i] < '0' || value[i] > '9') {
            return false;
        }
    }
    return true;
}

}  // namespace

void TextBox::measure(float s) {
    width = (420.0f + kChromePadding * 2.0f) * s;
    height = (44.0f + kChromePadding * 2.0f) * s;
}

void TextBox::place(float x, float y) { field = Rect{x, y, width, height}; }

float TextBox::text_left(float s) const { return field.x + kPaddingHorizontal * s; }

int TextBox::selection_begin() const { return std::min(anchor, caret); }

int TextBox::selection_end() const { return std::max(anchor, caret); }

std::string TextBox::selected_text() const {
    if (!has_selection()) {
        return std::string();
    }
    const size_t begin = static_cast<size_t>(selection_begin());
    const size_t end = static_cast<size_t>(selection_end());
    return text.substr(begin, end - begin);
}

std::string TextBox::cut_selection() {
    const std::string chosen = selected_text();
    if (chosen.empty() || disabled) {
        return chosen;
    }
    coalesce_undo = false;
    push_undo();
    delete_selection();
    coalesce_undo = true;
    return chosen;
}

void TextBox::delete_selection() {
    if (!has_selection()) {
        return;
    }
    const size_t begin = static_cast<size_t>(selection_begin());
    const size_t end = static_cast<size_t>(selection_end());
    text.erase(begin, end - begin);
    caret = static_cast<int>(begin);
    anchor = caret;
}

void TextBox::select_all() {
    anchor = 0;
    caret = static_cast<int>(text.size());
}

int TextBox::index_at_x(TextRenderer& text_renderer, float x, float s, float font_size) const {
    // 文本按 -scroll 偏移绘制，换算回内容坐标。
    const float local = x - text_left(s) + scroll;
    if (local <= 0.0f) {
        return 0;
    }
    float pen = 0.0f;
    size_t i = 0;
    while (i < text.size()) {
        const size_t next = next_boundary(text, i);
        const float advance =
            text_renderer.measure(text.substr(i, next - i).c_str(), kFaceRegular, font_size);
        if (local < pen + advance * 0.5f) {
            return static_cast<int>(i);
        }
        pen += advance;
        i = next;
    }
    return static_cast<int>(text.size());
}

void TextBox::place_caret_at(TextRenderer& text_renderer, float x, float s, float font_size) {
    caret = index_at_x(text_renderer, x, s, font_size);
    anchor = caret;
}

Rect TextBox::right_icon_rect(float s) const {
    const float box_w = (kRightIconSize + 12.0f) * s;
    const float box_h = (kRightIconSize + 4.0f) * s;
    const float visible_top = field.y + kChromePadding * s;
    return Rect{field.right() - kClearPaddingRight * s - box_w, visible_top + (44.0f * s - box_h) * 0.5f,
                box_w, box_h};
}

bool TextBox::accepts(const std::string& value) const {
    if (numeric_only) {
        return valid_integer(value);
    }
    return true;
}

void TextBox::push_undo() {
    if (coalesce_undo) {
        return;
    }
    undo_stack.push_back(Snapshot{text, caret, anchor});
    if (undo_stack.size() > 64) {
        undo_stack.erase(undo_stack.begin());
    }
    redo_stack.clear();
}

void TextBox::undo() {
    if (undo_stack.empty()) {
        return;
    }
    redo_stack.push_back(Snapshot{text, caret, anchor});
    const Snapshot state = undo_stack.back();
    undo_stack.pop_back();
    text = state.text;
    caret = state.caret;
    anchor = state.anchor;
    coalesce_undo = false;
}

void TextBox::redo() {
    if (redo_stack.empty()) {
        return;
    }
    undo_stack.push_back(Snapshot{text, caret, anchor});
    const Snapshot state = redo_stack.back();
    redo_stack.pop_back();
    text = state.text;
    caret = state.caret;
    anchor = state.anchor;
    coalesce_undo = false;
}

void TextBox::insert_text(const std::string& utf8) {
    if (disabled || utf8.empty()) {
        return;
    }
    push_undo();
    if (has_selection()) {
        delete_selection();
    }
    std::string next = text;
    next.insert(std::min<size_t>(static_cast<size_t>(caret), next.size()), utf8);
    if (!accepts(next)) {
        return;
    }
    caret = static_cast<int>(std::min<size_t>(static_cast<size_t>(caret), text.size()) +
                             utf8.size());
    anchor = caret;
    text = next;
    coalesce_undo = true;
}

void TextBox::backspace() {
    if (disabled) {
        return;
    }
    if (has_selection()) {
        delete_selection();
        coalesce_undo = true;
        return;
    }
    if (caret <= 0) {
        return;
    }
    push_undo();
    const size_t from = prev_boundary(text, static_cast<size_t>(caret));
    text.erase(from, static_cast<size_t>(caret) - from);
    caret = static_cast<int>(from);
    anchor = caret;
    coalesce_undo = true;
}

void TextBox::erase_forward() {
    if (disabled) {
        return;
    }
    if (has_selection()) {
        delete_selection();
        coalesce_undo = true;
        return;
    }
    if (static_cast<size_t>(caret) >= text.size()) {
        return;
    }
    push_undo();
    const size_t to = next_boundary(text, static_cast<size_t>(caret));
    text.erase(static_cast<size_t>(caret), to - static_cast<size_t>(caret));
    anchor = caret;
    coalesce_undo = true;
}

void TextBox::move_caret(int direction) {
    coalesce_undo = false;
    if (has_selection()) {
        // 有选区时方向键先折叠选区，不再逐字移动。
        caret = direction < 0 ? selection_begin() : selection_end();
        anchor = caret;
        return;
    }
    if (direction < 0) {
        caret = static_cast<int>(prev_boundary(text, static_cast<size_t>(caret)));
    } else {
        caret = static_cast<int>(next_boundary(text, static_cast<size_t>(caret)));
    }
    anchor = caret;
}

void TextBox::move_caret_extend(int direction) {
    if (direction < 0) {
        caret = static_cast<int>(prev_boundary(text, static_cast<size_t>(caret)));
    } else {
        caret = static_cast<int>(next_boundary(text, static_cast<size_t>(caret)));
    }
}

void TextBox::move_word(int direction, bool extend) {
    coalesce_undo = false;
    if (extend) {
        caret = static_cast<int>(direction < 0 ? prev_word_start(text, static_cast<size_t>(caret))
                                               : next_word_end(text, static_cast<size_t>(caret)));
        return;
    }
    if (has_selection()) {
        caret = direction < 0 ? selection_begin() : selection_end();
        anchor = caret;
        return;
    }
    caret = static_cast<int>(direction < 0 ? prev_word_start(text, static_cast<size_t>(caret))
                                           : next_word_end(text, static_cast<size_t>(caret)));
    anchor = caret;
}

void TextBox::move_to_edge(bool to_end, bool extend) {
    coalesce_undo = false;
    caret = to_end ? static_cast<int>(text.size()) : 0;
    if (!extend) {
        anchor = caret;
    }
}

void TextBox::delete_word_backward() {
    if (disabled) {
        return;
    }
    if (has_selection()) {
        delete_selection();
        coalesce_undo = true;
        return;
    }
    if (caret <= 0) {
        return;
    }
    push_undo();
    const size_t from = prev_word_start(text, static_cast<size_t>(caret));
    text.erase(from, static_cast<size_t>(caret) - from);
    caret = static_cast<int>(from);
    anchor = caret;
    coalesce_undo = true;
}

void TextBox::delete_word_forward() {
    if (disabled) {
        return;
    }
    if (has_selection()) {
        delete_selection();
        coalesce_undo = true;
        return;
    }
    if (static_cast<size_t>(caret) >= text.size()) {
        return;
    }
    push_undo();
    const size_t to = next_word_end(text, static_cast<size_t>(caret));
    text.erase(static_cast<size_t>(caret), to - static_cast<size_t>(caret));
    anchor = caret;
    coalesce_undo = true;
}

void TextBox::clear() {
    if (text.empty()) {
        caret = 0;
        anchor = 0;
        return;
    }
    coalesce_undo = false;
    push_undo();
    text.clear();
    caret = 0;
    anchor = 0;
    coalesce_undo = true;
}

void TextBox::draw(const Context& ctx) const {
    const Theme& theme = ctx.theme;
    const float s = ctx.s;
    const double now = ctx.now;
    Painter& painter = ctx.painter;
    TextRenderer& text_renderer = ctx.text;
    const bool dark = ctx.dark;

    const float radius = theme.radius_text_box * s;
    const Rect area{field.x, field.y + kChromePadding * s, field.w, 44.0f * s};

    // ---- 底盘：底边基线阴影（裁剪到底边以下）----
    const float baseline_radius = std::max(radius, 0.0f);
    const Rect baseline{area.x + baseline_radius, std::max(area.bottom() - s, area.y),
                        area.w - baseline_radius * 2.0f, s};
    const Recti saved_clip = painter.clip();
    painter.set_clip(intersect(saved_clip, cover(Rect{area.x, area.bottom() - 0.5f * s, area.w,
                                                      kChromePadding * s})));
    painter.draw_shadow(baseline, 0.0f, theme.shadow_text_box.color, theme.shadow_text_box.sigma() * s,
                        0.0f, theme.shadow_text_box.dy * s);
    painter.set_clip(saved_clip);

    // ---- 填充 + 四周细轮廓 ----
    painter.fill_round_rect(area, radius,
                            disabled ? theme.control_fill_disabled : theme.neutral_background_1);
    painter.stroke_round_rect(area, radius, theme.stroke_thickness * s,
                              disabled ? theme.control_stroke_disabled
                                       : theme.neutral_stroke_subtle);

    // ---- 底边横向基线 ----
    painter.fill_rect(baseline, disabled ? theme.control_stroke_disabled
                                         : theme.control_stroke_secondary);

    // ---- 焦点高亮层（被圆角裁剪）----
    if (focused && !disabled) {
        const Rect band{area.x, area.bottom() - kUnderlineThickness * s, area.w,
                        kUnderlineThickness * s};
        painter.fill_round_rect_clip(area, radius, band, theme.accent_fill_default);
    }

    // ---- 文本 / 占位符 / 光标 ----
    const int face = kFaceRegular;
    const float font_size = theme.body.size * s;
    const float line_height = theme.body.line_height * s;
    const float text_x = area.x + kPaddingHorizontal * s;
    const float text_y = area.y + (area.h - line_height) * 0.5f;
    const bool show_placeholder = !focused && text.empty();
    const bool show_icon = !disabled && right_icon >= 0 &&
                           (right_icon_custom || (focused && !text.empty()));

    // 可视文本区：右侧为清除按钮让位，超出部分靠水平滚动查看。
    const float visible_right =
        show_icon ? right_icon_rect(s).x - 2.0f * s : area.right() - kPaddingHorizontal * s;
    const float visible_w = std::max(visible_right - text_x, 8.0f * s);
    const size_t caret_index =
        std::min<size_t>(static_cast<size_t>(std::max(caret, 0)), text.size());
    const std::string caret_prefix = text.substr(0, caret_index);
    const float caret_local = text_width(text_renderer, caret_prefix.c_str(), face, font_size);

    // 让光标始终留在可视区内：超出右边界则右移内容，左移则回滚。
    if (text.empty()) {
        scroll = 0.0f;
    } else {
        const float edge = 2.0f * s;
        if (caret_local - scroll > visible_w - edge) {
            scroll = caret_local - (visible_w - edge);
        }
        if (caret_local < scroll) {
            scroll = caret_local;
        }
        const float content_w = text_width(text_renderer, text.c_str(), face, font_size);
        scroll = std::clamp(scroll, 0.0f, std::max(0.0f, content_w - visible_w + edge));
    }

    // 文本、选区与光标统一裁剪在可视区内。
    const Recti text_clip = intersect(
        saved_clip, cover(Rect{text_x, area.y + 2.0f * s, visible_w, area.h - 4.0f * s}));
    painter.set_clip(text_clip);

    // ---- 选区底色（在文字之下）----
    if (has_selection()) {
        const size_t begin = static_cast<size_t>(selection_begin());
        const size_t end = static_cast<size_t>(selection_end());
        const std::string before = text.substr(0, begin);
        const std::string chosen = text.substr(begin, end - begin);
        const float x0 =
            text_x + text_width(text_renderer, before.c_str(), face, font_size) - scroll;
        const float x1 = x0 + text_width(text_renderer, chosen.c_str(), face, font_size);
        painter.fill_rect(Rect{x0, area.y + kPaddingVertical * s, std::max(x1 - x0, s),
                               area.h - kPaddingVertical * 2.0f * s},
                          theme.text_selection);
    }

    if (show_placeholder) {
        if (!placeholder.empty()) {
            draw_text(painter, text_renderer, placeholder.c_str(), face, font_size, line_height,
                      text_x, text_y, disabled ? theme.text_disabled : theme.text_secondary);
        }
    } else {
        draw_text(painter, text_renderer, text.c_str(), face, font_size, line_height,
                  text_x - scroll, text_y,
                  disabled ? theme.text_disabled : theme.text_primary);

        if (focused && !has_selection() && std::fmod(now, 1.06) < 0.53) {
            painter.fill_rect(Rect{text_x + caret_local - scroll, area.y + kPaddingVertical * s,
                                   std::max(s, 1.0f), area.h - kPaddingVertical * 2.0f},
                              theme.text_primary);
        }
    }
    painter.set_clip(saved_clip);

    // ---- 右侧图标 ----
    if (show_icon) {
        const Rect box = right_icon_rect(s);
        if (right_hovered) {
            painter.fill_round_rect(box, theme.radius_small * s, theme.subtle_fill_secondary);
        }
        icons::draw(painter, right_icon, box.x + (box.w - kRightIconSize * s) * 0.5f,
                    box.y + (box.h - kRightIconSize * s) * 0.5f, kRightIconSize * s,
                    theme.text_secondary, s, icons::variant_for(dark));
    }
}

}  // namespace aether
