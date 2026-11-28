#include "platform/window.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include <SDL3/SDL.h>

#include "gfx/painter.h"

#if defined(_WIN32)
#include <windows.h>
#endif

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

/// 一个圆角方块的 alpha 覆盖率：4×4 超采样求出圆内比例，再按预乘语义逐通道缩放。
void mask_corner(uint32_t* pixels, int stride, int x0, int y0, int size, float cx, float cy,
                 float radius) {
    constexpr int kSamples = 4;
    constexpr float kStep = 1.0f / kSamples;
    constexpr int kTotal = kSamples * kSamples;
    for (int y = 0; y < size; ++y) {
        uint32_t* row = pixels + static_cast<size_t>(y0 + y) * stride + x0;
        for (int x = 0; x < size; ++x) {
            int inside = 0;
            for (int sy = 0; sy < kSamples; ++sy) {
                const float dy = static_cast<float>(y0 + y) + (sy + 0.5f) * kStep - cy;
                for (int sx = 0; sx < kSamples; ++sx) {
                    const float dx = static_cast<float>(x0 + x) + (sx + 0.5f) * kStep - cx;
                    if (dx * dx + dy * dy <= radius * radius) {
                        ++inside;
                    }
                }
            }
            if (inside == kTotal) {
                continue;  // 完全在圆内的像素保持原样
            }
            const float k = static_cast<float>(inside) / static_cast<float>(kTotal);
            const uint32_t p = row[x];
            const auto mul = [k](uint32_t v) {
                return static_cast<uint32_t>(static_cast<float>(v) * k + 0.5f);
            };
            row[x] = (mul((p >> 24) & 0xFF) << 24) | (mul((p >> 16) & 0xFF) << 16) |
                     (mul((p >> 8) & 0xFF) << 8) | mul(p & 0xFF);
        }
    }
}

}  // namespace

Window::Window() = default;

Window::~Window() { destroy(); }

bool Window::create(const char* title, int width, int height) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return false;
    }
    Uint64 flags =
        SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#if !defined(_WIN32)
    // 圆角靠窗口表面自带的 alpha 与合成器混合；X11 还要求该标志才能做区域裁剪。
    // Windows 走层叠窗口逐像素上屏，不需要（也不适合）该标志。
    flags |= SDL_WINDOW_TRANSPARENT;
#endif
    window_ = SDL_CreateWindow(title, width, height, flags);
    if (window_ == nullptr) {
        SDL_Quit();
        return false;
    }
    SDL_StartTextInput(window_);
    refresh_scale();
    // 发行版只探测一次（优先读缓存），界面层直接查询即可。
    distro_ = detect_distro();
    init_present();
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
#if defined(_WIN32)
    destroy_layered_surface();
#endif
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
    if ((SDL_GetWindowFlags(window_) & SDL_WINDOW_MINIMIZED) != 0) {
        return Surface{};  // 最小化时不做任何绘制，避免白白光栅化
    }
    refresh_scale();

#if defined(_WIN32)
    if (present_ == Present::Layered) {
        HWND hwnd = static_cast<HWND>(hwnd_);
        RECT rect{};
        RECT client{};
        if (hwnd == nullptr || !GetWindowRect(hwnd, &rect) || !GetClientRect(hwnd, &client)) {
            return Surface{};
        }
        const int ww = rect.right - rect.left;
        const int wh = rect.bottom - rect.top;
        const int cw = client.right;
        const int ch = client.bottom;
        if (ww <= 0 || wh <= 0 || cw <= 0 || ch <= 0 || !ensure_layered_surface(ww, wh)) {
            return Surface{};
        }
        POINT origin{0, 0};
        ClientToScreen(hwnd, &origin);
        present_offset_x_ = origin.x - rect.left;
        present_offset_y_ = origin.y - rect.top;
        present_client_w_ = cw;
        present_client_h_ = ch;
        if (present_offset_x_ != 0 || present_offset_y_ != 0 || cw != ww || ch != wh) {
            // 最大化时客户端四周的 8px 边条在屏幕外：清成透明，避免残留上一帧像素。
            uint32_t* pixels = static_cast<uint32_t*>(present_bits_);
            for (int y = 0; y < wh; ++y) {
                const bool in_client_y = y >= present_offset_y_ && y < present_offset_y_ + ch;
                uint32_t* row = pixels + static_cast<size_t>(y) * ww;
                if (!in_client_y) {
                    std::memset(row, 0, static_cast<size_t>(ww) * sizeof(uint32_t));
                    continue;
                }
                std::memset(row, 0, static_cast<size_t>(present_offset_x_) * sizeof(uint32_t));
                std::memset(row + present_offset_x_ + cw, 0,
                            static_cast<size_t>(ww - present_offset_x_ - cw) * sizeof(uint32_t));
            }
        }
        return Surface(static_cast<uint32_t*>(present_bits_) +
                           static_cast<size_t>(present_offset_y_) * ww + present_offset_x_,
                       cw, ch, ww);
    }
