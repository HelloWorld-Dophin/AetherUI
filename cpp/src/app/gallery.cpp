#include "app/gallery.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include <SDL3/SDL.h>

#include "ui/icons.h"

namespace aether {
namespace {

constexpr float kPagePadding = 32.0f;
constexpr float kTitleHeight = 40.0f;
constexpr float kCaptionHeight = 20.0f;
constexpr float kPageGap = 16.0f;

constexpr int kDropdownColumns = 2;
constexpr int kPlacementColumns = 2;

constexpr int kIconColumns = 3;
constexpr float kIconsViewportHeight = 512.0f;
constexpr float kIconsRowHeight = 96.0f;
constexpr float kIconsRowGap = 12.0f;
constexpr float kIconsCopySize = 32.0f;
constexpr float kIconCellHeight = 88.0f;

constexpr float kTitleBarHeight = 40.0f;
constexpr float kDropdownChromeHeight = 52.0f;

struct FontCard {
    const char* name;
    const char* family;
    int face;
    const char* sample;
};

const FontCard kFontCards[] = {
    {"更纱黑体 Light", "Sarasa UI SC", kFaceLight, "AetherUI 字体展示 · 0123 · The quick brown fox"},
    {"更纱黑体 Regular", "Sarasa UI SC", kFaceRegular, "AetherUI 字体展示 · 0123 · The quick brown fox"},
    {"更纱黑体 SemiBold", "Sarasa UI SC", kFaceSemiBold, "AetherUI 字体展示 · 0123 · The quick brown fox"},
    {"JetBrains Mono Regular", "JetBrains Mono", kFaceMono, "fn main() { println!(\"AetherUI\"); }"},
    {"JetBrains Mono SemiBold", "JetBrains Mono", kFaceMonoSemiBold,
     "fn main() { println!(\"AetherUI\"); }"},
    {"Noto Sans Symbols 2", "Noto Sans Symbols 2", kFaceSymbols, "→ ✓ ★ ♥ ⚙ ⌘ ⚡ ✉"},
    {"Noto Emoji", "Noto Emoji", kFaceEmoji, "😀 🚀 ❤️ 🌏 🔧 ✨"},
};

/// 网格中第 index 个下拉框闭合框的左上角。
Point grid_origin(int index, int columns, Point origin, float gap) {
    const float field_w = 280.0f + 8.0f;
    const float field_h = kDropdownChromeHeight;
    const int col = index % std::max(columns, 1);
    const int row = index / std::max(columns, 1);
    return Point{origin.x + static_cast<float>(col) * (field_w + gap),
                 origin.y + static_cast<float>(row) * (field_h + gap)};
}

void fill_centered_text(Painter& painter, TextRenderer& text, const char* value, int face,
                        float size, float line_height, const Rect& box, Color color) {
    const float y = box.y + (box.h - line_height) * 0.5f;
    draw_text(painter, text, value, face, size, line_height, box.x, y, color);
}

void copy_to_clipboard(const std::string& value) {
    if (!value.empty()) {
        SDL_SetClipboardText(value.c_str());
    }
}

/// 粘贴：单行输入框去掉换行与制表符。
void paste_from_clipboard(TextBox& box) {
    char* raw = SDL_GetClipboardText();
    if (raw == nullptr) {
        return;
    }
    std::string single;
    for (const char* p = raw; *p != '\0'; ++p) {
        if (*p != '\n' && *p != '\r' && *p != '\t') {
            single += *p;
        }
    }
    SDL_free(raw);
    box.insert_text(single);
}

/// 输入框键盘快捷键：Ctrl+A/C/X/V/Z/Y、Ctrl+方向、Home/End、Shift 扩选。
void handle_text_shortcut(TextBox& box, const Event& event) {
    const bool ctrl = (event.mods & SDL_KMOD_CTRL) != 0;
    const bool shift = (event.mods & SDL_KMOD_SHIFT) != 0;

    if (ctrl) {
        switch (event.key) {
            case SDLK_A:
                box.select_all();
                return;
            case SDLK_C:
                copy_to_clipboard(box.selected_text());
                return;
            case SDLK_X:
                copy_to_clipboard(box.cut_selection());
                return;
            case SDLK_V:
                paste_from_clipboard(box);
                return;
            case SDLK_Z:
                if (shift) {
                    box.redo();
                } else {
                    box.undo();
                }
                return;
            case SDLK_Y:
                box.redo();
                return;
            case SDLK_BACKSPACE:
                box.delete_word_backward();
                return;
            case SDLK_DELETE:
                box.delete_word_forward();
                return;
            case SDLK_LEFT:
                box.move_word(-1, shift);
                return;
            case SDLK_RIGHT:
                box.move_word(1, shift);
                return;
            case SDLK_HOME:
                box.move_to_edge(false, shift);
                return;
            case SDLK_END:
                box.move_to_edge(true, shift);
                return;
            default:
                return;
        }
    }

    switch (event.key) {
        case SDLK_BACKSPACE:
            box.backspace();
            break;
        case SDLK_DELETE:
            box.erase_forward();
            break;
        case SDLK_LEFT:
            if (shift) {
                box.move_caret_extend(-1);
            } else {
                box.move_caret(-1);
            }
            break;
        case SDLK_RIGHT:
            if (shift) {
                box.move_caret_extend(1);
            } else {
                box.move_caret(1);
            }
            break;
        case SDLK_HOME:
            box.move_to_edge(false, shift);
            break;
        case SDLK_END:
            box.move_to_edge(true, shift);
            break;
        default:
            break;
    }
}

}  // namespace

Gallery::Gallery(TextRenderer& text) : text_(text) {
    theme_ = Theme::dark_theme();
    build_widgets();
}

void Gallery::build_widgets() {
    icon_home_ = icons::index_of("HOME");
    icon_cursor_ = icons::index_of("CURSOR");
    icon_control_button_ = icons::index_of("CONTROL_BUTTON");
    icon_chevron_down_ = icons::index_of("CHEVRON_DOWN");
    icon_toggle_ = icons::index_of("TOGGLE");
    icon_text_font_ = icons::index_of("TEXT_FONT");
    icon_grid_ = icons::index_of("GRID");
    icon_info_ = icons::index_of("INFO");
    icon_copy_ = icons::index_of("COPY");
    icon_dismiss_ = icons::index_of("DISMISS");

    // ---- 导航：7 个主条目 + 「关于」固定底部 ----
    const struct {
        const char* label;
        int icon;
    } nav_specs[8] = {
        {"主页", icon_home_},       {"输入框", icon_cursor_},
        {"按钮", icon_control_button_}, {"下拉框", icon_chevron_down_},
        {"开关", icon_toggle_},     {"字体", icon_text_font_},
        {"图标", icon_grid_},       {"关于", icon_info_},
    };
    for (int i = 0; i < 7; ++i) {
        NavItem item;
        item.label = nav_specs[i].label;
        item.icon = nav_specs[i].icon;
        nav_.items.push_back(item);
    }
    NavItem about;
    about.label = nav_specs[7].label;
    about.icon = nav_specs[7].icon;
    nav_.footer.push_back(about);
    nav_.items[0].selected = true;
    nav_.scrollbar = true;

    // ---- 输入框 ----
    text_box_.placeholder = "请输入文本";
    numeric_box_.placeholder = "仅数字，可带一个前导负号";
    numeric_box_.numeric_only = true;
    disabled_box_.placeholder = "禁用状态";
    disabled_box_.disabled = true;
    icon_box_.placeholder = "右侧为自定义图标，点击触发回调";
    icon_box_.right_icon = icon_info_;
    icon_box_.right_icon_custom = true;
    text_box_.right_icon = icon_dismiss_;
    numeric_box_.right_icon = icon_dismiss_;
    disabled_box_.right_icon = icon_dismiss_;
    icon_box_.right_clicks = 0;

    // ---- 下拉框：四种组合 ----
    const struct {
        int icon;
        const char* label;
        bool disabled;
        bool selected;
        PanelPlacement placement;
    } dropdown_specs[4] = {
        {icon_home_, "启用 · 带图标", false, true, PanelPlacement::Center},
        {icon_control_button_, "禁用 · 带图标", true, false, PanelPlacement::Below},
        {icon_info_, "启用 · 无图标", false, true, PanelPlacement::Center},
        {-1, "禁用 · 无图标", true, false, PanelPlacement::Above},
    };
    dropdowns_.clear();
    for (const auto& spec : dropdown_specs) {
        Dropdown dropdown;
        DropdownOption option;
        option.icon = spec.icon;
        option.label = spec.label;
        option.disabled = spec.disabled;
        dropdown.items.push_back(option);
        if (spec.selected) {
            dropdown.selected = 0;
        }
        dropdown.placement = spec.placement;
        dropdown.width = 280.0f;
        dropdowns_.push_back(dropdown);
    }
    dropdown_notice_ = "启用 · 带图标";

    // ---- 面板位置演示：上方 / 居中 / 下方 ----
    placement_dropdowns_.clear();
    const struct {
        PanelPlacement placement;
        const char* name;
    } placement_specs[3] = {
        {PanelPlacement::Above, "上方"},
        {PanelPlacement::Center, "居中"},
        {PanelPlacement::Below, "下方"},
    };
    for (const auto& spec : placement_specs) {
        Dropdown dropdown;
        for (int n = 1; n <= 3; ++n) {
            DropdownOption option;
            option.label = std::string(spec.name) + " · 选项 " + std::to_string(n);
            dropdown.items.push_back(option);
        }
        dropdown.selected = 0;
        dropdown.placement = spec.placement;
        dropdown.width = 280.0f;
        placement_dropdowns_.push_back(dropdown);
    }

    // ---- 按钮：Primary / Standard 各四个 ----
    const struct {
        bool primary;
        int icon;
        const char* label;
        bool disabled;
    } button_specs[8] = {
        {true, icon_control_button_, "", false},
        {true, -1, "主要文字", false},
        {true, icon_control_button_, "主要按钮", false},
        {true, icon_control_button_, "禁用主要", true},
        {false, icon_control_button_, "", false},
        {false, -1, "标准文字", false},
        {false, icon_control_button_, "标准按钮", false},
        {false, icon_control_button_, "禁用标准", true},
    };
    buttons_.clear();
    for (const auto& spec : button_specs) {
        Button button;
        button.label = spec.label;
        button.icon = spec.icon;
        button.primary = spec.primary;
        button.disabled = spec.disabled;
        button.measure(text_, theme_, 1.0f);
        buttons_.push_back(button);
    }

    // ---- 图标页复制按钮 ----
    icon_buttons_.clear();
    icon_buttons_.resize(static_cast<size_t>(icons::kIconEntryCount));
    for (Button& button : icon_buttons_) {
        button.icon = icon_copy_;
        button.compact = true;
        button.measure(text_, theme_, 1.0f);
    }

    // ---- 开关 ----
    toggles_[0].label = "启用状态";
    toggles_[0].set_default_on(true);
    toggles_[1].label = "禁用状态";
    toggles_[1].set_default_on(true);
    toggles_[1].disabled = true;

    // ---- 标题栏 ----
    title_bar_.title = "AetherUI Gallery";
    title_bar_.corner_radius = 0.0f;
    nav_.corner_radius = 0.0f;

    // ---- 右上角主题下拉框 ----
    const char* preferences[] = {"跟随系统", "浅色", "深色"};
    theme_dropdown_.items.clear();
    for (const char* label : preferences) {
        DropdownOption option;
        option.label = label;
        theme_dropdown_.items.push_back(option);
    }
    theme_dropdown_.selected = 0;
    theme_dropdown_.placement = PanelPlacement::Below;
    theme_dropdown_.width = 0.0f;

    // ---- 图标页滚动条 ----
    const float rows = std::ceil(static_cast<float>(icons::kIconEntryCount) / kIconColumns);
    const float content = rows * kIconsRowHeight + (rows - 1.0f) * kIconsRowGap;
    icons_bar_.set_range(0.0, std::max(content - kIconsViewportHeight, 0.0f));
    icons_bar_.set_page_size(kIconsViewportHeight);
}

void Gallery::set_viewport(float width, float height, float scale) {
    width_ = width;
    height_ = height;
    scale_ = scale;
}

void Gallery::set_page(Page page, double now) {
    page_ = page;
    nav_.set_selected(static_cast<int>(page), now, scale_);
}

void Gallery::apply_theme() {
    const bool dark = theme_preference_ == 0 ? system_dark_ : theme_preference_ == 2;
    theme_ = dark ? Theme::dark_theme() : Theme::light();
}

void Gallery::toggle_theme(double now) {
    system_dark_ = !theme_.dark;
    apply_theme();
    (void)now;
}

void Gallery::set_theme_mode(bool dark) {
    system_dark_ = dark;
    theme_preference_ = 0;
    apply_theme();
}

void Gallery::debug_open(const std::string& which, double now) {
    (void)now;
    if (which == "theme") {
        theme_dropdown_.open = true;
        theme_dropdown_.panel.snap(1.0f);
        return;
    }
    if (which == "collapse") {
        nav_.expanded = false;
        nav_.width.snap(56.0f);
        return;
    }
    if (which.size() < 2) {
        return;
    }
    const int index = which[1] - '0';
    if (which[0] == 'd' && index >= 0 && index < static_cast<int>(dropdowns_.size())) {
        dropdowns_[static_cast<size_t>(index)].open = true;
        dropdowns_[static_cast<size_t>(index)].panel.snap(1.0f);
        dropdowns_[static_cast<size_t>(index)].hover_item = 0;
    } else if (which[0] == 'p' && index >= 0 &&
               index < static_cast<int>(placement_dropdowns_.size())) {
        placement_dropdowns_[static_cast<size_t>(index)].open = true;
        placement_dropdowns_[static_cast<size_t>(index)].panel.snap(1.0f);
    }
}

void Gallery::close_panels(double now) {
    for (Dropdown& dropdown : dropdowns_) {
        dropdown.close(now, theme_);
    }
    for (Dropdown& dropdown : placement_dropdowns_) {
        dropdown.close(now, theme_);
    }
    theme_dropdown_.close(now, theme_);
}

void Gallery::sync_ime(Window& window) {
    const float s = scale_;
    TextBox* boxes[4] = {&text_box_, &numeric_box_, &disabled_box_, &icon_box_};
    TextBox* target = nullptr;
    if (page_ == Page::TextBox) {
        for (TextBox* box : boxes) {
            if (box->focused && !box->disabled) {
                target = box;
                break;
            }
        }
    }
    if (target == nullptr) {
        // 没有聚焦输入框：清空一次锚点即可，避免每次按键都调用 IME 接口。
        if (ime_valid_) {
            window.clear_text_input_area();
            ime_valid_ = false;
        }
        return;
    }
    // 组字过程中重设锚点会让输入法反复重排（Windows 上可能卡住），交给输入法自己跟随。
    if (composing_) {
        return;
    }

    const float line_height = theme_.body.line_height * s;
    const float text_x = target->field.x + 10.0f * s;
    const float text_y = target->field.y + 4.0f * s + 12.0f * s;
    const size_t caret =
        std::min<size_t>(static_cast<size_t>(std::max(target->caret, 0)), target->text.size());
    const std::string prefix = target->text.substr(0, caret);
    const float caret_x =
        text_x + text_width(text_, prefix.c_str(), kFaceRegular, theme_.body.size * s) -
        target->scroll;
    if (ime_valid_ && std::fabs(ime_x_ - caret_x) < 0.5f && std::fabs(ime_y_ - text_y) < 0.5f &&
        std::fabs(ime_h_ - line_height) < 0.5f) {
        return;
    }
    window.set_text_input_area(caret_x, text_y, s, line_height);
    ime_x_ = caret_x;
    ime_y_ = text_y;
    ime_h_ = line_height;
    ime_valid_ = true;
}

void Gallery::tick(double now) {
    icons_offset_ = static_cast<float>(icons_bar_.value);
    (void)now;
}

// ---------------------------------------------------------------- 布局
void Gallery::layout(double now) {
    const float s = scale_;
    const float nav_w = std::clamp(nav_.width.value(now, theme_.motion_text_box_focus,
                                                    Theme::focus_underline()),
                                   56.0f, 240.0f) *
                        s;
    title_bar_.layout(0.0f, 0.0f, width_, s);
    nav_.layout(0.0f, kTitleBarHeight * s, nav_w, height_ - kTitleBarHeight * s, s);

    page_x_ = nav_w + kPagePadding * s;
    page_y_ = kTitleBarHeight * s + kPagePadding * s;
    content_w_ = std::max(width_ - nav_w - kPagePadding * 2.0f * s, 0.0f);

    // 主题下拉框贴内容区右上角。
    theme_dropdown_.measure(text_, theme_, s);
    theme_dropdown_.place(width_ - 20.0f * s - theme_dropdown_.field.w, kTitleBarHeight * s + 16.0f * s);

    // ---- 输入框页 ----
    TextBox* boxes[4] = {&text_box_, &numeric_box_, &disabled_box_, &icon_box_};
    float y = page_y_ + kTitleHeight * s + kPageGap * s + kCaptionHeight * s + kPageGap * s;
    for (TextBox* box : boxes) {
        box->measure(s);
        box->place(page_x_, y);
        y += box->height + kPageGap * s;
    }

    // ---- 按钮页 ----
    for (int row = 0; row < 2; ++row) {
        float x = page_x_;
        float row_y = page_y_ + 36.0f * s + kPageGap * s + theme_.caption.line_height * s +
                      kPageGap * s + static_cast<float>(row) * (32.0f + 12.0f) * s;
        for (int col = 0; col < 4; ++col) {
            Button& button = buttons_[static_cast<size_t>(row * 4 + col)];
            button.measure(text_, theme_, s);
            button.place(x, row_y);
            x += button.rect.w + 12.0f * s;
        }
    }

    // ---- 下拉框页 ----
    const float dropdown_top = page_y_ + kTitleHeight * s + kPageGap * s + kCaptionHeight * s + kPageGap * s;
    for (size_t i = 0; i < dropdowns_.size(); ++i) {
        Dropdown& dropdown = dropdowns_[i];
        dropdown.measure(text_, theme_, s);
        const Point origin = grid_origin(static_cast<int>(i), kDropdownColumns,
                                         Point{page_x_, dropdown_top}, kPageGap * s);
        dropdown.place(origin.x, origin.y);
    }
    const float placement_top = dropdown_top + 2.0f * kDropdownChromeHeight * s +
                                3.0f * kPageGap * s + kCaptionHeight * s;
    for (size_t i = 0; i < placement_dropdowns_.size(); ++i) {
        Dropdown& dropdown = placement_dropdowns_[i];
        dropdown.measure(text_, theme_, s);
        const Point origin = grid_origin(static_cast<int>(i), kPlacementColumns,
                                         Point{page_x_, placement_top}, kPageGap * s);
        dropdown.place(origin.x, origin.y);
    }

    // ---- 开关页 ----
    for (int i = 0; i < 2; ++i) {
        ToggleSwitch& toggle = toggles_[i];
        const float label_width = text_width(text_, toggle.label.c_str(), kFaceRegular,
                                             theme_.body.size * s);
        toggle.measure(label_width, theme_, s);
        toggle.place(page_x_, page_y_ + 36.0f * s + kPageGap * s + theme_.caption.line_height * s +
                                 kPageGap * s + static_cast<float>(i) * (40.0f + 12.0f) * s,
                     s);
    }

    // ---- 图标页 ----
    const float viewport_w = std::max(content_w_ - (12.0f + 8.0f) * s, 0.0f);
    const float viewport_x = page_x_;
    const float viewport_y = page_y_ + 36.0f * s + kPageGap * s + theme_.caption.line_height * s +
                             kPageGap * s;
    icons_bar_.layout(viewport_x + viewport_w + 8.0f * s, viewport_y, 12.0f * s,
                      kIconsViewportHeight * s, s);

    const float gap = 8.0f * s;
    const float cell_w = (viewport_w - gap * 2.0f) / 3.0f;
    for (size_t i = 0; i < icon_buttons_.size(); ++i) {
        const int col = static_cast<int>(i) % kIconColumns;
        const int row = static_cast<int>(i) / kIconColumns;
        const float cell_x = viewport_x + static_cast<float>(col) * (cell_w + gap);
        const float cell_y = viewport_y - icons_offset_ * s +
                             static_cast<float>(row) * (kIconCellHeight + kIconsRowGap) * s;
        Button& button = icon_buttons_[i];
        button.measure(text_, theme_, s);
        button.place(cell_x + cell_w - 10.0f * s - kIconsCopySize * s,
                     cell_y + kIconCellHeight * s - 10.0f * s - kIconsCopySize * s);
    }
}

// ---------------------------------------------------------------- 绘制
void Gallery::draw(Surface& surface, double now) {
    layout(now);

    Painter painter(surface);
    painter.fill_rect(Rect{0.0f, 0.0f, static_cast<float>(surface.width()),
                           static_cast<float>(surface.height())},
                      theme_.solid_background_base);

    Context ctx{painter, text_, theme_, scale_, now, theme_.dark};

    // 内容区（标题栏下方、导航右侧）。
    painter.set_clip(intersect(surface.bounds(),
                               cover(Rect{nav_.rect.right(), kTitleBarHeight * scale_,
                                          width_ - nav_.rect.right(),
                                          height_ - kTitleBarHeight * scale_})));
    draw_page(ctx, now);
    painter.set_clip(surface.bounds());

    // 导航、标题栏在内容之上；下拉面板再压在内容上方。
    draw_dropdown_overlays(ctx, now);

    nav_.draw(ctx);
    title_bar_.draw(ctx);

    // 右上角主题下拉框位于最顶层。
    const Recti saved = painter.clip();
    painter.set_clip(cover(Rect{nav_.rect.right(), kTitleBarHeight * scale_, width_ - nav_.rect.right(),
                                height_}));
    theme_dropdown_.draw_field(ctx);
    painter.set_clip(saved);
}

void Gallery::draw_dropdown_overlays(const Context& ctx, double now) {
    const float s = scale_;
    if (page_ == Page::Dropdown) {
        for (size_t i = 0; i < dropdowns_.size(); ++i) {
            const Dropdown& dropdown = dropdowns_[i];
            if (!dropdown.panel_visible(now)) {
                continue;
            }
            dropdown.draw_panel(ctx, dropdown.panel_top(dropdown.field.y, theme_, s));
        }
        for (const Dropdown& dropdown : placement_dropdowns_) {
            if (!dropdown.panel_visible(now)) {
                continue;
            }
            dropdown.draw_panel(ctx, dropdown.panel_top(dropdown.field.y, theme_, s));
        }
    }

    if (theme_dropdown_.panel_visible(now)) {
        theme_dropdown_.draw_panel(
            ctx, theme_dropdown_.panel_top(theme_dropdown_.field.y, theme_, s));
    }
}

void Gallery::draw_page(const Context& ctx, double now) {
    (void)now;
    switch (page_) {
        case Page::Home:
            page_home(ctx);
            break;
        case Page::TextBox:
            page_text_box(ctx);
            break;
        case Page::Button:
            page_button(ctx);
            break;
        case Page::Dropdown:
            page_dropdown(ctx);
            break;
        case Page::Toggle:
            page_toggle(ctx);
            break;
        case Page::Fonts:
            page_fonts(ctx);
            break;
        case Page::Icons:
            page_icons(ctx);
            break;
        case Page::About:
            page_about(ctx);
            break;
    }
}

void Gallery::page_home(const Context& ctx) {
    const float s = scale_;
    Painter& painter = ctx.painter;
    const float x = page_x_;
    float y = page_y_;
    draw_text(painter, text_, "欢迎使用 AetherUI", kFaceSemiBold, theme_.title.size * s,
              theme_.title.line_height * s, x, y, theme_.text_primary);
    y += theme_.title.line_height * s + kPageGap * s;
    draw_text(painter, text_, "这是一个 Fluent 风格的开源 C++ 控件库演示。", kFaceRegular,
              theme_.body_large.size * s, theme_.body_large.line_height * s, x, y,
              theme_.text_secondary);
    y += theme_.body_large.line_height * s + kPageGap * s;
    draw_text(painter, text_,
              "左侧导航使用 NavigationView，右侧内容区随选中项切换。所有颜色与动效均来自统一主题 token。",
              kFaceRegular, theme_.body.size * s, theme_.body.line_height * s, x, y,
              theme_.text_secondary);
}

void Gallery::page_text_box(const Context& ctx) {
    const float s = scale_;
    Painter& painter = ctx.painter;
    const float x = page_x_;
    float y = page_y_;

    const Rect title_box{x, y, content_w_, kTitleHeight * s};
    fill_centered_text(painter, text_, "输入框", kFaceSemiBold, theme_.title.size * s,
                       theme_.title.line_height * s,
                       Rect{title_box.x, title_box.y, title_box.w, title_box.h},
                       theme_.text_primary);
    y += kTitleHeight * s + kPageGap * s;

    const std::string caption =
        "TextBox · 圆角 6 · 内边距 12 / 10 · Elevation 1 · 自定义右侧图标点击 " +
        std::to_string(icon_box_.right_clicks) + " 次";
    draw_text(painter, text_, caption.c_str(), kFaceRegular, theme_.caption.size * s,
              theme_.caption.line_height * s, x, y, theme_.text_secondary);
    y += kCaptionHeight * s + kPageGap * s;

    TextBox* boxes[4] = {&text_box_, &numeric_box_, &disabled_box_, &icon_box_};
    for (TextBox* box : boxes) {
        box->draw(ctx);
    }
}

void Gallery::page_button(const Context& ctx) {
    const float s = scale_;
    Painter& painter = ctx.painter;
    float y = page_y_;

    draw_text(painter, text_, "按钮", kFaceSemiBold, theme_.title.size * s,
              theme_.title.line_height * s, page_x_, y, theme_.text_primary);
    y += theme_.title.line_height * s + kPageGap * s;
    draw_text(painter, text_,
              "Button · Primary / Standard · 图标 / 文字 / 禁用 · 四态（Normal / PointerOver / Pressed / Disabled）· 自动内容尺寸 + 最小高度兜底",
              kFaceRegular, theme_.caption.size * s, theme_.caption.line_height * s, page_x_, y,
              theme_.text_secondary);
    y += theme_.caption.line_height * s + kPageGap * s;

    for (const Button& button : buttons_) {
        button.draw(ctx);
    }

    const std::string counter = "已点击 " + std::to_string(button_clicks_) + " 次";
    draw_text(painter, text_, counter.c_str(), kFaceRegular, theme_.caption.size * s,
              theme_.caption.line_height * s, page_x_, y + 2.0f * (32.0f + 12.0f) * s + kPageGap * s,
              theme_.text_secondary);
}

void Gallery::page_dropdown(const Context& ctx) {
    const float s = scale_;
    Painter& painter = ctx.painter;
    float y = page_y_;

    draw_text(painter, text_, "下拉框", kFaceSemiBold, theme_.title.size * s,
              theme_.title.line_height * s, page_x_,
              y + (kTitleHeight - theme_.title.line_height) * s * 0.5f, theme_.text_primary);
    y += kTitleHeight * s + kPageGap * s;
    draw_text(painter, text_, "Dropdown × 7 · 4 个菜单项组合 + 3 种面板位置（上方 / 居中 / 下方）",
              kFaceRegular, theme_.caption.size * s, theme_.caption.line_height * s, page_x_, y,
              theme_.text_secondary);
    y += kCaptionHeight * s + kPageGap * s;

    for (const Dropdown& dropdown : dropdowns_) {
        dropdown.draw_field(ctx);
    }
    y += 2.0f * kDropdownChromeHeight * s + 2.0f * kPageGap * s;

    draw_text(painter, text_, "面板位置：上方 / 居中 / 下方", kFaceRegular, theme_.caption.size * s,
              theme_.caption.line_height * s, page_x_, y, theme_.text_secondary);
    y += kCaptionHeight * s + kPageGap * s;

    for (const Dropdown& dropdown : placement_dropdowns_) {
        dropdown.draw_field(ctx);
    }
    y += 2.0f * kDropdownChromeHeight * s + kPageGap * s;

    const std::string notice =
        "选中变更回调：" + (dropdown_notice_.empty() ? std::string("未选中") : dropdown_notice_);
    draw_text(painter, text_, notice.c_str(), kFaceRegular, theme_.caption.size * s,
              theme_.caption.line_height * s, page_x_, y, theme_.text_secondary);
}

void Gallery::page_toggle(const Context& ctx) {
    const float s = scale_;
    Painter& painter = ctx.painter;
    float y = page_y_;

    draw_text(painter, text_, "开关", kFaceSemiBold, theme_.title.size * s,
              theme_.title.line_height * s, page_x_, y, theme_.text_primary);
    y += theme_.title.line_height * s + kPageGap * s;
    draw_text(painter, text_,
              "ToggleSwitch · 点击切换 · 按住滑块拖拽实时跟随，松开后按轨道中点吸附 · 拖拽放大 / 回弹 · 禁用态不可交互",
              kFaceRegular, theme_.caption.size * s, theme_.caption.line_height * s, page_x_, y,
              theme_.text_secondary);

    for (const ToggleSwitch& toggle : toggles_) {
        toggle.draw(ctx);
    }
}

void Gallery::page_fonts(const Context& ctx) {
    const float s = scale_;
    Painter& painter = ctx.painter;
    float y = page_y_;

    draw_text(painter, text_, "字体", kFaceSemiBold, theme_.title.size * s,
              theme_.title.line_height * s, page_x_, y, theme_.text_primary);
    y += theme_.title.line_height * s + kPageGap * s;
    draw_text(painter, text_, "Fonts · 库内嵌字体直接调用，示例文本按对应字体族与字重渲染",
              kFaceRegular, theme_.caption.size * s, theme_.caption.line_height * s, page_x_, y,
              theme_.text_secondary);
    y += theme_.caption.line_height * s + kPageGap * s;

    const float card_w = content_w_;
    for (const FontCard& card : kFontCards) {
        const float card_h = 70.0f * s;
        const Rect box{page_x_, y, card_w, card_h};
        painter.fill_round_rect(box, theme_.radius_card * s, theme_.neutral_background_1);
        painter.stroke_round_rect(box, theme_.radius_card * s, theme_.stroke_thickness * s,
                                  theme_.card_stroke_default);

        const std::string header = std::string(card.name) + " · " + card.family;
        draw_text(painter, text_, header.c_str(), kFaceRegular, theme_.caption.size * s,
                  theme_.caption.line_height * s, box.x + 12.0f * s, box.y + 12.0f * s,
                  theme_.text_secondary);
        draw_text(painter, text_, card.sample, card.face, 20.0f * s, 26.0f * s,
                  box.x + 12.0f * s, box.y + 12.0f * s + theme_.caption.line_height * s + 4.0f * s,
                  theme_.text_primary);
        y += card_h + 8.0f * s;
    }
}

void Gallery::page_icons(const Context& ctx) {
    const float s = scale_;
    Painter& painter = ctx.painter;
    float y = page_y_;

    draw_text(painter, text_, "图标", kFaceSemiBold, theme_.title.size * s,
              theme_.title.line_height * s, page_x_, y, theme_.text_primary);
    y += theme_.title.line_height * s + kPageGap * s;

    std::string caption = "Icons · 内置 " + std::to_string(icons::kIconEntryCount) +
                          " 个图标（icons::ALL 可直接遍历扩展）· ";
    if (copied_icon_ >= 0) {
        caption += std::string("已复制：") + icons::name_of(copied_icon_);
    } else {
        caption += "点击复制可写入图标常量名";
    }
    draw_text(painter, text_, caption.c_str(), kFaceRegular, theme_.caption.size * s,
              theme_.caption.line_height * s, page_x_, y, theme_.text_secondary);
    y += theme_.caption.line_height * s + kPageGap * s;

    const float viewport_w = std::max(content_w_ - 20.0f * s, 0.0f);
    const Rect viewport{page_x_, y, viewport_w, kIconsViewportHeight * s};

    const Recti saved = painter.clip();
    painter.set_clip(intersect(saved, cover(viewport)));

    const float gap = 8.0f * s;
    const float cell_w = (viewport_w - gap * 2.0f) / 3.0f;
    for (int i = 0; i < icons::kIconEntryCount; ++i) {
        const int col = i % kIconColumns;
        const int row = i / kIconColumns;
        const Rect cell{viewport.x + static_cast<float>(col) * (cell_w + gap),
                        viewport.y - icons_offset_ * s +
                            static_cast<float>(row) * (kIconCellHeight + kIconsRowGap) * s,
                        cell_w, kIconCellHeight * s};
        if (cell.bottom() < viewport.y || cell.y > viewport.bottom()) {
            continue;
        }
        painter.fill_round_rect(cell, theme_.radius_card * s, theme_.neutral_background_1);
        painter.stroke_round_rect(cell, theme_.radius_card * s, theme_.stroke_thickness * s,
                                  theme_.card_stroke_default);

        icons::draw(painter, i, cell.x + (cell.w - 28.0f * s) * 0.5f, cell.y + 10.0f * s,
                    28.0f * s, theme_.text_primary, s, icons::variant_for(ctx.dark));

        draw_text(painter, text_, icons::name_of(i), kFaceMono, theme_.caption.size * s,
                  theme_.caption.line_height * s, cell.x + 10.0f * s,
                  cell.bottom() - 10.0f * s - kIconsCopySize * s +
                      (kIconsCopySize - theme_.caption.line_height) * s * 0.5f,
                  theme_.text_secondary);

        icon_buttons_[static_cast<size_t>(i)].draw(ctx);
    }
    painter.set_clip(saved);

    icons_bar_.draw(ctx);
}

void Gallery::page_about(const Context& ctx) {
    const float s = scale_;
    Painter& painter = ctx.painter;
    float y = page_y_;

    draw_text(painter, text_, "关于 AetherUI", kFaceSemiBold, theme_.title.size * s,
              theme_.title.line_height * s, page_x_, y, theme_.text_primary);
    y += theme_.title.line_height * s + kPageGap * s;
    draw_text(painter, text_, "AetherUI 是一个 MIT 协议、可商用的开源 UI 库，定位为 Linux 可用的 Fluent 风格控件库。",
              kFaceRegular, theme_.body_large.size * s, theme_.body_large.line_height * s, page_x_,
              y, theme_.text_secondary);
    y += theme_.body_large.line_height * s + kPageGap * s;
    draw_text(painter, text_, "Rendering: SDL3 + self-written CPU rasterizer · C++20 · MIT License",
              kFaceMono, theme_.caption.size * s, theme_.caption.line_height * s, page_x_, y,
              theme_.text_tertiary);
}

// ---------------------------------------------------------------- 事件
void Gallery::handle_event(const Event& event, Window& window, double now) {
    const float s = scale_;

    switch (event.type) {
        case Event::Type::Resize:
            width_ = static_cast<float>(event.width);
            height_ = static_cast<float>(event.height);
            break;
        case Event::Type::MouseMove: {
            cursor_x_ = event.x;
            cursor_y_ = event.y;
            title_bar_.hover_minimize = title_bar_.btn_minimize.contains(event.x, event.y);
            title_bar_.hover_maximize = title_bar_.btn_maximize.contains(event.x, event.y);
            title_bar_.hover_close = title_bar_.btn_close.contains(event.x, event.y);
            nav_.hamburger_hovered = nav_.hamburger_at(event.x, event.y);

            const int hovered_item = nav_.item_at(event.x, event.y, s);
            for (size_t i = 0; i < nav_.items.size(); ++i) {
                nav_.items[i].hovered = hovered_item == static_cast<int>(i);
            }
            for (size_t i = 0; i < nav_.footer.size(); ++i) {
                nav_.footer[i].hovered =
                    hovered_item == static_cast<int>(nav_.items.size() + i);
            }

            TextBox* boxes[4] = {&text_box_, &numeric_box_, &disabled_box_, &icon_box_};
            for (TextBox* box : boxes) {
                if (box->disabled) {
                    box->hovered = false;
                    box->right_hovered = false;
                    continue;
                }
                box->hovered = box->field.contains(event.x, event.y);
                box->right_hovered = box->right_icon_rect(s).contains(event.x, event.y) &&
                                     (box->right_icon_custom || (box->focused && !box->text.empty()));
            }

            // 拖拽选区：按住左键移动时只移动活动端，锚点保持不动。
            if (drag_box_ != nullptr && page_ == Page::TextBox) {
                drag_box_->caret =
                    drag_box_->index_at_x(text_, event.x, s, theme_.body.size * s);
            }

            for (Button& button : buttons_) {
                button.hovered = !button.disabled && button.rect.contains(event.x, event.y);
            }
            if (page_ == Page::Icons) {
                for (Button& button : icon_buttons_) {
                    button.hovered = button.rect.contains(event.x, event.y);
                }
            }

            for (ToggleSwitch& toggle : toggles_) {
                toggle.update_hover(toggle.rect.contains(event.x, event.y), now, theme_);
                if (toggle.pressed) {
                    toggle.update_drag(event.x, now, theme_);
                }
            }

            if (icons_bar_.rect.contains(event.x, event.y) != icons_bar_.hovered) {
                icons_bar_.hovered = icons_bar_.rect.contains(event.x, event.y);
                icons_bar_.hover.retarget(icons_bar_.hovered ? 1.0f : 0.0f, now,
                                          theme_.motion_fast, theme_.easing);
            }
            if (icons_bar_.dragging) {
                icons_bar_.drag_to(event.y - icons_bar_.rect.y);
            }

            for (Dropdown& dropdown : dropdowns_) {
                dropdown.hover_item = -1;
            }
            for (Dropdown& dropdown : placement_dropdowns_) {
                dropdown.hover_item = -1;
            }
            theme_dropdown_.hover_item = -1;

            // 面板内条目悬停。
            const float panel_h = dropdowns_.empty() ? 0.0f : 0.0f;
            (void)panel_h;
            auto hover_panel = [&](Dropdown& dropdown) {
                if (!dropdown.open) {
                    return;
                }
                const float top = dropdown.panel_top(dropdown.field.y, theme_, s);
                const float row_h = std::max(theme_.body.line_height * s, 16.0f * s) + 12.0f * s;
                for (size_t i = 0; i < dropdown.items.size(); ++i) {
                    const Rect row{dropdown.field.x + 4.0f * s,
                                   top + 4.0f * s + static_cast<float>(i) * (row_h + 2.0f * s),
                                   dropdown.chrome_width() - 8.0f * s, row_h};
                    if (row.contains(event.x, event.y) && !dropdown.items[i].disabled) {
                        dropdown.hover_item = static_cast<int>(i);
                        break;
                    }
                }
            };
            if (page_ == Page::Dropdown) {
                for (Dropdown& dropdown : dropdowns_) {
                    hover_panel(dropdown);
                }
                for (Dropdown& dropdown : placement_dropdowns_) {
                    hover_panel(dropdown);
                }
            }
            hover_panel(theme_dropdown_);
            break;
        }
        case Event::Type::MouseDown: {
            if (event.key != 1) {
                break;
            }
            mouse_down_ = true;

            // 1. 标题栏
            if (title_bar_.btn_close.contains(event.x, event.y)) {
                window.request_close();
                break;
            }
            if (title_bar_.btn_maximize.contains(event.x, event.y)) {
                window.toggle_maximize();
                title_bar_.maximized = window.maximized();
                break;
            }
            if (title_bar_.btn_minimize.contains(event.x, event.y)) {
                window.minimize();
                break;
            }
            if (event.y < kTitleBarHeight * s) {
                // 双击标题栏切换最大化，否则开始拖动窗口。
                if (last_title_click_ >= 0.0 && now - last_title_click_ < 0.5 &&
                    std::abs(event.x - last_title_click_x_) < 6.0f * s) {
                    window.toggle_maximize();
                    title_bar_.maximized = window.maximized();
                    last_title_click_ = -1.0;
                } else {
                    window.begin_drag();
                    last_title_click_ = now;
                    last_title_click_x_ = event.x;
                }
                break;
            }

            // 2. 悬浮面板（主题面板在最上层）
            bool panel_handled = false;
            auto click_panel = [&](Dropdown& dropdown, bool is_theme) {
                if (!dropdown.open) {
                    return;
                }
                const float top = dropdown.panel_top(dropdown.field.y, theme_, s);
                const float row_h = std::max(theme_.body.line_height * s, 16.0f * s) + 12.0f * s;
                const Rect panel{dropdown.field.x, top, dropdown.chrome_width(),
                                 dropdown.panel_height(theme_, s)};
                if (!panel.contains(event.x, event.y)) {
                    return;
                }
                panel_handled = true;
                for (size_t i = 0; i < dropdown.items.size(); ++i) {
                    const Rect row{panel.x + 4.0f * s,
                                   panel.y + 4.0f * s + static_cast<float>(i) * (row_h + 2.0f * s),
                                   panel.w - 8.0f * s, row_h};
                    if (row.contains(event.x, event.y) && !dropdown.items[i].disabled) {
                        dropdown.select(static_cast<int>(i), now, theme_);
                        if (is_theme) {
                            theme_preference_ = static_cast<int>(i);
                            apply_theme();
                        } else {
                            const DropdownOption* item = dropdown.selected_item();
                            if (item != nullptr) {
                                dropdown_notice_ = item->label;
                            }
                        }
                        break;
                    }
                }
            };

            click_panel(theme_dropdown_, true);
            if (!panel_handled && page_ == Page::Dropdown) {
                for (Dropdown& dropdown : dropdowns_) {
                    click_panel(dropdown, false);
                }
                for (Dropdown& dropdown : placement_dropdowns_) {
                    click_panel(dropdown, false);
                }
            }

            // 3. 主题下拉框闭合框
            if (!panel_handled && theme_dropdown_.field.contains(event.x, event.y)) {
                close_panels(now);
                theme_dropdown_.toggle(now, theme_);
                break;
            }

            // 4. 导航
            if (!panel_handled && nav_.rect.contains(event.x, event.y)) {
                if (nav_.hamburger_at(event.x, event.y)) {
                    nav_.press_feedback(now);
                    nav_.toggle_expanded(now, theme_);
                    break;
                }
                const int index = nav_.item_at(event.x, event.y, s);
                if (index >= 0) {
                    close_panels(now);
                    page_ = static_cast<Page>(index);
                    nav_.set_selected(index, now, s);
                    break;
                }
            }

            // 5. 页面控件
            if (!panel_handled) {
                TextBox* boxes[4] = {&text_box_, &numeric_box_, &disabled_box_, &icon_box_};
                drag_box_ = nullptr;
                for (TextBox* box : boxes) {
                    if (page_ != Page::TextBox || box->disabled) {
                        box->focused = false;
                        continue;
                    }
                    box->focused = box->field.contains(event.x, event.y);
                    box->pressed = box->focused;
                    if (box->focused) {
                        const bool double_click =
                            last_text_box_ == box && now - last_text_click_ < 0.4;
                        if (double_click) {
                            box->select_all();
                        } else {
                            box->place_caret_at(text_, event.x, s, theme_.body.size * s);
                        }
                        drag_box_ = box;
                        last_text_box_ = box;
                        last_text_click_ = now;
                    }
                }
                // 失焦的输入框不再保留选区高亮。
                for (TextBox* box : boxes) {
                    if (!box->focused) {
                        box->anchor = box->caret;
                    }
                }

                switch (page_) {
                    case Page::Button:
                        for (Button& button : buttons_) {
                            if (!button.disabled && button.rect.contains(event.x, event.y)) {
                                button.pressed = true;
                            }
                        }
                        break;
                    case Page::Toggle:
                        for (ToggleSwitch& toggle : toggles_) {
                            if (!toggle.disabled && toggle.rect.contains(event.x, event.y)) {
                                toggle.update_press(event.x, now, theme_);
                            }
                        }
                        break;
                    case Page::Dropdown:
                        for (Dropdown& dropdown : dropdowns_) {
                            if (dropdown.field.contains(event.x, event.y)) {
                                close_panels(now);
                                dropdown.toggle(now, theme_);
                                panel_handled = true;
                                break;
                            }
                        }
                        if (!panel_handled) {
                            for (Dropdown& dropdown : placement_dropdowns_) {
                                if (dropdown.field.contains(event.x, event.y)) {
                                    close_panels(now);
                                    dropdown.toggle(now, theme_);
                                    panel_handled = true;
                                    break;
                                }
                            }
                        }
                        break;
                    case Page::Icons:
                        if (icon_box_.focused) {
                            icon_box_.focused = false;
                        }
                        for (size_t i = 0; i < icon_buttons_.size(); ++i) {
                            if (icon_buttons_[i].rect.contains(event.x, event.y)) {
                                icon_buttons_[i].pressed = true;
                            }
                        }
                        if (icons_bar_.rect.contains(event.x, event.y)) {
                            icons_bar_.press_at(event.y - icons_bar_.rect.y, now, theme_);
                        }
                        break;
                    default:
                        break;
                }

                // 输入框右侧图标
                TextBox* targets[3] = {&text_box_, &numeric_box_, &icon_box_};
                if (page_ == Page::TextBox) {
                    for (TextBox* box : targets) {
                        const bool icon_shown = box->right_icon_custom ||
                                                (box->focused && !box->text.empty());
                        if (icon_shown && box->right_icon_rect(s).contains(event.x, event.y)) {
                            if (box->right_icon_custom) {
                                ++box->right_clicks;
                            } else {
                                box->clear();
                            }
                            box->focused = true;
                        }
                    }
                }
            }

            // 点击面板与闭合框之外：收起所有面板。
            if (!panel_handled) {
                bool inside_field = theme_dropdown_.field.contains(event.x, event.y);
                for (const Dropdown& dropdown : dropdowns_) {
                    inside_field = inside_field || dropdown.field.contains(event.x, event.y);
                }
                for (const Dropdown& dropdown : placement_dropdowns_) {
                    inside_field = inside_field || dropdown.field.contains(event.x, event.y);
                }
                if (!inside_field) {
                    for (Dropdown& dropdown : dropdowns_) {
                        dropdown.close(now, theme_);
                    }
                    for (Dropdown& dropdown : placement_dropdowns_) {
                        dropdown.close(now, theme_);
                    }
                    theme_dropdown_.close(now, theme_);
                }
            }
            break;
        }
        case Event::Type::MouseUp: {
            if (event.key != 1) {
                break;
            }
            mouse_down_ = false;
            drag_box_ = nullptr;
            nav_.pointer_up(now);

            for (Button& button : buttons_) {
                if (button.pressed) {
                    const bool clicked = button.rect.contains(event.x, event.y);
                    button.pressed = false;
                    if (clicked) {
                        ++button_clicks_;
                    }
                }
            }
            for (Button& button : icon_buttons_) {
                if (button.pressed) {
                    button.pressed = false;
                    if (button.rect.contains(event.x, event.y) && page_ == Page::Icons) {
                        for (size_t i = 0; i < icon_buttons_.size(); ++i) {
                            if (&icon_buttons_[i] == &button) {
                                copied_icon_ = static_cast<int>(i);
                                SDL_SetClipboardText(icons::name_of(static_cast<int>(i)));
                                break;
                            }
                        }
                    }
                }
            }

            for (ToggleSwitch& toggle : toggles_) {
                toggle.update_release(now, theme_);
            }

            if (icons_bar_.dragging) {
                icons_bar_.release(now, theme_);
            }

            for (TextBox* box : {&text_box_, &numeric_box_, &disabled_box_, &icon_box_}) {
                box->pressed = false;
            }
            break;
        }
        case Event::Type::Wheel: {
            if (page_ != Page::Icons) {
                break;
            }
            const Rect viewport{page_x_, icons_bar_.rect.y, icons_bar_.rect.x - page_x_,
                                icons_bar_.rect.h};
            const bool over_bar = icons_bar_.rect.contains(event.x, event.y);
            if (!viewport.contains(event.x, event.y) && !over_bar) {
                break;
            }
            icons_bar_.wheel(event.wheel);
            break;
        }
        case Event::Type::KeyDown: {
            if (event.key == SDLK_ESCAPE) {
                window.request_close();
                break;
            }
            // 组字期间按键属于输入法，界面层不要重复处理。
            if (composing_) {
                break;
            }
            TextBox* box = nullptr;
            TextBox* boxes[4] = {&text_box_, &numeric_box_, &disabled_box_, &icon_box_};
            for (TextBox* candidate : boxes) {
                if (candidate->focused && !candidate->disabled) {
                    box = candidate;
                }
            }
            if (box != nullptr) {
                handle_text_shortcut(*box, event);
            }
            break;
        }
        case Event::Type::Text: {
            composing_ = false;  // 上屏即组字结束
            TextBox* boxes[4] = {&text_box_, &numeric_box_, &disabled_box_, &icon_box_};
            for (TextBox* box : boxes) {
                if (box->focused && !box->disabled) {
                    box->insert_text(event.text);
                }
            }
            break;
        }
        case Event::Type::Editing: {
            // 组字串为空表示组字结束，此时才允许重设候选窗锚点。
            composing_ = !event.text.empty();
            if (!composing_) {
                sync_ime(window);
            }
            break;
        }
        default:
            break;
    }

    if (event.type == Event::Type::MouseDown) {
        composing_ = false;
        sync_ime(window);
    }
}

}  // namespace aether
