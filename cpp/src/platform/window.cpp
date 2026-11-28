#include "platform/window.h"

#include <algorithm>
#include <cmath>

#include <SDL3/SDL.h>

namespace aether {
namespace {

/// 边缘判定宽度（逻辑点）。
constexpr float kResizeBorder = 6.0f;
constexpr int kMinWidth = 480;
constexpr int kMinHeight = 360;

bool format_is_direct(SDL_PixelFormat format) {
    return format == SDL_PIXELFORMAT_ARGB8888 || format == SDL_PIXELFORMAT_XRGB8888;
}

int cursor_index_for_edges(int edges) {
    if (edges == 0) {
        return 0;
    }
    const bool left = (edges & 1) != 0;
    const bool right = (edges & 2) != 0;
    const bool top = (edges & 4) != 0;
    const bool bottom = (edges & 8) != 0;
    if ((left && top) || (right && bottom)) {
        return 1;
    }
    if ((right && top) || (left && bottom)) {
        return 2;
    }
    if (left || right) {
        return 3;
    }
    return 4;
}

SDL_SystemCursor system_cursor_for_index(int index) {
    switch (index) {
        case 1:
            return SDL_SYSTEM_CURSOR_NWSE_RESIZE;
        case 2:
            return SDL_SYSTEM_CURSOR_NESW_RESIZE;
        case 3:
            return SDL_SYSTEM_CURSOR_EW_RESIZE;
        case 4:
            return SDL_SYSTEM_CURSOR_NS_RESIZE;
        default:
            return SDL_SYSTEM_CURSOR_DEFAULT;
    }
}

}  // namespace

Window::Window() = default;

Window::~Window() { destroy(); }

bool Window::create(const char* title, int width, int height) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return false;
    }
    window_ = SDL_CreateWindow(title, width, height,
                               SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE |
                                   SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (window_ == nullptr) {
        SDL_Quit();
        return false;
    }
    SDL_StartTextInput(window_);
    refresh_scale();
    return true;
}

void Window::destroy() {
    for (SDL_Cursor*& cursor : cursors_) {
        if (cursor != nullptr) {
            SDL_DestroyCursor(cursor);
            cursor = nullptr;
        }
    }
    if (own_surface_ != nullptr) {
        SDL_DestroySurface(own_surface_);
        own_surface_ = nullptr;
    }
    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
        SDL_Quit();
    }
}

void Window::refresh_scale() {
    if (window_ == nullptr) {
        return;
    }
    int pw = 0;
    int ph = 0;
    int lw = 0;
    int lh = 0;
    SDL_GetWindowSizeInPixels(window_, &pw, &ph);
    SDL_GetWindowSize(window_, &lw, &lh);
    scale_ = (lw > 0 && pw > 0) ? static_cast<float>(pw) / static_cast<float>(lw) : 1.0f;
}

float Window::pixel_scale() {
    refresh_scale();
    return scale_;
}

Surface Window::begin_frame() {
    if (window_ == nullptr) {
        return Surface{};
    }
    SDL_Surface* window_surface = SDL_GetWindowSurface(window_);
    if (window_surface == nullptr) {
        return Surface{};
    }

    if (format_is_direct(window_surface->format)) {
        return Surface(static_cast<uint32_t*>(window_surface->pixels), window_surface->w,
                       window_surface->h, window_surface->pitch / 4);
    }

    if (own_surface_ == nullptr || own_surface_->w != window_surface->w ||
        own_surface_->h != window_surface->h) {
        if (own_surface_ != nullptr) {
            SDL_DestroySurface(own_surface_);
        }
        own_surface_ = SDL_CreateSurface(window_surface->w, window_surface->h,
                                         SDL_PIXELFORMAT_ARGB8888);
    }
    if (own_surface_ == nullptr) {
        return Surface{};
    }
    return Surface(static_cast<uint32_t*>(own_surface_->pixels), own_surface_->w, own_surface_->h,
                   own_surface_->pitch / 4);
}

void Window::end_frame() {
    if (window_ == nullptr) {
        return;
    }
    if (own_surface_ != nullptr) {
        SDL_Surface* window_surface = SDL_GetWindowSurface(window_);
        if (window_surface != nullptr) {
            SDL_BlitSurface(own_surface_, nullptr, window_surface, nullptr);
        }
    }
    SDL_UpdateWindowSurface(window_);
}

