//! Gallery：八个页面的演示窗口。
//!
//! 左侧 NavigationView + 自绘 TitleBar + 右上角主题下拉框；
//! 页面：主页 / 输入框 / 按钮 / 下拉框 / 开关 / 字体 / 图标 / 关于。

#pragma once

#include <string>
#include <vector>

#include "gfx/surface.h"
#include "platform/window.h"
#include "text/font.h"
#include "ui/button.h"
#include "ui/dropdown.h"
#include "ui/navigation.h"
#include "ui/scroll_bar.h"
#include "ui/text_box.h"
#include "ui/theme.h"
#include "ui/title_bar.h"
#include "ui/toggle_switch.h"

namespace aether {

enum class Page { Home, TextBox, Button, Dropdown, Toggle, Fonts, Icons, About };

class Gallery {
public:
    explicit Gallery(TextRenderer& text);

    void set_viewport(float width, float height, float scale);
    void set_page(Page page, double now);
    Page page() const { return page_; }

    void handle_event(const Event& event, Window& window, double now);
    /// 推进动画（无事件时也需调用）。
    void tick(double now);

    void draw(Surface& surface, double now);

    bool dark() const { return theme_.dark; }
    void toggle_theme(double now);
    /// 直接指定明暗（截图模式 / 系统主题变化）。
    void set_theme_mode(bool dark);

    /// 截图模式：强制展开某个下拉面板（"theme" / "d0".."d3" / "p0".."p2"）。
    void debug_open(const std::string& which, double now);

private:
    void build_widgets();
    void layout(double now);
    void apply_theme();
    void draw_background(Surface& surface) const;
    void draw_page(const Context& ctx, double now);
    void draw_dropdown_overlays(const Context& ctx, double now);

    /// 关闭所有展开的面板。
    void close_panels(double now);
    /// 把聚焦输入框的光标位置同步给输入法，避免候选窗跑出窗口。
    void sync_ime(Window& window);

    // ---- 页面绘制 ----
    void page_home(const Context& ctx);
    void page_text_box(const Context& ctx);
    void page_button(const Context& ctx);
    void page_dropdown(const Context& ctx);
    void page_toggle(const Context& ctx);
    void page_fonts(const Context& ctx);
    void page_icons(const Context& ctx);
    void page_about(const Context& ctx);

    TextRenderer& text_;
    Theme theme_;
    bool system_dark_ = true;
    int theme_preference_ = 0;  // 0 跟随系统 / 1 浅色 / 2 深色

    float width_ = 1120.0f;
    float height_ = 720.0f;
    float scale_ = 1.0f;

    Page page_ = Page::Home;
    NavigationView nav_;
    TitleBar title_bar_;
    Dropdown theme_dropdown_;

    TextBox text_box_;
    TextBox numeric_box_;
    TextBox disabled_box_;
    TextBox icon_box_;

    std::vector<Dropdown> dropdowns_;
    std::vector<Dropdown> placement_dropdowns_;
    std::string dropdown_notice_;

    std::vector<Button> buttons_;
    int button_clicks_ = 0;
    std::vector<Button> icon_buttons_;
    int copied_icon_ = -1;

    ToggleSwitch toggles_[2];

    ScrollBar icons_bar_;
    float icons_offset_ = 0.0f;

    float cursor_x_ = -1.0f;
    float cursor_y_ = -1.0f;
    bool mouse_down_ = false;
    /// 正在拖拽选区的输入框（左键按住时随鼠标移动活动端）。
    TextBox* drag_box_ = nullptr;
    /// 双击判定：上一次点击的输入框与时间。
    TextBox* last_text_box_ = nullptr;
    double last_text_click_ = -1.0;
    /// 输入法是否处于组字状态（组字期间不再重设候选窗锚点）。
    bool composing_ = false;
    /// 上一次上报给输入法的锚点，避免重复调用 Win32 IME 接口。
    bool ime_valid_ = false;
    float ime_x_ = 0.0f;
    float ime_y_ = 0.0f;
    float ime_h_ = 0.0f;
    double last_title_click_ = -1.0;
    float last_title_click_x_ = 0.0f;

    /// 页面坐标：内容区左上角。
    float page_x_ = 0.0f;
    float page_y_ = 0.0f;
    float content_w_ = 0.0f;

    // 图标下标（构造时解析一次）。
    int icon_home_ = -1;
    int icon_cursor_ = -1;
    int icon_control_button_ = -1;
    int icon_chevron_down_ = -1;
    int icon_toggle_ = -1;
    int icon_text_font_ = -1;
    int icon_grid_ = -1;
    int icon_info_ = -1;
    int icon_copy_ = -1;
    int icon_dismiss_ = -1;
};

}  // namespace aether
