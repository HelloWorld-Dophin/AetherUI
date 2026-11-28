//! AetherUI Gallery：CPU 光栅版演示窗口入口。
//!
//! 全部界面由自写光栅绘制：SDL3 只负责窗口与输入，表面即最终像素。
//! `--shot <path.ppm> [light|dark] [page]` 为离屏截图模式，用于核对像素。

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "app/gallery.h"
#include "gfx/surface.h"
#include "platform/window.h"
#include "text/font.h"

namespace {

double now_seconds() {
    static const double freq = static_cast<double>(SDL_GetPerformanceFrequency());
    return static_cast<double>(SDL_GetPerformanceCounter()) / freq;
}

std::vector<std::string> font_search_dirs() {
    std::vector<std::string> dirs;
    const std::string base = aether::executable_dir();
    const char* suffixes[] = {"assets/fonts/", "../assets/fonts/", "../../assets/fonts/",
                              "../../../assets/fonts/"};
    for (const char* suffix : suffixes) {
        dirs.push_back(base + suffix);
    }
    dirs.emplace_back("assets/fonts/");
    dirs.emplace_back(AETHERUI_FONT_DIR "/");
    return dirs;
}

std::string find_font(const std::vector<std::string>& dirs, const char* file) {
    for (const std::string& dir : dirs) {
        const std::string path = dir + file;
        if (FILE* probe = std::fopen(path.c_str(), "rb")) {
            std::fclose(probe);
            return path;
        }
    }
    return std::string();
}

bool write_ppm(const char* path, const aether::Surface& surface) {
    FILE* file = std::fopen(path, "wb");
    if (file == nullptr) {
        return false;
    }
    std::fprintf(file, "P6\n%d %d\n255\n", surface.width(), surface.height());
    std::vector<uint8_t> row(static_cast<size_t>(surface.width()) * 3);
    for (int y = 0; y < surface.height(); ++y) {
        const uint32_t* src = surface.row(y);
        for (int x = 0; x < surface.width(); ++x) {
            row[static_cast<size_t>(x) * 3 + 0] = static_cast<uint8_t>((src[x] >> 16) & 0xFF);
            row[static_cast<size_t>(x) * 3 + 1] = static_cast<uint8_t>((src[x] >> 8) & 0xFF);
            row[static_cast<size_t>(x) * 3 + 2] = static_cast<uint8_t>(src[x] & 0xFF);
        }
        std::fwrite(row.data(), 1, row.size(), file);
    }
    std::fclose(file);
    return true;
}

aether::Page page_from_name(const char* name) {
    if (std::strcmp(name, "textbox") == 0) {
        return aether::Page::TextBox;
    }
    if (std::strcmp(name, "button") == 0) {
        return aether::Page::Button;
    }
    if (std::strcmp(name, "dropdown") == 0) {
        return aether::Page::Dropdown;
    }
    if (std::strcmp(name, "toggle") == 0) {
        return aether::Page::Toggle;
    }
    if (std::strcmp(name, "fonts") == 0) {
        return aether::Page::Fonts;
    }
    if (std::strcmp(name, "icons") == 0) {
        return aether::Page::Icons;
    }
    if (std::strcmp(name, "about") == 0) {
        return aether::Page::About;
    }
    return aether::Page::Home;
}

}  // namespace

int main(int argc, char** argv) {
    aether::TextRenderer text;
    if (!text.init()) {
        std::fprintf(stderr, "FreeType init failed\n");
        return 1;
    }

    const std::vector<std::string> dirs = font_search_dirs();
    const char* font_files[] = {"SarasaUiSC-Regular.ttf", "SarasaUiSC-Light.ttf",
                                "SarasaUiSC-SemiBold.ttf", "NotoSansSymbols2-Regular.ttf",
                                "NotoEmoji-Regular.ttf", "JetBrainsMono-Regular.ttf",
                                "JetBrainsMono-SemiBold.ttf"};
    for (const char* file : font_files) {
        const std::string path = find_font(dirs, file);
        if (path.empty() || text.add_face(path.c_str()) < 0) {
            std::fprintf(stderr, "font load failed: %s\n", file);
        }
    }
    if (text.face_count() == 0) {
        std::fprintf(stderr, "no usable font, aborting\n");
        return 1;
    }

    // ---- 离屏截图：不开窗口，直接绘制一帧并写盘 ----
    if (argc >= 3 && std::strcmp(argv[1], "--shot") == 0) {
        constexpr int kWidth = 1120;
        constexpr int kHeight = 720;
        const bool light = argc >= 4 && std::strcmp(argv[3], "light") == 0;
        const aether::Page page = argc >= 5 ? page_from_name(argv[4]) : aether::Page::Home;

        aether::Gallery gallery(text);
        gallery.set_theme_mode(!light);
        gallery.set_viewport(static_cast<float>(kWidth), static_cast<float>(kHeight), 1.0f);
        gallery.set_page(page, 0.0);
        if (argc >= 6) {
            gallery.debug_open(argv[5], 0.0);
        }
        gallery.tick(0.0);

        std::vector<uint32_t> pixels(static_cast<size_t>(kWidth) * kHeight, 0);
        aether::Surface surface(pixels.data(), kWidth, kHeight, kWidth);
        gallery.draw(surface, 0.0);
        if (!write_ppm(argv[2], surface)) {
            std::fprintf(stderr, "ppm write failed\n");
            return 1;
        }
        std::printf("wrote %s\n", argv[2]);
        return 0;
    }

    aether::Window window;
    if (!window.create("AetherUI Gallery", 1120, 720)) {
        std::fprintf(stderr, "window creation failed: %s\n", SDL_GetError());
        return 1;
    }

    aether::Gallery gallery(text);
    gallery.set_theme_mode(true);

    const double frame_target = 1.0 / 60.0;
    double clock = 0.0;
    double last_frame = now_seconds();

    bool running = true;
    while (running) {
        const double frame_begin = now_seconds();
        const double dt = std::clamp(frame_begin - last_frame, 0.0, 0.1);
        last_frame = frame_begin;
        clock += dt;

        aether::Event event;
        while (window.poll(event)) {
            if (event.type == aether::Event::Type::Quit) {
                running = false;
                break;
            }
            gallery.handle_event(event, window, clock);
        }
        if (!running) {
            break;
        }

        gallery.tick(clock);

        aether::Surface surface = window.begin_frame();
        if (!surface.valid()) {
            SDL_Delay(4);
            continue;
        }
        gallery.set_viewport(static_cast<float>(surface.width()),
                             static_cast<float>(surface.height()), window.pixel_scale());
        gallery.draw(surface, clock);
        window.end_frame();

        const double spent = now_seconds() - frame_begin;
        if (spent < frame_target) {
            SDL_Delay(static_cast<uint32_t>((frame_target - spent) * 1000.0));
        }
    }
    return 0;
}
