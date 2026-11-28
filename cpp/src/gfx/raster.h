//! 自写光栅核心：路径 → 覆盖率蒙版。
//!
//! 算法：扫描线 + 非零环绕规则。纵向 8 个子扫描线采样，
//! 横向按解析面积精确累加，因此斜边与圆角在 1:1 缩放下也能得到
//! 干净的抗锯齿（不是粗暴的 4x4 超采样）。

#pragma once

#include <vector>

#include "gfx/geometry.h"

namespace aether {

/// 覆盖率蒙版：`a[y - y0][x - x0]` 取值为 0..1。
struct Mask {
    int x0 = 0;
    int y0 = 0;
    int w = 0;
    int h = 0;
    std::vector<float> a;

    bool empty() const { return w <= 0 || h <= 0; }
    float* row(int y) { return a.data() + static_cast<size_t>(y - y0) * w; }
    const float* row(int y) const { return a.data() + static_cast<size_t>(y - y0) * w; }
};

struct Edge {
    float x0 = 0.0f;
    float y0 = 0.0f;
    float x1 = 0.0f;
    float y1 = 0.0f;
};

class Path {
public:
    void move_to(float x, float y);
    void line_to(float x, float y);
    void quad_to(float cx, float cy, float x, float y);
    void cubic_to(float cx1, float cy1, float cx2, float cy2, float x, float y);

    void add_rect(Rect r, bool reverse = false);
    void add_round_rect(Rect r, float radius, bool reverse = false);
    void add_circle(float cx, float cy, float radius, bool reverse = false);

    void clear();
    bool empty() const { return subpaths_.empty(); }

    /// 摊平成线段边表（子路径隐式闭合）。
    void build_edges(std::vector<Edge>& out) const;

private:
    Point current() const;
    std::vector<std::vector<Point>> subpaths_;
};

class Rasterizer {
public:
    /// 把路径栅格化到 `out`（只覆盖 `clip` 内的像素）。返回是否产生了非空蒙版。
    bool rasterize(const Path& path, Recti clip, Mask& out);

private:
    struct Crossing {
        float x = 0.0f;
        int dir = 0;
    };

    std::vector<Edge> edges_;
    std::vector<Crossing> crossings_;
};

/// 三次盒式模糊（3 次迭代近似高斯），就地处理蒙版。
void blur_mask(Mask& mask, float sigma);

}  // namespace aether
