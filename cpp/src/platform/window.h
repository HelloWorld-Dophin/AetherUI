//! SDL3 窗口与输入。渲染完全走 CPU：直接写窗口表面，不创建任何 GPU 渲染器。
//!
//! 窗口使用无边框模式，标题栏由界面层自绘。拖动与边缘拉伸都由本层自己跟随鼠标
//! （`SDL_SetWindowPosition` / `SDL_SetWindowSize`），**不进入 Windows 的模态
//! move/size 循环**，因此拖动过程中渲染循环不会停摆。

#pragma once

#include <string>

#include "gfx/geometry.h"
#include "gfx/surface.h"
#include "platform/distro.h"
#include "platform/paths.h"
#include "ui/input.h"

struct SDL_Window;
struct SDL_Surface;
struct SDL_Cursor;

namespace aether {

class Window {
public:
    Window();
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool create(const char* title, int width, int height);
    void destroy();

    /// 取本帧可写的像素缓冲（直接指向窗口表面，常见平台下零拷贝）。
    Surface begin_frame();
    void end_frame();

    /// 取一个事件；返回 false 表示队列已空。
    bool poll(Event& out);

    /// 逻辑点 → 物理像素的缩放系数（高 DPI 下为 1.25 / 1.5 / 2 等）。
    float pixel_scale();
    void* native() const { return window_; }

    // ---- 无边框窗口的窗口级操作（标题栏本身由界面层绘制）----
    /// 在标题栏按下时调用：后续鼠标移动会拖动窗口，直到松开。
    void begin_drag();
    void minimize();
    void toggle_maximize();
    bool maximized() const;
    void request_close();

    /// 告诉输入法候选窗应该出现在哪里（参数为界面像素坐标）。
    void set_text_input_area(float x, float y, float w, float h);
    /// 没有输入框聚焦时清空候选窗锚点。
    void clear_text_input_area();

    /// 窗口圆角半径（逻辑点，内部按 DPI 换算）。0 表示直角；最大化时自动按 0 处理。
    void set_corner_radius(float radius);

    /// 窗口外沿 1px 边框颜色（由界面层按主题传入，alpha 为 0 表示不画）。
    void set_border_color(Color color);

    /// 系统当前是否为深色（Windows 读个性化设置、Linux 读 portal，均由 SDL 处理）。
    /// 无法判定时按浅色处理。
    bool system_dark();

    /// 发行版（启动时探测并写入缓存），deepin 后续用于取 DDE 主题色。
    Distro distro() const { return distro_; }

    /// 设置任务栏 / Alt-Tab 图标（`argb` 为 `0xAARRGGBB` 直通 alpha，行优先）。
    void set_icon(const uint32_t* argb, int width, int height);

private:
    enum EdgeFlags { kEdgeLeft = 1, kEdgeRight = 2, kEdgeTop = 4, kEdgeBottom = 8 };

    /// 圆角的上屏方式：Windows 用层叠窗口保 alpha，X11 用区域裁剪，其余平台窗口表面自带 alpha。
    enum class Present { Surface, Layered, Shape };

    void refresh_scale();
    int edge_at(float x, float y) const;
    void begin_resize(int edges);
    void update_chrome_drag();
    void end_chrome_drag();
    void update_cursor(int edges);

    void init_present();
    /// AETHERUI_LOG=1 时把后端信息打到 stderr，方便排查平台差异。
    void log_backend() const;
    /// 沿窗口外沿描一圈圆角边框（内容画完之后、圆角蒙版之前）。
    void draw_border(Surface& frame) const;
    void apply_corner_alpha(Surface& frame, bool has_alpha) const;
    float corner_radius_px() const;
    /// X11 专用：把圆角矩形作为窗口可视区域（二值裁剪，硬边）。
    void refresh_window_shape();

#if defined(_WIN32)
    bool init_layered_present();
    void destroy_layered_surface();
    bool ensure_layered_surface(int width, int height);
    void present_layered();
    /// 等当前帧按新尺寸画完后再进入系统拖动（否则拖动中显示的是旧尺寸画面）。
    void maybe_start_pending_drag();
    void* hwnd_ = nullptr;
    void* present_dc_ = nullptr;
    void* present_bitmap_ = nullptr;
    void* present_bits_ = nullptr;
    int present_width_ = 0;
    int present_height_ = 0;
    /// 客户端区域在层叠缓冲区中的偏移（无边框可拉伸窗口最大化时四周有 8px 屏外边）。
    int present_offset_x_ = 0;
    int present_offset_y_ = 0;
    int present_client_w_ = 0;
    int present_client_h_ = 0;
    bool drag_pending_ = false;
#endif

    /// 最近一次鼠标位置（物理像素），用于最大化还原时把标题栏摆回鼠标下方。
    float last_mouse_x_ = 0.0f;
    float last_mouse_y_ = 0.0f;

    SDL_Window* window_ = nullptr;
    SDL_Surface* own_surface_ = nullptr;
    float scale_ = 1.0f;
    Distro distro_ = Distro::Other;
    Present present_ = Present::Surface;
    /// 没有 alpha 也没有形状裁剪的窗口（X11 用 AETHERUI_X11_NO_ALPHA=1 时）。
    bool opaque_frame_ = false;
    float corner_radius_ = 0.0f;
    bool shape_dirty_ = true;
    Color border_color_{};

    bool dragging_ = false;
    bool resizing_ = false;
    int resize_edges_ = 0;
    float drag_mouse_x_ = 0.0f;
    float drag_mouse_y_ = 0.0f;
    int frame_x_ = 0;
    int frame_y_ = 0;
    int frame_w_ = 0;
    int frame_h_ = 0;

    /// 0=默认，1=西北/东南，2=东北/西南，3=左右，4=上下。
    int cursor_index_ = -1;
    SDL_Cursor* cursors_[5] = {};
};

}  // namespace aether
