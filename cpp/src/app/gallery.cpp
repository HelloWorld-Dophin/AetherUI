#include "app/gallery.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

#include <SDL3/SDL.h>

#include "platform/clipboard.h"
#include "ui/icons.h"
#include "ui/layout.h"

namespace aether {
namespace {

// 输入框页在 `boxes_` 中的下标。
constexpr int kBoxText = 0;
constexpr int kBoxNumeric = 1;
constexpr int kBoxDisabled = 2;
constexpr int kBoxIcon = 3;

constexpr int kIconColumns = 3;
constexpr float kIconsViewportHeight = 512.0f;
constexpr float kIconsRowGap = 12.0f;
constexpr float kIconCellHeight = 88.0f;
constexpr float kTileGap = 8.0f;
constexpr float kFontCardHeight = 70.0f;
constexpr float kFontCardGap = 8.0f;
constexpr float kRowGap = 12.0f;

struct FontCard {
    const char* name;
    const char* family;
    int face;
    const char* sample;
};

const FontCard kFontCards[] = {
    {"更纱黑体 Light", "Sarasa UI SC", kFaceLight, "AetherUI 字体展示 · 0123 · The quick brown fox"},
    {"更纱黑体 Regular", "Sarasa UI SC", kFaceRegular, "AetherUI 字体展示 · 0123 · The quick brown fox"},
    {"更纱黑体 SemiBold", "Sarasa UI SC", kFaceSemiBold,
     "AetherUI 字体展示 · 0123 · The quick brown fox"},
    {"JetBrains Mono Regular", "JetBrains Mono", kFaceMono, "fn main() { println!(\"AetherUI\"); }"},
    {"JetBrains Mono SemiBold", "JetBrains Mono", kFaceMonoSemiBold,
     "fn main() { println!(\"AetherUI\"); }"},
    {"Noto Sans Symbols 2", "Noto Sans Symbols 2", kFaceSymbols, "→ ✓ ★ ♥ ⚙ ⌘ ⚡ ✉"},
    {"Noto Emoji", "Noto Emoji", kFaceEmoji, "😀 🚀 ❤️ 🌏 🔧 ✨"},
};

constexpr int kFontCardCount = static_cast<int>(sizeof(kFontCards) / sizeof(kFontCards[0]));

/// 不区分大小写的子串匹配（`needle` 需已转小写）。
bool name_contains(const char* name, const std::string& needle) {
    if (needle.empty()) {
        return true;
    }
    const size_t length = needle.size();
    for (const char* p = name; *p != '\0'; ++p) {
        size_t i = 0;
        while (i < length && p[i] != '\0' &&
               std::tolower(static_cast<unsigned char>(p[i])) == needle[i]) {
            ++i;
        }
        if (i == length) {
            return true;
        }
    }
    return false;
}

std::string to_lower_ascii(const std::string& value) {
    std::string result = value;
    for (char& c : result) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

}  // namespace

Gallery::Gallery(TextRenderer& text, const std::string& title, const AppIcon& icon)
    : text_(text) {
    themes_.set_system_dark(true);
    build_widgets();
    title_bar_.title = title;
    title_bar_.icon = icon;
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

    const Theme& theme = themes_.theme();

    // ---- 导航：7 个主条目 + 「关于」固定底部 ----
    const struct {
        const char* label;
        int icon;
    } nav_specs[8] = {
        {"主页", icon_home_},           {"输入框", icon_cursor_},
        {"按钮", icon_control_button_}, {"下拉框", icon_chevron_down_},
        {"开关", icon_toggle_},         {"字体", icon_text_font_},
        {"图标", icon_grid_},           {"关于", icon_info_},
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
    nav_.corner_radius = 0.0f;
    nav_.on_select = [this](int index) { set_page(static_cast<Page>(index), now_); };

    // ---- 输入框 ----
    boxes_.resize(4);
    boxes_[kBoxText].placeholder = "请输入文本";
    boxes_[kBoxNumeric].placeholder = "仅数字，可带一个前导负号";
    boxes_[kBoxNumeric].numeric_only = true;
    boxes_[kBoxDisabled].placeholder = "禁用状态";
    boxes_[kBoxDisabled].disabled = true;
    boxes_[kBoxIcon].placeholder = "右侧为自定义图标，点击触发回调";
    boxes_[kBoxIcon].right_icon = icon_info_;
    boxes_[kBoxIcon].right_icon_custom = true;
    for (TextBox& box : boxes_) {
        box.right_icon = box.right_icon_custom ? icon_info_ : icon_dismiss_;
    }
    for (TextBox& box : boxes_) {
        focus_.add(&box);
    }

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

    for (Dropdown& dropdown : dropdowns_) {
        dropdown.on_select = [this](int index) {
            const DropdownOption* item = dropdowns_[static_cast<size_t>(index)].selected_item();
            if (item != nullptr) {
                dropdown_notice_ = item->label;
            }
        };
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
        button.measure(text_, theme, 1.0f);
        button.on_click = [this]() { ++button_clicks_; };
        buttons_.push_back(button);
    }

    // ---- 图标页：图标卡片（右下角为复制按钮）----
    icon_tiles_.clear();
    icon_tiles_.resize(static_cast<size_t>(icons::kIconEntryCount));
    for (size_t i = 0; i < icon_tiles_.size(); ++i) {
        IconTile& tile = icon_tiles_[i];
        tile.icon = static_cast<int>(i);
        tile.name = icons::name_of(static_cast<int>(i));
        tile.visible = false;
        tile.action.icon = icon_copy_;
        tile.action.compact = true;
        tile.action.measure(text_, theme, 1.0f);
        tile.action.on_click = [this, i]() {
            copied_icon_ = static_cast<int>(i);
            set_clipboard_text(icons::name_of(static_cast<int>(i)));
        };
    }

    // ---- 图标页：搜索框（右侧换成内嵌放大镜）----
    icon_search_.placeholder = "搜索图标名称";
    icon_search_.right_icon = icons::index_of("SEARCH");
    icon_search_.right_icon_custom = true;
    icon_search_.width = 320.0f;
    focus_.add(&icon_search_);

    // ---- 开关 ----
    toggles_[0].label = "启用状态";
    toggles_[0].set_default_on(true);
    toggles_[1].label = "禁用状态";
    toggles_[1].set_default_on(true);
    toggles_[1].disabled = true;

    // ---- 标题栏 ----
    title_bar_.corner_radius = 0.0f;

    // ---- 右上角主题下拉框 ----
    const char* preferences[] = {"跟随系统", "浅色", "深色"};
    theme_dropdown_.items.clear();
    for (const char* label : preferences) {
        DropdownOption option;
        option.label = label;
        theme_dropdown_.items.push_back(option);
    }
    theme_dropdown_.selected = ThemeManager::kFollowSystem;
    theme_dropdown_.placement = PanelPlacement::Below;
    theme_dropdown_.width = 0.0f;
    theme_dropdown_.on_select = [this](int index) {
        themes_.set_system_dark(host_.system_dark());
        themes_.set_preference(index);
        theme_dropdown_.selected = themes_.preference();
    };

    // ---- 字体页卡片 ----
    font_cards_.assign(static_cast<size_t>(kFontCardCount), Card{});
    font_meta_.resize(static_cast<size_t>(kFontCardCount));
    font_sample_.resize(static_cast<size_t>(kFontCardCount));
    for (int i = 0; i < kFontCardCount; ++i) {
        font_meta_[static_cast<size_t>(i)].text =
            std::string(kFontCards[i].name) + " · " + kFontCards[i].family;
        font_meta_[static_cast<size_t>(i)].style = TextStyle::Caption;
        font_meta_[static_cast<size_t>(i)].role = TextRole::Secondary;

        Label& sample = font_sample_[static_cast<size_t>(i)];
        sample.text = kFontCards[i].sample;
        sample.face = kFontCards[i].face;
        sample.style = TextStyle::Body;
        sample.role = TextRole::Primary;
        sample.size = 20.0f;
        sample.line_height = 26.0f;
    }

    // 主页 / 关于页正文
    paragraph_.style = TextStyle::BodyLarge;
    paragraph_.role = TextRole::Secondary;
    paragraph_.wrap = true;
    note_.style = TextStyle::Caption;
    note_.role = TextRole::Secondary;
}

void Gallery::set_viewport(float width, float height, float scale) {
    width_ = width;
    height_ = height;
    scale_ = scale;
}

void Gallery::set_page(Page page, double now) {
    page_ = page;
    nav_.set_selected(static_cast<int>(page), now, scale_);
    close_panels(now);
    focus_.clear();
}

void Gallery::toggle_theme(double now) {
    themes_.toggle();
    theme_dropdown_.selected = themes_.preference();
    (void)now;
}

void Gallery::set_theme_mode(bool dark) {
    themes_.set_system_dark(dark);
    themes_.set_preference(ThemeManager::kFollowSystem);
    theme_dropdown_.selected = themes_.preference();
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
        Dropdown& dropdown = dropdowns_[static_cast<size_t>(index)];
        dropdown.open = true;
        dropdown.panel.snap(1.0f);
        dropdown.hover_item = 0;
    } else if (which[0] == 'p' && index >= 0 &&
               index < static_cast<int>(placement_dropdowns_.size())) {
        Dropdown& dropdown = placement_dropdowns_[static_cast<size_t>(index)];
        dropdown.open = true;
        dropdown.panel.snap(1.0f);
    }
}

void Gallery::close_panels(double now) {
    const Theme& theme = themes_.theme();
    for (Dropdown& dropdown : dropdowns_) {
        dropdown.close(now, theme);
    }
    for (Dropdown& dropdown : placement_dropdowns_) {
        dropdown.close(now, theme);
    }
    theme_dropdown_.close(now, theme);
}

// ---------------------------------------------------------------- 布局
void Gallery::layout(double now) {
    const float s = scale_;
    const Theme& theme = themes_.theme();

    const float nav_w = std::clamp(nav_.width.value(now, theme.motion_text_box_focus,
                                                    Theme::focus_underline()),
                                   56.0f, 240.0f) *
                        s;
    title_bar_.layout(0.0f, 0.0f, width_, s);
    nav_.layout(0.0f, title_bar_.bar.h, nav_w, height_ - title_bar_.bar.h, s);

    page_x_ = nav_w + kContentPadding * s;
    page_y_ = title_bar_.bar.h + kContentPadding * s;
    content_w_ = std::max(width_ - nav_w - kContentPadding * 2.0f * s, 0.0f);

    // 右上角主题下拉框贴内容区右上角。
    theme_dropdown_.measure(text_, theme, s);
    theme_dropdown_.place(width_ - 20.0f * s - theme_dropdown_.field.w,
                          title_bar_.bar.h + 16.0f * s);

    // 图标页：先按关键词过滤，标题说明里的命中数才是本帧的。
    if (page_ == Page::Icons) {
        update_icon_filter();
    }

    header_.set(page_title(), page_caption());    header_.layout(text_, theme, s, page_x_, page_y_, content_w_);
    const float top = header_.bottom() + kSectionGap * s;

    switch (page_) {
        case Page::Home:
            layout_home(top, s);
            break;
        case Page::TextBox:
            layout_text_box(top, s);
            break;
        case Page::Button:
            layout_button(top, s);
            break;
        case Page::Dropdown:
            layout_dropdown(top, s);
            break;
        case Page::Toggle:
            layout_toggle(top, s);
            break;
        case Page::Fonts:
            layout_fonts(top, s);
            break;
        case Page::Icons:
            layout_icons(top, s);
            break;
        case Page::About:
            layout_about(top, s);
            break;
    }

    // 参与事件的下拉面板只包含当前页可见的那几个。
    dropdowns_group_.clear();
    if (page_ == Page::Dropdown) {
        for (Dropdown& dropdown : dropdowns_) {
            dropdowns_group_.add(dropdown);
        }
        for (Dropdown& dropdown : placement_dropdowns_) {
            dropdowns_group_.add(dropdown);
        }
    }
    dropdowns_group_.add(theme_dropdown_);
}

void Gallery::layout_home(float top, float s) {
    const Theme& theme = themes_.theme();
    paragraph_.text = "这是一个 Fluent 风格的开源 C++ 控件库演示。";
    paragraph_.face = kFaceRegular;
    paragraph_.style = TextStyle::BodyLarge;
    paragraph_.role = TextRole::Secondary;
    paragraph_.wrap = true;
    paragraph_.layout(text_, theme, s, page_x_, top, content_w_);

    note_.text = "左侧导航使用 NavigationView，右侧内容区随选中项切换。所有颜色与动效均来自统一主题 token。";
    note_.face = kFaceRegular;
    note_.style = TextStyle::Body;
    note_.role = TextRole::Secondary;
    note_.wrap = true;
    note_.layout(text_, theme, s, page_x_, paragraph_.rect.bottom() + kSectionGap * s, content_w_);
}

void Gallery::layout_text_box(float top, float s) {
    VStack stack;
    stack.reset(page_x_, top, content_w_, kSectionGap * s);
    for (TextBox& box : boxes_) {
        box.measure(s);
        box.place(page_x_, stack.next(box.height));
    }
}

void Gallery::layout_button(float top, float s) {
    const Theme& theme = themes_.theme();
    float y = top;
    for (int row = 0; row < 2; ++row) {
        float x = page_x_;
        float row_h = 0.0f;
        for (int col = 0; col < 4; ++col) {
            Button& button = buttons_[static_cast<size_t>(row * 4 + col)];
            button.measure(text_, theme, s);
            button.place(x, y);
            x += button.rect.w + kRowGap * s;
            row_h = std::max(row_h, button.rect.h);
        }
        y += row_h + kRowGap * s;
    }

    note_.text = "已点击 " + std::to_string(button_clicks_) + " 次";
    note_.face = kFaceRegular;
    note_.style = TextStyle::Caption;
    note_.role = TextRole::Secondary;
    note_.wrap = false;
    note_.layout(text_, theme, s, page_x_, y + kSectionGap * s, content_w_);
}

void Gallery::layout_dropdown(float top, float s) {
    const Theme& theme = themes_.theme();
    const float gap = kSectionGap * s;

    float cell_w = 280.0f * s;
    float cell_h = 52.0f * s;
    if (!dropdowns_.empty()) {
        dropdowns_[0].measure(text_, theme, s);
        cell_w = dropdowns_[0].field.w;
        cell_h = dropdowns_[0].field.h;
    }

    Grid grid;
    grid.reset(page_x_, top, 2, cell_w, cell_h, gap, gap);
    for (size_t i = 0; i < dropdowns_.size(); ++i) {
        Dropdown& dropdown = dropdowns_[i];
        dropdown.measure(text_, theme, s);
        const Rect cell = grid.cell(static_cast<int>(i));
        dropdown.place(cell.x, cell.y);
    }

    const float after_first = top + 2.0f * (cell_h + gap);
    note_.text = "面板位置：上方 / 居中 / 下方";
    note_.face = kFaceRegular;
    note_.style = TextStyle::Caption;
    note_.role = TextRole::Secondary;
    note_.wrap = false;
    note_.layout(text_, theme, s, page_x_, after_first, content_w_);

    Grid placement_grid;
    placement_grid.reset(page_x_, note_.rect.bottom() + gap, 2, cell_w, cell_h, gap, gap);
    for (size_t i = 0; i < placement_dropdowns_.size(); ++i) {
        Dropdown& dropdown = placement_dropdowns_[i];
        dropdown.measure(text_, theme, s);
        const Rect cell = placement_grid.cell(static_cast<int>(i));
        dropdown.place(cell.x, cell.y);
    }

    footer_.text = "选中变更回调：" + (dropdown_notice_.empty() ? std::string("未选中") : dropdown_notice_);
    footer_.face = kFaceRegular;
    footer_.style = TextStyle::Caption;
    footer_.role = TextRole::Secondary;
    footer_.wrap = false;
    footer_.layout(text_, theme, s, page_x_,
                   note_.rect.bottom() + 2.0f * (cell_h + gap) + kSectionGap * s, content_w_);
}

void Gallery::layout_toggle(float top, float s) {
    const Theme& theme = themes_.theme();
    float y = top;
    for (ToggleSwitch& toggle : toggles_) {
        const float label_width =
            text_width(text_, toggle.label.c_str(), kFaceRegular, theme.body.size * s);
        toggle.measure(label_width, theme, s);
        toggle.place(page_x_, y, s);
        y += toggle.rect.h + kRowGap * s;
    }
}

void Gallery::layout_fonts(float top, float s) {
    const Theme& theme = themes_.theme();
    float y = top;
    for (int i = 0; i < kFontCardCount; ++i) {
        const size_t index = static_cast<size_t>(i);
        font_cards_[index].place(page_x_, y, content_w_, kFontCardHeight * s);
        font_meta_[index].layout(text_, theme, s, page_x_ + 12.0f * s, y + 12.0f * s,
                                 content_w_ - 24.0f * s);
        font_sample_[index].layout(text_, theme, s, page_x_ + 12.0f * s,
                                   font_meta_[index].rect.bottom() + 4.0f * s, 0.0f);
        y += (kFontCardHeight + kFontCardGap) * s;
    }
}

void Gallery::update_icon_filter() {
    const std::string needle = to_lower_ascii(icon_search_.text);
    icon_matches_.clear();
    for (int i = 0; i < icons::kIconEntryCount; ++i) {
        if (name_contains(icons::name_of(i), needle)) {
            icon_matches_.push_back(i);
        }
    }
}

void Gallery::layout_icons(float top, float s) {
    const Theme& theme = themes_.theme();

    // 搜索框在视口上方，右侧图标为内嵌放大镜。
    icon_search_.measure(s);
    icon_search_.place(page_x_, top);

    const int count = static_cast<int>(icon_matches_.size());
    const int rows = (count + kIconColumns - 1) / kIconColumns;
    const float cell_h = kIconCellHeight * s;
    const float row_gap = kIconsRowGap * s;
    const float content_h =
        rows > 0 ? static_cast<float>(rows) * (cell_h + row_gap) - row_gap : 0.0f;

    const float view_top = icon_search_.field.bottom() + kRowGap * s;
    const float available = std::max(height_ - view_top - kContentPadding * s, 120.0f * s);
    icons_view_.layout(page_x_, view_top, content_w_,
                       std::min(kIconsViewportHeight * s, available), content_h, s);

    // 关键词变化后把滚动位置拉回顶部。
    if (icon_search_.text != icon_filter_source_) {
        icon_filter_source_ = icon_search_.text;
        icons_view_.bar.set_value(0.0);
    }

    const Rect content = icons_view_.content_rect();
    const float gap = kTileGap * s;
    const float cell_w = std::max((content.w - gap * 2.0f) / 3.0f, 0.0f);

    for (IconTile& tile : icon_tiles_) {
        tile.visible = false;
    }

    Grid grid;
    grid.reset(content.x, content.y, kIconColumns, cell_w, cell_h, gap, row_gap);
    const float offset = icons_view_.offset();
    for (int i = 0; i < count; ++i) {
        IconTile& tile = icon_tiles_[static_cast<size_t>(icon_matches_[static_cast<size_t>(i)])];
        const Rect cell = grid.cell(i);
        const float y = cell.y - offset;
        tile.place(cell.x, y, cell_w, cell_h, s);
        tile.visible = y + cell_h > content.y && y < content.bottom();
    }

    note_.text = "没有匹配的图标，换个关键词试试";
    note_.face = kFaceRegular;
    note_.style = TextStyle::Body;
    note_.role = TextRole::Tertiary;
    note_.wrap = false;
    note_.layout(text_, theme, s, page_x_, view_top + 8.0f * s, 0.0f);
}

void Gallery::layout_about(float top, float s) {
    const Theme& theme = themes_.theme();
    paragraph_.text =
        "AetherUI 是一个 MIT 协议、可商用的开源 UI 库，定位为 Linux 可用的 Fluent 风格控件库。";
    paragraph_.face = kFaceRegular;
    paragraph_.style = TextStyle::BodyLarge;
    paragraph_.role = TextRole::Secondary;
    paragraph_.wrap = true;
    paragraph_.layout(text_, theme, s, page_x_, top, content_w_);

    note_.text = "Rendering: SDL3 + self-written CPU rasterizer · C++20 · MIT License";
    note_.face = kFaceMono;
    note_.style = TextStyle::Caption;
    note_.role = TextRole::Tertiary;
    note_.wrap = false;
    note_.layout(text_, theme, s, page_x_, paragraph_.rect.bottom() + kSectionGap * s, content_w_);
}

// ---------------------------------------------------------------- 绘制
void Gallery::draw(Surface& surface, double now) {
    layout(now);

    Painter painter(surface);
    const Theme& theme = themes_.theme();
    painter.fill_rect(Rect{0.0f, 0.0f, static_cast<float>(surface.width()),
                           static_cast<float>(surface.height())},
                      theme.solid_background_base);

    const Context ctx{painter, text_, theme, scale_, now, theme.dark};

    // 内容区（标题栏下方、导航右侧）。
    const Rect content_area{nav_.rect.right(), title_bar_.bar.h, width_ - nav_.rect.right(),
                            height_ - title_bar_.bar.h};
    painter.set_clip(intersect(surface.bounds(), cover(content_area)));
    draw_page(ctx);
    dropdowns_group_.draw_panels(ctx);
    painter.set_clip(surface.bounds());

    // 导航、标题栏在内容之上。
    nav_.draw(ctx);
    title_bar_.draw(ctx);

    // 右上角主题下拉框位于最顶层。
    const Recti saved = painter.clip();
    painter.set_clip(cover(content_area));
    theme_dropdown_.draw_field(ctx);
    painter.set_clip(saved);
}

void Gallery::draw_page(const Context& ctx) {
    header_.draw(ctx);
    switch (page_) {
        case Page::Home:
            paragraph_.draw(ctx);
            note_.draw(ctx);
            break;
        case Page::TextBox:
            for (const TextBox& box : boxes_) {
                box.draw(ctx);
            }
            break;
        case Page::Button:
            for (const Button& button : buttons_) {
                button.draw(ctx);
            }
            note_.draw(ctx);
            break;
        case Page::Dropdown:
            for (const Dropdown& dropdown : dropdowns_) {
                dropdown.draw_field(ctx);
            }
            note_.draw(ctx);
            for (const Dropdown& dropdown : placement_dropdowns_) {
                dropdown.draw_field(ctx);
            }
            footer_.draw(ctx);
            break;
        case Page::Toggle:
            for (const ToggleSwitch& toggle : toggles_) {
                toggle.draw(ctx);
            }
            break;
        case Page::Fonts:
            for (int i = 0; i < kFontCardCount; ++i) {
                const size_t index = static_cast<size_t>(i);
                font_cards_[index].draw(ctx);
                font_meta_[index].draw(ctx);
                font_sample_[index].draw(ctx);
            }
            break;
        case Page::Icons:
            draw_icons(ctx);
            break;
        case Page::About:
            paragraph_.draw(ctx);
            note_.draw(ctx);
            break;
    }
}

void Gallery::draw_icons(const Context& ctx) {
    icon_search_.draw(ctx);
    if (icon_matches_.empty()) {
        note_.draw(ctx);
    }

    const Recti saved = icons_view_.begin_clip(ctx);
    for (const IconTile& tile : icon_tiles_) {
        if (tile.visible) {
            tile.draw(ctx);
        }
    }
    icons_view_.end_clip(ctx, saved);
    icons_view_.draw_bar(ctx);
}

std::string Gallery::page_title() const {
    switch (page_) {
        case Page::Home:
            return "欢迎使用 AetherUI";
        case Page::TextBox:
            return "输入框";
        case Page::Button:
            return "按钮";
        case Page::Dropdown:
            return "下拉框";
        case Page::Toggle:
            return "开关";
        case Page::Fonts:
            return "字体";
        case Page::Icons:
            return "图标";
        case Page::About:
            return "关于 AetherUI";
    }
    return std::string();
}

std::string Gallery::page_caption() const {
    switch (page_) {
        case Page::Home:
            return std::string();
        case Page::TextBox:
            return "TextBox · 圆角 6 · 内边距 12 / 10 · Elevation 1 · 自定义右侧图标点击 " +
                   std::to_string(boxes_[kBoxIcon].right_clicks) + " 次";
        case Page::Button:
            return "Button · Primary / Standard · 图标 / 文字 / 禁用 · 四态（Normal / "
                   "PointerOver / Pressed / Disabled）· 自动内容尺寸 + 最小高度兜底";
        case Page::Dropdown:
            return "Dropdown × 7 · 4 个菜单项组合 + 3 种面板位置（上方 / 居中 / 下方）";
        case Page::Toggle:
            return "ToggleSwitch · 点击切换 · 按住滑块拖拽实时跟随，松开后按轨道中点吸附 · 拖拽放大 "
                   "/ 回弹 · 禁用态不可交互";
        case Page::Fonts:
            return "Fonts · 库内嵌字体直接调用，示例文本按对应字体族与字重渲染";
        case Page::Icons: {
            if (!icon_search_.text.empty()) {
                return "Icons · 搜索「" + icon_search_.text + "」命中 " +
                       std::to_string(icon_matches_.size()) + " / " +
                       std::to_string(icons::kIconEntryCount) + " 个图标";
            }
            std::string caption = "Icons · 内置 " + std::to_string(icons::kIconEntryCount) +
                                  " 个图标（icons::ALL 可直接遍历扩展）· ";
            if (copied_icon_ >= 0) {
                caption += std::string("已复制：") + icons::name_of(copied_icon_);
            } else {
                caption += "点击复制可写入图标常量名";
            }
            return caption;
        }
        case Page::About:
            return std::string();
    }
    return std::string();
}

// ---------------------------------------------------------------- 事件
void Gallery::handle_event(const Event& event, Window& window, double now) {
    now_ = now;
    host_.bind(&window);
    const UiContext ui{text_, themes_.theme(), scale_, now, themes_.dark(), &host_};

    switch (event.type) {
        case Event::Type::Resize:
            width_ = static_cast<float>(event.width);
            height_ = static_cast<float>(event.height);
            break;
        case Event::Type::SystemTheme:
            // 系统切换深浅色：只有「跟随系统」时才跟着变，手动选择不受影响。
            themes_.handle_event(event);
            theme_dropdown_.selected = themes_.preference();
            break;
        case Event::Type::WindowState:
            // 系统贴边吸附等非按钮途径改变最大化状态时，同步窗口按钮图标。
            title_bar_.maximized = event.maximized;
            break;
        default:
            break;
    }

    if (title_bar_.handle_event(event, ui)) {
        return;
    }
    if (event.type == Event::Type::KeyDown && event.key == SDLK_ESCAPE) {
        window.request_close();
        return;
    }
    nav_.handle_event(event, ui);
    dropdowns_group_.handle_event(event, ui);

    if (event.type == Event::Type::MouseDown) {
        focus_.press(event);
    }
    focus_.handle_event(event, ui);

    if (is_pointer_event(event)) {
        switch (page_) {
            case Page::TextBox:
                forward_events(boxes_, event, ui);
                break;
            case Page::Button:
                forward_events(buttons_, event, ui);
                break;
            case Page::Toggle:
                forward_events(toggles_, event, ui);
                break;
            case Page::Icons:
                icon_search_.handle_event(event, ui);
                forward_events(icon_tiles_, event, ui);
                icons_view_.handle_event(event, ui);
                break;
            default:
                break;
        }
    }

    if (event.type == Event::Type::MouseDown) {
        focus_.sync_ime(ui);
    }
}

}  // namespace aether
