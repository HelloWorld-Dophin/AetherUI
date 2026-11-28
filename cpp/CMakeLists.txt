cmake_minimum_required(VERSION 3.20)

project(AetherUI LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/utf-8 /W3 /MP)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
endif()

# ---------------------------------------------------------------- SDL3（静态）
# 只用到窗口 / 输入 / 输入法 / CPU 窗口表面，不创建任何 GPU 渲染器。
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_TESTS_LINK_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/SDL EXCLUDE_FROM_ALL)

# ------------------------------------------------------------- FreeType（静态）
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
add_subdirectory(third_party/freetype EXCLUDE_FROM_ALL)

if(TARGET SDL3::SDL3)
    set(AETHERUI_SDL_TARGET SDL3::SDL3)
elseif(TARGET SDL3-static)
    set(AETHERUI_SDL_TARGET SDL3-static)
else()
    message(FATAL_ERROR "找不到 SDL3 目标")
endif()

# --------------------------------------------------------------------- 主程序
add_executable(aetherui
    src/app/main.cpp
    src/app/app.cpp
    src/app/gallery.cpp
    src/gfx/surface.cpp
    src/gfx/raster.cpp
    src/gfx/painter.cpp
    src/platform/window.cpp
    src/platform/distro.cpp
    src/platform/paths.cpp
    src/platform/clipboard.cpp
    src/text/font.cpp
    src/ui/theme.cpp
    src/ui/theme_manager.cpp
    src/ui/widget.cpp
    src/ui/label.cpp
    src/ui/card.cpp
    src/ui/focus.cpp
    src/ui/icons.cpp
    src/ui/icon_data.cpp
    src/ui/svg.cpp
    src/ui/app_icon.cpp
    src/ui/button.cpp
    src/ui/icon_tile.cpp
    src/ui/toggle_switch.cpp
    src/ui/text_box.cpp
    src/ui/dropdown.cpp
    src/ui/navigation.cpp
    src/ui/scroll_bar.cpp
    src/ui/scroll_view.cpp
    src/ui/title_bar.cpp
)

target_include_directories(aetherui PRIVATE src)
target_link_libraries(aetherui PRIVATE ${AETHERUI_SDL_TARGET} freetype)
target_compile_definitions(aetherui PRIVATE
    AETHERUI_FONT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../assets/fonts")
