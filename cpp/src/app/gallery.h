//! Gallery：八个页面的演示窗口。
//!
//! 左侧 NavigationView + 自绘 TitleBar + 右上角主题下拉框；
//! 页面：主页 / 输入框 / 按钮 / 下拉框 / 开关 / 字体 / 图标 / 关于。
//! 本文件只做「组合 + 调用控件」：命中测试、事件分发、焦点、滚动、主题
//! 全部由 ui 层的控件自己完成。

#pragma once

#include <string>
#include <vector>

#include "gfx/surface.h"
#include "platform/window.h"
#include "text/font.h"
#include "ui/button.h"
#include "ui/card.h"
#include "ui/dropdown.h"
#include "ui/focus.h"
#include "ui/icon_tile.h"
#include "ui/label.h"
#include "ui/navigation.h"
#include "ui/scroll_view.h"
#include "ui/text_box.h"
#include "ui/theme_manager.h"
#include "ui/title_bar.h"
#include "ui/toggle_switch.h"

namespace aether {

enum class Page { Home, TextBox, Button, Dropdown, Toggle, Fonts, Icons, About };

class Gallery {
public:
    /// `title` 与 `icon` 由 App 统一给出（与窗口标题 / 任务栏图标同源）。
    Gallery(TextRenderer& text, const std::string& title, const AppIcon& icon);

    void set_viewport(float width, float height, float scale);
    void set_page(Page page, double now);
    Page page() const { return page_; }

    void handle_event(const Event& event, Window& window, double now);
    void draw(Surface& surface, double now);

    bool dark() const { return themes_.dark(); }
    /// 窗口外沿 1px 边框的颜色（跟随主题，具体绘制由窗口控件负责）。
    Color window_border_color() const { return themes_.theme().window_stroke; }
    void toggle_theme(double now);
    /// 直接指定明暗（截图模式 / 系统主题变化）。
    void set_theme_mode(bool dark);

    /// 截图模式：强制展开某个下拉面板（"theme" / "d0".."d3" / "p0".."p2"）。
    void debug_open(const std::string& which, double now);

private:
    /// 把控件的宿主请求转给窗口。
    class Host : public WidgetHost {
    public:
        void bind(Window* window) { window_ = window; }

        void request_close() override {
            if (window_ != nullptr) {
                window_->request_close();
            }
        }
        void minimize() override {
            if (window_ != nullptr) {
                window_->minimize();
            }
        }
        void toggle_maximize() override {
            if (window_ != nullptr) {
                window_->toggle_maximize();
            }
        }
        bool maximized() const override { return window_ != nullptr && window_->maximized(); }
        void begin_drag() override {
            if (window_ != nullptr) {
                window_->begin_drag();
            }
        }
        void set_text_input_area(float x, float y, float w, float h) override {
            if (window_ != nullptr) {
                window_->set_text_input_area(x, y, w, h);
            }
        }
        void clear_text_input_area() override {
            if (window_ != nullptr) {
                window_->clear_text_input_area();
            }
        }
        bool system_dark() const override { return window_ == nullptr || window_->system_dark(); }

    private:
        Window* window_ = nullptr;
    };

    void build_widgets();

    // ---- 布局 ----
    void layout(double now);
    void layout_home(float top, float s);
    void layout_text_box(float top, float s);
    void layout_button(float top, float s);
    void layout_dropdown(float top, float s);
    void layout_toggle(float top, float s);
    void layout_fonts(float top, float s);
    void layout_icons(float top, float s);
    void layout_about(float top, float s);
    /// 按搜索框内容重建 `icon_matches_`。
    void update_icon_filter();

    // ---- 绘制 ----
    void draw_page(const Context& ctx);
    void draw_icons(const Context& ctx);

    std::string page_title() const;
    std::string page_caption() const;
    /// 关闭所有展开的面板（切页时用）。
    void close_panels(double now);

    TextRenderer& text_;
    ThemeManager themes_;
    Host host_;
    FocusManager focus_;
    DropdownGroup dropdowns_group_;

    float width_ = 1120.0f;
    float height_ = 720.0f;
    float scale_ = 1.0f;

    Page page_ = Page::Home;
    NavigationView nav_;
    TitleBar title_bar_;
    Dropdown theme_dropdown_;

    std::vector<TextBox> boxes_;
    std::vector<Dropdown> dropdowns_;
    std::vector<Dropdown> placement_dropdowns_;
    std::vector<Button> buttons_;
    std::vector<IconTile> icon_tiles_;
    ToggleSwitch toggles_[2];

    /// 图标页搜索框（右侧为内嵌放大镜），按名称过滤图标卡片。
    TextBox icon_search_;
    /// 当前命中的图标下标（每帧按关键词重建）。
    std::vector<int> icon_matches_;
    /// 上一次过滤用的关键词（变化时把滚动位置拉回顶部）。
    std::string icon_filter_source_;

    ScrollView icons_view_;
    std::vector<Card> font_cards_;
    std::vector<Label> font_meta_;
    std::vector<Label> font_sample_;

    PageHeader header_;
    Label paragraph_;
    Label note_;
    Label footer_;

    int button_clicks_ = 0;
    int copied_icon_ = -1;
    std::string dropdown_notice_;

    /// 页面坐标：内容区左上角。
    float page_x_ = 0.0f;
    float page_y_ = 0.0f;
    float content_w_ = 0.0f;
    /// 当前帧时刻（控件回调里切页要用）。
    double now_ = 0.0;

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