int Window::edge_at(float x, float y) const {
    if (window_ == nullptr || maximized()) {
        return 0;
    }
    int ww = 0;
    int wh = 0;
    SDL_GetWindowSize(window_, &ww, &wh);
    int edges = 0;
    if (x < kResizeBorder) {
        edges |= kEdgeLeft;
    }
    if (x > static_cast<float>(ww) - kResizeBorder) {
        edges |= kEdgeRight;
    }
    if (y < kResizeBorder) {
        edges |= kEdgeTop;
    }
    if (y > static_cast<float>(wh) - kResizeBorder) {
        edges |= kEdgeBottom;
    }
    return edges;
}

void Window::update_cursor(int edges) {
    const int index = cursor_index_for_edges(edges);
    if (index == cursor_index_) {
        return;
    }
    cursor_index_ = index;
    if (cursors_[index] == nullptr) {
        cursors_[index] = SDL_CreateSystemCursor(system_cursor_for_index(index));
    }
    SDL_SetCursor(cursors_[index]);
}

void Window::set_text_input_area(float x, float y, float w, float h) {
    if (window_ == nullptr) {
        return;
    }
    // SDL 以「窗口坐标（逻辑点）」接收锚点，界面层给的是物理像素。
    SDL_Rect rect{};
    rect.x = static_cast<int>(std::lround(x / scale_));
    rect.y = static_cast<int>(std::lround(y / scale_));
    rect.w = std::max(1, static_cast<int>(std::lround(w / scale_)));
    rect.h = std::max(1, static_cast<int>(std::lround(h / scale_)));
    SDL_SetTextInputArea(window_, &rect, 0);
}

void Window::clear_text_input_area() {
    if (window_ != nullptr) {
        SDL_SetTextInputArea(window_, nullptr, 0);
    }
}

void Window::begin_drag() {
    if (window_ == nullptr) {
        return;
    }
    if (maximized()) {
        SDL_RestoreWindow(window_);
    }
    float gx = 0.0f;
    float gy = 0.0f;
    SDL_GetGlobalMouseState(&gx, &gy);
    SDL_GetWindowPosition(window_, &frame_x_, &frame_y_);
    drag_mouse_x_ = gx;
    drag_mouse_y_ = gy;
    dragging_ = true;
    SDL_CaptureMouse(true);
}

void Window::begin_resize(int edges) {
    if (window_ == nullptr) {
        return;
    }
    float gx = 0.0f;
    float gy = 0.0f;
    SDL_GetGlobalMouseState(&gx, &gy);
    SDL_GetWindowPosition(window_, &frame_x_, &frame_y_);
    SDL_GetWindowSize(window_, &frame_w_, &frame_h_);
    drag_mouse_x_ = gx;
    drag_mouse_y_ = gy;
    resize_edges_ = edges;
    resizing_ = true;
    SDL_CaptureMouse(true);
}

void Window::update_chrome_drag() {
    float gx = 0.0f;
    float gy = 0.0f;
    SDL_GetGlobalMouseState(&gx, &gy);
    const int dx = static_cast<int>(std::lround(gx - drag_mouse_x_));
    const int dy = static_cast<int>(std::lround(gy - drag_mouse_y_));

    if (dragging_) {
        SDL_SetWindowPosition(window_, frame_x_ + dx, frame_y_ + dy);
        return;
    }
    if (!resizing_) {
        return;
    }

    int x = frame_x_;
    int y = frame_y_;
    int w = frame_w_;
    int h = frame_h_;
    if ((resize_edges_ & kEdgeLeft) != 0) {
        x += dx;
        w -= dx;
    }
    if ((resize_edges_ & kEdgeRight) != 0) {
        w += dx;
    }
    if ((resize_edges_ & kEdgeTop) != 0) {
        y += dy;
        h -= dy;
    }
    if ((resize_edges_ & kEdgeBottom) != 0) {
        h += dy;
    }

    const int clamped_w = std::max(w, kMinWidth);
    const int clamped_h = std::max(h, kMinHeight);
    if ((resize_edges_ & kEdgeLeft) != 0) {
        x = frame_x_ + (frame_w_ - clamped_w);
    }
    if ((resize_edges_ & kEdgeTop) != 0) {
        y = frame_y_ + (frame_h_ - clamped_h);
    }
    SDL_SetWindowPosition(window_, x, y);
    SDL_SetWindowSize(window_, clamped_w, clamped_h);
}