#endif

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
#if defined(_WIN32)
    if (present_ == Present::Layered) {
        if (present_bits_ == nullptr || present_client_w_ <= 0 || present_client_h_ <= 0) {
            return;
        }
        Surface frame(static_cast<uint32_t*>(present_bits_) +
                          static_cast<size_t>(present_offset_y_) * present_width_ + present_offset_x_,
                      present_client_w_, present_client_h_, present_width_);
        draw_border(frame);
        apply_corner_alpha(frame, true);
        present_layered();
        maybe_start_pending_drag();
        return;
    }
#endif

    SDL_Surface* window_surface = SDL_GetWindowSurface(window_);
    if (window_surface == nullptr) {
        return;
    }
    // 只有 ARGB8888 的窗口表面才保留 alpha；XRGB 会把它丢掉，圆角只能靠区域裁剪。
    const bool has_alpha = window_surface->format == SDL_PIXELFORMAT_ARGB8888;
    if (own_surface_ != nullptr) {
        Surface frame(static_cast<uint32_t*>(own_surface_->pixels), own_surface_->w, own_surface_->h,
                      own_surface_->pitch / 4);
        draw_border(frame);
        apply_corner_alpha(frame, has_alpha);
        SDL_SetSurfaceBlendMode(own_surface_, SDL_BLENDMODE_NONE);
        SDL_BlitSurface(own_surface_, nullptr, window_surface, nullptr);
    } else {
        Surface frame(static_cast<uint32_t*>(window_surface->pixels), window_surface->w,
                      window_surface->h, window_surface->pitch / 4);
        draw_border(frame);
        apply_corner_alpha(frame, has_alpha);
    }
    SDL_UpdateWindowSurface(window_);
    if (present_ == Present::Shape && shape_dirty_) {
        refresh_window_shape();
    }
}

void Window::set_corner_radius(float radius) {
    corner_radius_ = std::max(radius, 0.0f);
    shape_dirty_ = true;
}

void Window::set_border_color(Color color) { border_color_ = color; }

void Window::set_icon(const uint32_t* argb, int width, int height) {
    if (window_ == nullptr || argb == nullptr || width <= 0 || height <= 0) {
        return;
    }
    SDL_Surface* icon = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_ARGB8888);
    if (icon == nullptr) {
        return;
    }
    const size_t row_bytes = static_cast<size_t>(width) * sizeof(uint32_t);
    for (int y = 0; y < height; ++y) {
        std::memcpy(static_cast<uint8_t*>(icon->pixels) + static_cast<size_t>(y) * icon->pitch,
                    argb + static_cast<size_t>(y) * width, row_bytes);
    }
    SDL_SetWindowIcon(window_, icon);
    SDL_DestroySurface(icon);
}

void Window::draw_border(Surface& frame) const {
    if (border_color_.a <= 0.0f || !frame.valid()) {
        return;
    }
    // 边框逻辑宽度固定 1px，高 DPI 下换算成物理像素；贴外沿绘制，避免糊成两像素。
    const float width = std::max(scale_, 1.0f);
    const Rect outer{0.0f, 0.0f, static_cast<float>(frame.width()),
                     static_cast<float>(frame.height())};
    Painter painter(frame);
    painter.stroke_round_rect(outer, corner_radius_px(), width, border_color_);
}

float Window::corner_radius_px() const {
    if (maximized()) {
        return 0.0f;  // 最大化/全屏时贴着屏幕边，不该有圆角
    }
    return corner_radius_ * scale_;
}

void Window::apply_corner_alpha(Surface& frame, bool has_alpha) const {
    if (!has_alpha) {
        return;
    }
    const float radius = corner_radius_px();
    const int size = static_cast<int>(std::ceil(radius));
    const int w = frame.width();
    const int h = frame.height();
    if (radius <= 0.0f || size <= 0 || size * 2 > w || size * 2 > h) {
        return;
    }
    uint32_t* pixels = frame.pixels();
    const int stride = frame.stride();
    // 只有四个 r×r 方块里存在半透明像素，其余保持不透明，开销可以忽略。
    mask_corner(pixels, stride, 0, 0, size, radius, radius, radius);
    mask_corner(pixels, stride, w - size, 0, size, static_cast<float>(w) - radius, radius, radius);
    mask_corner(pixels, stride, 0, h - size, size, radius, static_cast<float>(h) - radius, radius);
    mask_corner(pixels, stride, w - size, h - size, size, static_cast<float>(w) - radius,
                static_cast<float>(h) - radius, radius);
}

