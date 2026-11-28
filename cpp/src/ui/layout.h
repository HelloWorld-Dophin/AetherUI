//! 页面组合用的极简布局：竖向堆叠与等宽网格。

#pragma once

#include <algorithm>

#include "gfx/geometry.h"

namespace aether {

/// 内容页四周内边距（逻辑点）。
constexpr float kContentPadding = 32.0f;
/// 相邻区块之间的间距（逻辑点）。
constexpr float kSectionGap = 16.0f;

/// 竖向堆叠：`next` 返回当前区块顶部并推进光标。
struct VStack {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float gap = 0.0f;

    void reset(float left, float top, float w, float line_gap) {
        x = left;
        y = top;
        width = w;
        gap = line_gap;
    }

    float next(float height) {
        const float top = y;
        y += height + gap;
        return top;
    }

    float cursor() const { return y; }
};

/// 等宽列网格：按列数换行。
struct Grid {
    float x = 0.0f;
    float y = 0.0f;
    int columns = 1;
    float cell_width = 0.0f;
    float cell_height = 0.0f;
    float gap_x = 0.0f;
    float gap_y = 0.0f;

    void reset(float left, float top, int cols, float cell_w, float cell_h, float gx, float gy) {
        x = left;
        y = top;
        columns = std::max(cols, 1);
        cell_width = cell_w;
        cell_height = cell_h;
        gap_x = gx;
        gap_y = gy;
    }

    Rect cell(int index) const {
        const int col = index % columns;
        const int row = index / columns;
        return Rect{x + static_cast<float>(col) * (cell_width + gap_x),
                    y + static_cast<float>(row) * (cell_height + gap_y), cell_width, cell_height};
    }

    /// 给定行数时的内容总高度。
    float height_for_rows(int rows) const {
        if (rows <= 0) {
            return 0.0f;
        }
        return static_cast<float>(rows) * cell_height + static_cast<float>(rows - 1) * gap_y;
    }
};

}  // namespace aether
