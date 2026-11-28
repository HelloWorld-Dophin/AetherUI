#include "ui/icons.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "gfx/raster.h"

namespace aether {
namespace icons {
namespace {

constexpr float kViewBox = 24.0f;

/// 已栅格化的图标位图（单通道覆盖率）。
struct IconBitmap {
    int size = 0;
    std::vector<uint8_t> alpha;
    bool valid = false;
};

struct Key {
    int index = 0;
    int px = 0;
    int bold = 0;

    bool operator==(const Key& other) const {
        return index == other.index && px == other.px && bold == other.bold;
    }
};

struct KeyHash {
    size_t operator()(const Key& k) const {
        size_t h = static_cast<size_t>(k.index) * 0x9E3779B97F4A7C15ull;
        h ^= static_cast<size_t>(k.px) * 0xC2B2AE3D27D4EB4Full;
        h ^= static_cast<size_t>(k.bold) * 0x165667B19E3779F9ull;
        return h ^ (h >> 29);
    }
};

std::unordered_map<Key, IconBitmap, KeyHash>& cache() {
    static std::unordered_map<Key, IconBitmap, KeyHash> store;
    return store;
}

struct Flattener {
    std::vector<std::vector<Point>> subpaths;
    std::vector<Point> current;

    void move_to(float x, float y) {
        flush();
        current.push_back(Point{x, y});
    }
    void line_to(float x, float y) { current.push_back(Point{x, y}); }

    void cubic_to(float x1, float y1, float x2, float y2, float x, float y) {
        if (current.empty()) {
            current.push_back(Point{x, y});
            return;
        }
        const Point p0 = current.back();
        const float len = std::hypot(x1 - p0.x, y1 - p0.y) + std::hypot(x2 - x1, y2 - y1) +
                          std::hypot(x - x2, y - y2);
        const int n = std::clamp(static_cast<int>(std::ceil(len / 0.5f)), 2, 48);
        for (int i = 1; i <= n; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(n);
            const float u = 1.0f - t;
            const float a = u * u * u;
            const float b = 3.0f * u * u * t;
            const float c = 3.0f * u * t * t;
            const float d = t * t * t;
            current.push_back(Point{a * p0.x + b * x1 + c * x2 + d * x, a * p0.y + b * y1 + c * y2 + d * y});
        }
    }

    void flush() {
        if (current.size() >= 2) {
            subpaths.push_back(current);
        }
        current.clear();
    }

    void finish() {
        flush();
    }
};

/// 把描边宽度以「线段矩形 + 圆头」的方式并进路径（非零环绕直接并集）。
void append_stroke(Path& path, const std::vector<std::vector<Point>>& subpaths, float half) {
    for (const std::vector<Point>& poly : subpaths) {
        const size_t n = poly.size();
        for (size_t i = 0; i < n; ++i) {
            const Point& a = poly[i];
            const Point& b = poly[(i + 1) % n];
            const float dx = b.x - a.x;
            const float dy = b.y - a.y;
            const float len = std::hypot(dx, dy);
            if (len > 1e-4f) {
                const float nx = -dy / len * half;
                const float ny = dx / len * half;
                path.move_to(a.x + nx, a.y + ny);
                path.line_to(b.x + nx, b.y + ny);
                path.line_to(b.x - nx, b.y - ny);
                path.line_to(a.x - nx, a.y - ny);
            }
            path.add_circle(a.x, a.y, half);
        }
    }
}

const IconBitmap& bitmap(int index, int px, bool bold) {
    const Key key{index, px, bold ? 1 : 0};
    auto it = cache().find(key);
    if (it != cache().end()) {
        return it->second;
    }

    IconBitmap out;
    out.size = px;

    const IconEntry& entry = kIconEntries[std::clamp(index, 0, kIconEntryCount - 1)];
    const int path_index = bold ? entry.bold : entry.regular;
    const IconPath& source = kIconPaths[std::clamp(path_index, 0, kIconPathCount - 1)];

    const float scale = static_cast<float>(px) / kViewBox;
    Path path;
    Flattener flat;
    const float* c = source.cmds;
    const float* end = source.cmds + source.count;
    while (c < end) {
        const int op = static_cast<int>(c[0]);
        switch (op) {
            case 0:
                path.move_to(c[1] * scale, c[2] * scale);
                flat.move_to(c[1] * scale, c[2] * scale);
                c += 3;
                break;
            case 1:
                path.line_to(c[1] * scale, c[2] * scale);
                flat.line_to(c[1] * scale, c[2] * scale);
                c += 3;
                break;
            case 2:
                path.cubic_to(c[1] * scale, c[2] * scale, c[3] * scale, c[4] * scale,
                              c[5] * scale, c[6] * scale);
                flat.cubic_to(c[1] * scale, c[2] * scale, c[3] * scale, c[4] * scale,
                              c[5] * scale, c[6] * scale);
                c += 7;
                break;
            case 3:
                flat.flush();
                c += 1;
                break;
            default:
                c = end;
                break;
        }
    }
    flat.finish();

    if (source.stroke > 0.0f) {
        append_stroke(path, flat.subpaths, source.stroke * scale * 0.5f);
    }

    Rasterizer rasterizer;
    Mask mask;
    if (rasterizer.rasterize(path, Recti{0, 0, px, px}, mask)) {
        out.alpha.assign(static_cast<size_t>(px) * px, 0);
        for (int y = 0; y < mask.h; ++y) {
            const int ty = mask.y0 + y;
            if (ty < 0 || ty >= px) {
                continue;
            }
            const float* row = mask.row(mask.y0 + y);
            for (int x = 0; x < mask.w; ++x) {
                const int tx = mask.x0 + x;
                if (tx < 0 || tx >= px) {
                    continue;
                }
                const float cov = std::min(row[x], 1.0f);
                out.alpha[static_cast<size_t>(ty) * px + tx] =
                    static_cast<uint8_t>(std::lround(cov * 255.0f));
            }
        }
        out.valid = true;
    }

    auto [inserted, _] = cache().emplace(key, std::move(out));
    return inserted->second;
}

}  // namespace

int index_of(const char* name) {
    if (name == nullptr) {
        return -1;
    }
    const char* bare = name;
    const char* prefix = std::strstr(name, "icons::");
    if (prefix != nullptr) {
        bare = prefix + 7;
    }
    for (int i = 0; i < kIconEntryCount; ++i) {
        const char* entry = kIconEntries[i].name;
        const char* entry_bare = std::strstr(entry, "icons::");
        entry_bare = entry_bare != nullptr ? entry_bare + 7 : entry;
        if (std::strcmp(entry_bare, bare) == 0) {
            return i;
        }
    }
    return -1;
}

const char* name_of(int index) {
    if (index < 0 || index >= kIconEntryCount) {
        return "";
    }
    return kIconEntries[index].name;
}

void draw(Painter& painter, int index, float x, float y, float size, Color color, float scale,
          Variant variant) {
    if (index < 0 || index >= kIconEntryCount || size <= 0.0f || color.a <= 0.0f) {
        return;
    }
    const int px = std::max(1, static_cast<int>(std::lround(size * scale)));
    const IconBitmap& bmp = bitmap(index, px, variant == Variant::Bold);
    if (!bmp.valid) {
        return;
    }
    painter.blit_alpha(static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)),
                       bmp.alpha.data(), bmp.size, bmp.size, bmp.size, color);
}

}  // namespace icons
}  // namespace aether
