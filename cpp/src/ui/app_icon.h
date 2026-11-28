//! AppIcon：应用图标来源 —— 品牌图标、内置 Fluent 图标名或 SVG 文件。
//!
//! 传 `"AETHERUI"` 用品牌图标（强调色圆角方块 + 白色星芒），
//! 传 `"SPARKLE"` 用内置图标，传 `"sparkle.svg"` / 文件路径用 SVG；
//! 同一个对象既给标题栏自绘 logo，也能栅格化成原生窗口（任务栏）图标。

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "gfx/painter.h"
#include "ui/svg.h"

namespace aether {

class AppIcon {
public:
    AppIcon() = default;
    /// 构造时立即解析来源；`valid()` 表示是否可用。
    explicit AppIcon(const std::string& source);

    bool load();
    bool valid() const { return brand_ || builtin_ >= 0 || svg_.valid(); }
    const std::string& source() const { return source_; }

    /// 绘制到 (x, y)（像素坐标），`size` 为逻辑边长，`scale` 为 DPI 系数。
    void draw(Painter& painter, float x, float y, float size, float scale, Color color,
              bool dark) const;

    /// 栅格化成 `0xAARRGGBB` 直通 alpha 像素（行优先），用于原生窗口图标。
    bool rasterize(int px, Color color, std::vector<uint32_t>& out) const;

private:
    /// 品牌图标：强调色圆角方块打底 + 白色星芒。
    void paint_brand(Painter& painter, float x, float y, float px, Color color,
                     bool dark) const;

    std::string source_;
    bool brand_ = false;
    int builtin_ = -1;
    svg::Image svg_;
};

}  // namespace aether
