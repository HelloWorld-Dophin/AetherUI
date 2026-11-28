//! SDL3 窗口与输入。渲染完全走 CPU：直接写窗口表面，不创建任何 GPU 渲染器。
//!
//! 窗口使用无边框模式，标题栏由界面层自绘。拖动与边缘拉伸都由本层自己跟随鼠标
//! （`SDL_SetWindowPosition` / `SDL_SetWindowSize`），**不进入 Windows 的模态
//! move/size 循环**，因此拖动过程中渲染循环不会停摆。

#pragma once

#include <string>

#include "gfx/geometry.h"
#include "gfx/surface.h"

struct SDL_Window;
struct SDL_Surface;
struct SDL_Cursor;

namespace aether {

struct Event {
    enum class Type {
        None,
        Quit,
        Resize,
        MouseMove,
        MouseDown,
        MouseUp,
        Wheel,
        KeyDown,
        Text,
        /// 输入法组字过程中的实时串（为空表示组字结束）。
        Editing
    };

    Type type = Type::None;
    /// 鼠标位置，已换算成像素坐标。
    float x = 0.0f;
    float y = 0.0f;
    /// 滚轮增量（像素，正数向上）。
    float wheel = 0.0f;
    int width = 0;
    int height = 0;
    int key = 0;
    /// 修饰键状态（SDL_KMOD_* 位组合：Ctrl / Shift / Alt）。
    int mods = 0;
    std::string text;
};

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

private:
    enum EdgeFlags { kEdgeLeft = 1, kEdgeRight = 2, kEdgeTop = 4, kEdgeBottom = 8 };

    void refresh_scale();
    int edge_at(float x, float y) const;
    void begin_resize(int edges);
    void update_chrome_drag();
    void end_chrome_drag();
    void update_cursor(int edges);

    SDL_Window* window_ = nullptr;
    SDL_Surface* own_surface_ = nullptr;
    float scale_ = 1.0f;

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

/// 可执行文件所在目录（以路径分隔符结尾）。
std::string executable_dir();

}  // namespace aether
