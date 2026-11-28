//! 极简 SVG 路径加载：只解析 `<path d="...">` 与 `viewBox`。
//!
//! 支持 M/L/H/V/C/S/Q/T/Z 及其相对形式；`fill-rule` 统一按非零环绕处理，
//! `<circle>` / `<rect>` / `transform` 等不在支持范围内。
//! 图标文件在启动时解析一次，绘制时按目标尺寸缩放，不再读盘。

#pragma once

#include <string>
#include <vector>

#include "gfx/raster.h"

namespace aether {
namespace svg {

/// 一条路径指令；坐标保留 viewBox 原始单位，绘制时才缩放。
struct Segment {
    enum class Kind { Move, Line, Quad, Cubic, Close };

    Kind kind = Kind::Move;
    /// Move/Line 用 p[0..1]，Quad 用 p[0..3]，Cubic 用 p[0..5]。
    float p[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
};

struct Image {
    std::vector<Segment> segments;
    float view_x = 0.0f;
    float view_y = 0.0f;
    float view_w = 0.0f;
    float view_h = 0.0f;

    bool valid() const { return !segments.empty() && view_w > 0.0f && view_h > 0.0f; }
    void clear() { *this = Image{}; }

    /// 缩放到 `size` 见方并平移到 (x, y)，结果追加到 `path`。
    void build(Path& path, float x, float y, float size) const;

    /// 读取并解析 SVG 文件。
    static bool load(const char* file_path, Image& out);
    /// 解析内存中的 SVG 文本。
    static bool parse(const std::string& text, Image& out);
};

}  // namespace svg
}  // namespace aether