void Window::end_chrome_drag() {
    dragging_ = false;
    resizing_ = false;
    resize_edges_ = 0;
    SDL_CaptureMouse(false);
    cursor_index_ = -1;
    update_cursor(0);
}

void Window::minimize() {
    if (window_ != nullptr) {
        SDL_MinimizeWindow(window_);
    }
}

bool Window::maximized() const {
    if (window_ == nullptr) {
        return false;
    }
    return (SDL_GetWindowFlags(window_) & SDL_WINDOW_MAXIMIZED) != 0;
}

void Window::toggle_maximize() {
    if (window_ == nullptr) {
        return;
    }
    if (maximized()) {
        SDL_RestoreWindow(window_);
    } else {
        SDL_MaximizeWindow(window_);
    }
}

void Window::request_close() {
    SDL_Event quit{};
    quit.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&quit);
}

bool Window::poll(Event& out) {
    if (window_ == nullptr) {
        return false;
    }
    SDL_Event raw;
    while (SDL_PollEvent(&raw)) {
        switch (raw.type) {
            case SDL_EVENT_QUIT:
                out = Event{};
                out.type = Event::Type::Quit;
                return true;
            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                out = Event{};
                out.type = Event::Type::Resize;
                out.width = raw.window.data1;
                out.height = raw.window.data2;
                return true;
            case SDL_EVENT_MOUSE_MOTION:
                if (dragging_ || resizing_) {
                    update_chrome_drag();
                    continue;  // 拖动期间的移动不再作为界面事件下发
                }
                update_cursor(edge_at(raw.motion.x, raw.motion.y));
                out = Event{};
                out.type = Event::Type::MouseMove;
                out.x = raw.motion.x * scale_;
                out.y = raw.motion.y * scale_;
                return true;
            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                if (raw.button.button == SDL_BUTTON_LEFT && !dragging_ && !resizing_) {
                    const int edges = edge_at(raw.button.x, raw.button.y);
                    if (edges != 0) {
                        begin_resize(edges);
                        continue;  // 边缘按下属于窗口拉伸，不交给界面层
                    }
                }
                out = Event{};
                out.type = Event::Type::MouseDown;
                out.x = raw.button.x * scale_;
                out.y = raw.button.y * scale_;
                out.key = static_cast<int>(raw.button.button);
                return true;
            }
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (dragging_ || resizing_) {
                    end_chrome_drag();
                    continue;
                }
                out = Event{};
                out.type = Event::Type::MouseUp;
                out.x = raw.button.x * scale_;
                out.y = raw.button.y * scale_;
                out.key = static_cast<int>(raw.button.button);
                return true;
            case SDL_EVENT_MOUSE_WHEEL:
                out = Event{};
                out.type = Event::Type::Wheel;
                out.x = raw.wheel.mouse_x * scale_;
                out.y = raw.wheel.mouse_y * scale_;
                out.wheel = raw.wheel.y * 48.0f;
                return true;
            case SDL_EVENT_KEY_DOWN:
                out = Event{};
                out.type = Event::Type::KeyDown;
                out.key = static_cast<int>(raw.key.key);
                out.mods = static_cast<int>(raw.key.mod);
                return true;
            case SDL_EVENT_TEXT_INPUT:
                out = Event{};
                out.type = Event::Type::Text;
                out.text = raw.text.text != nullptr ? raw.text.text : "";
                return true;
            case SDL_EVENT_TEXT_EDITING:
                out = Event{};
                out.type = Event::Type::Editing;
                out.text = raw.edit.text != nullptr ? raw.edit.text : "";
                return true;
            default:
                break;
        }
    }
    return false;
}

std::string executable_dir() {
    // SDL3 的 SDL_GetBasePath 返回内部缓存，调用方不得释放。
    const char* base = SDL_GetBasePath();
    return base != nullptr ? std::string(base) : std::string();
}

}  // namespace aether
