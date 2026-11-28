//! 绘制指令层：把「圆角矩形 / 描边 / 阴影 / 文本」翻译成覆盖率蒙版 + 预乘合成。
//!
//! 所有坐标都是像素坐标；缩放由调用方负责（`scale` 在布局层统一乘好）。

#pragma once

#include <cstdint>

#include "gfx/geometry.h"
#include "gfx/raster.h"
#include "gfx/surface.h"

namespace aether {

class Painter {
public:
    explicit Painter(Surface& target);

    Surface& target() { return target_; }

    void set_clip(Recti clip);
    Recti clip() const { return clip_; }

    void fill_rect(Rect r, Color c);
    void fill_round_rect(Rect r, float radius, Color c);
    void fill_circle(float cx, float cy, float radius, Color c);
    void stroke_round_rect(Rect r, float radius, float width, Color c);
    /// 直线段（带端帽），用于标题栏按钮的小图标。
    void stroke_line(float x0, float y0, float x1, float y1, float width, Color c);

    /// 径向渐隐色晕（模拟 Mica 对壁纸的低频采样）。
    void fill_radial_glow(float cx, float cy, float radius, Color c);

    /// Fluent 软阴影：圆角矩形蒙版 + 盒式模糊 + 偏移合成。
    void draw_shadow(Rect r, float radius, Color c, float sigma, float dx, float dy);

    /// 蒙版填充（覆盖率来自 [#Mask]）。
    void fill_mask(const Mask& mask, Color c);

    /// 直接填充任意路径（图标等复杂形状），非零环绕规则。
    void fill_path(const Path& path, Color c);

    /// 圆角矩形填充，但只在 `clip_rect` 覆盖的范围内生效（用于高亮条被圆角裁剪）。
    void fill_round_rect_clip(Rect r, float radius, Rect clip_rect, Color c);

    /// 单通道 alpha 位图叠加（字形、图标蒙版）。
    void blit_alpha(int x, int y, const uint8_t* src, int w, int h, int stride, Color c);

private:
    Surface& target_;
    Recti clip_;
    Path path_;
    Rasterizer rasterizer_;
    Mask mask_;
};

}  // namespace aether