void Window::init_present() {
#if defined(_WIN32)
    // Windows 的软件窗口表面走 GDI BitBlt，会丢掉 alpha：改用层叠窗口逐像素上屏。
    if (init_layered_present()) {
        present_ = Present::Layered;
        return;
    }
#endif
    const char* driver = SDL_GetCurrentVideoDriver();
    // X11 的软件表面是 24 位、没有 alpha，只能靠 XShape 区域裁剪（硬边）。
    if (driver != nullptr && std::strcmp(driver, "x11") == 0) {
        present_ = Present::Shape;
        return;
    }
    // Wayland / macOS 的窗口表面本身带 alpha，直接写像素即可。
    present_ = Present::Surface;
}

void Window::refresh_window_shape() {
    shape_dirty_ = false;
    if (window_ == nullptr) {
        return;
    }
    int w = 0;
    int h = 0;
    SDL_GetWindowSizeInPixels(window_, &w, &h);
    const float radius = corner_radius_px();
    if (radius <= 0.0f || w <= 0 || h <= 0) {
        SDL_SetWindowShape(window_, nullptr);
        return;
    }
    SDL_Surface* shape = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_ARGB8888);
    if (shape == nullptr) {
        return;
    }
    // 区域裁剪是二值的：逐行算出圆角矩形的可视区间，中间填不透明即可。
    uint32_t* pixels = static_cast<uint32_t*>(shape->pixels);
    const int pitch = shape->pitch / 4;
    for (int y = 0; y < h; ++y) {
        const float cy = static_cast<float>(y) + 0.5f;
        float inset = 0.0f;
        if (cy < radius) {
            const float d = radius - cy;
            inset = radius - std::sqrt(std::max(radius * radius - d * d, 0.0f));
        } else if (cy > static_cast<float>(h) - radius) {
            const float d = cy - (static_cast<float>(h) - radius);
            inset = radius - std::sqrt(std::max(radius * radius - d * d, 0.0f));
        }
        const int x0 = static_cast<int>(std::lround(inset));
        uint32_t* row = pixels + static_cast<size_t>(y) * pitch;
        for (int x = x0; x < w - x0; ++x) {
            row[x] = 0xFFFFFFFFu;
        }
    }
    SDL_SetWindowShape(window_, shape);
    SDL_DestroySurface(shape);
}

#if defined(_WIN32)
bool Window::init_layered_present() {
    SDL_PropertiesID props = SDL_GetWindowProperties(window_);
    HWND hwnd = static_cast<HWND>(
        SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
    if (hwnd == nullptr) {
        return false;
    }
    const LONG_PTR style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, style | WS_EX_LAYERED);
    if ((GetWindowLongPtr(hwnd, GWL_EXSTYLE) & WS_EX_LAYERED) == 0) {
        return false;
    }
    hwnd_ = hwnd;
    return true;
}

void Window::destroy_layered_surface() {
    if (present_dc_ != nullptr) {
        DeleteDC(static_cast<HDC>(present_dc_));
        present_dc_ = nullptr;
    }
    if (present_bitmap_ != nullptr) {
        DeleteObject(static_cast<HBITMAP>(present_bitmap_));
        present_bitmap_ = nullptr;
    }
    present_bits_ = nullptr;
    present_width_ = 0;
    present_height_ = 0;
}

bool Window::ensure_layered_surface(int width, int height) {
    if (present_bits_ != nullptr && present_width_ == width && present_height_ == height) {
        return true;
    }
    destroy_layered_surface();
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;  // 负数 = 自上而下，与界面坐标一致
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    HDC dc = CreateCompatibleDC(nullptr);
    void* bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (dc == nullptr || bitmap == nullptr || bits == nullptr) {
        if (bitmap != nullptr) {
            DeleteObject(bitmap);
        }
        if (dc != nullptr) {
            DeleteDC(dc);
        }
        return false;
    }
    SelectObject(dc, bitmap);
    present_dc_ = dc;
    present_bitmap_ = bitmap;
    present_bits_ = bits;
    present_width_ = width;
    present_height_ = height;
    return true;
}

