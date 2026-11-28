//! Fluent 图标：SVG 路径数据 + 8 位 alpha 位图缓存。
//!
//! 数据由 `cpp/tools/gen_icons.js` 从 `assets/icons/*.svg` 生成，坐标固定为 24×24 视框；
//! 绘制时按目标边长缩放并缓存成单通道位图，之后只做着色叠加，
//! 因此图标页一次铺满上百个图标也不会有光栅化开销。

#pragma once

#include <cstdint>
#include <vector>

#include "gfx/geometry.h"
#include "gfx/painter.h"

namespace aether {

class Painter;

namespace icons {

/// 一条路径：命令流（见 gen_icons.js）+ 描边宽度（0 表示只填充）。
struct IconPath {
    const float* cmds = nullptr;
    int count = 0;
    float stroke = 0.0f;
};

/// 图标条目：名称（`icons::HOME` 形式）与常规 / 加粗两条路径。
struct IconEntry {
    const char* name = nullptr;
    int regular = 0;
    int bold = 0;
};

extern const IconPath kIconPaths[];
extern const int kIconPathCount;
extern const IconEntry kIconEntries[];
extern const int kIconEntryCount;

/// 名称 → 图标下标；找不到返回 -1。
int index_of(const char* name);

/// 下标 → 名称；越界返回空字符串。
const char* name_of(int index);

/// 图标变体：浅色主题用加粗（浅底上细笔画发虚），深色主题用常规。
enum class Variant { Regular, Bold };

void draw(Painter& painter, int index, float x, float y, float size, Color color, float scale,
          Variant variant);

/// 按明暗模式选择变体的便捷入口。
inline Variant variant_for(bool dark) { return dark ? Variant::Regular : Variant::Bold; }

}  // namespace icons
}  // namespace aether