void Window::present_layered() {
    HWND hwnd = static_cast<HWND>(hwnd_);
    HDC dc = static_cast<HDC>(present_dc_);
    if (hwnd == nullptr || dc == nullptr || present_bits_ == nullptr) {
        return;
    }
    int pw = 0;
    int ph = 0;
    SDL_GetWindowSizeInPixels(window_, &pw, &ph);
    if (pw <= 0 || ph <= 0) {
        return;
    }
    HDC screen = GetDC(nullptr);
    if (screen == nullptr) {
        return;
    }
    POINT src{0, 0};
    // psize 必须等于当前窗口尺寸（含无边框窗口最大化时屏外的 8px 边），
    // 这样系统认为几何没有变化，不会和最大化/DPI 重排互相拉扯。
    SIZE size{present_width_, present_height_};
    BLENDFUNCTION blend{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    UpdateLayeredWindow(hwnd, screen, nullptr, &size, dc, &src, 0, &blend, ULW_ALPHA);
    ReleaseDC(nullptr, screen);
}
#endif

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

bool Window::system_dark() {
    // SDL 内部已按平台分派：Windows 读个性化注册表、Linux 读 portal、macOS 读系统外观。
    return SDL_GetSystemTheme() == SDL_SYSTEM_THEME_DARK;
}

void Window::begin_drag() {
    if (window_ == nullptr) {
        return;
    }
#if defined(_WIN32)
    if (hwnd_ != nullptr) {
        // 走系统原生标题栏拖动：只有系统拖动才有 Win11 贴边分屏提示与 Aero Snap。
        if (maximized()) {
            // 最大化时先还原，并把窗口摆到鼠标仍压在标题栏的位置；
            // 真正开始拖动推迟到这一帧按新尺寸画完之后，避免拖动中露出旧尺寸的画面。
            int mw = 0;
            int mh = 0;
            SDL_GetWindowSize(window_, &mw, &mh);
            float gx = 0.0f;
            float gy = 0.0f;
            SDL_GetGlobalMouseState(&gx, &gy);
            const float span = static_cast<float>(mw) * scale_;
            const float rel_x = span > 0.0f ? std::clamp(last_mouse_x_ / span, 0.0f, 1.0f) : 0.5f;
            SDL_RestoreWindow(window_);
            int rw = 0;
            int rh = 0;
            SDL_GetWindowSize(window_, &rw, &rh);
            const int px = static_cast<int>(std::lround(gx - rel_x * static_cast<float>(rw) * scale_));
            const int py = static_cast<int>(std::lround(gy - last_mouse_y_));
            SDL_SetWindowPosition(window_, px, py);
            drag_pending_ = true;
            return;
        }
        ReleaseCapture();
        SendMessage(static_cast<HWND>(hwnd_), WM_NCLBUTTONDOWN, HTCAPTION, 0);
        return;
    }
#endif
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

#if defined(_WIN32)
void Window::maybe_start_pending_drag() {
    if (!drag_pending_ || hwnd_ == nullptr) {
        return;
    }
    drag_pending_ = false;
    ReleaseCapture();
    SendMessage(static_cast<HWND>(hwnd_), WM_NCLBUTTONDOWN, HTCAPTION, 0);
}
#endif

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
    shape_dirty_ = true;
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
                shape_dirty_ = true;
                out = Event{};
                out.type = Event::Type::Resize;
                out.width = raw.window.data1;
                out.height = raw.window.data2;
                return true;
            case SDL_EVENT_WINDOW_MAXIMIZED:
            case SDL_EVENT_WINDOW_RESTORED:
                // 最大化时圆角归零、还原时恢复，两者都要重算窗口形状。
                shape_dirty_ = true;
                out = Event{};
                out.type = Event::Type::WindowState;
                out.maximized = raw.type == SDL_EVENT_WINDOW_MAXIMIZED;
                return true;
            case SDL_EVENT_MOUSE_MOTION:
                last_mouse_x_ = raw.motion.x * scale_;
                last_mouse_y_ = raw.motion.y * scale_;
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
                last_mouse_x_ = raw.button.x * scale_;
                last_mouse_y_ = raw.button.y * scale_;
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
            case SDL_EVENT_SYSTEM_THEME_CHANGED:
                out = Event{};
                out.type = Event::Type::SystemTheme;
                out.dark = SDL_GetSystemTheme() == SDL_SYSTEM_THEME_DARK;
                return true;
            default:
                break;
        }
    }
    return false;
}

}  // namespace aether
