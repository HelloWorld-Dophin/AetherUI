#include "gfx/raster.h"

#include <algorithm>
#include <cmath>

namespace aether {
namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kDegToRad = kPi / 180.0f;

float distance(const Point& a, const Point& b) {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    return std::sqrt(dx * dx + dy * dy);
}

/// 把圆弧摊平成折线，弦高误差控制在 0.15px 以内。
void append_arc(std::vector<Point>& pts, float cx, float cy, float r, float start_deg,
                float end_deg) {
    const float sweep = std::fabs(end_deg - start_deg) * kDegToRad;
    if (sweep <= 0.0f) {
        return;
    }
    float step = sweep;
    if (r > 0.15f) {
        step = 2.0f * std::acos(1.0f - 0.15f / r);
    }
    const int segments = std::clamp(static_cast<int>(std::ceil(sweep / std::max(step, 1e-4f))), 1, 128);
    for (int i = 1; i <= segments; ++i) {
        const float t = start_deg + (end_deg - start_deg) * (static_cast<float>(i) / segments);
        const float rad = t * kDegToRad;
        pts.push_back(Point{cx + r * std::cos(rad), cy + r * std::sin(rad)});
    }
}

/// 把一条水平跨度按面积精确累加到覆盖率行上。
inline void add_span(float* row, int x0, int w, float xa, float xb, float weight) {
    const float fx0 = std::clamp(xa - static_cast<float>(x0), 0.0f, static_cast<float>(w));
    const float fx1 = std::clamp(xb - static_cast<float>(x0), 0.0f, static_cast<float>(w));
    if (fx1 <= fx0) {
        return;
    }
    const int ia = static_cast<int>(fx0);
    const int ib = static_cast<int>(fx1);
    if (ia >= w) {
        return;
    }
    if (ia == ib) {
        row[ia] += (fx1 - fx0) * weight;
        return;
    }
    row[ia] += (static_cast<float>(ia + 1) - fx0) * weight;
    for (int i = ia + 1; i < ib && i < w; ++i) {
        row[i] += weight;
    }
    if (ib < w) {
        row[ib] += (fx1 - static_cast<float>(ib)) * weight;
    }
}

void box_blur_h(const std::vector<float>& src, std::vector<float>& dst, int w, int h, int r) {
    const float inv = 1.0f / static_cast<float>(2 * r + 1);
    for (int y = 0; y < h; ++y) {
        const float* s = src.data() + static_cast<size_t>(y) * w;
        float* d = dst.data() + static_cast<size_t>(y) * w;
        float sum = 0.0f;
        for (int i = -r; i <= r; ++i) {
            sum += s[std::clamp(i, 0, w - 1)];
        }
        for (int x = 0; x < w; ++x) {
            d[x] = sum * inv;
            sum += s[std::clamp(x + r + 1, 0, w - 1)] - s[std::clamp(x - r, 0, w - 1)];
        }
    }
}

void box_blur_v(const std::vector<float>& src, std::vector<float>& dst, int w, int h, int r) {
    const float inv = 1.0f / static_cast<float>(2 * r + 1);
    for (int x = 0; x < w; ++x) {
        float sum = 0.0f;
        for (int i = -r; i <= r; ++i) {
            sum += src[static_cast<size_t>(std::clamp(i, 0, h - 1)) * w + x];
        }
        for (int y = 0; y < h; ++y) {
            dst[static_cast<size_t>(y) * w + x] = sum * inv;
            sum += src[static_cast<size_t>(std::clamp(y + r + 1, 0, h - 1)) * w + x] -
                   src[static_cast<size_t>(std::clamp(y - r, 0, h - 1)) * w + x];
        }
    }
}

}  // namespace

Point Path::current() const {
    if (subpaths_.empty() || subpaths_.back().empty()) {
        return Point{0.0f, 0.0f};
    }
    return subpaths_.back().back();
}

void Path::move_to(float x, float y) {
    subpaths_.emplace_back();
    subpaths_.back().push_back(Point{x, y});
}

void Path::line_to(float x, float y) {
    if (subpaths_.empty()) {
        move_to(x, y);
        return;
    }
    subpaths_.back().push_back(Point{x, y});
}

void Path::quad_to(float cx, float cy, float x, float y) {
    const Point p0 = current();
    const float len = distance(p0, Point{cx, cy}) + distance(Point{cx, cy}, Point{x, y});
    const int n = std::clamp(static_cast<int>(std::ceil(len / 0.4f)), 2, 96);
    for (int i = 1; i <= n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(n);
        const float u = 1.0f - t;
        line_to(u * u * p0.x + 2.0f * u * t * cx + t * t * x,
                u * u * p0.y + 2.0f * u * t * cy + t * t * y);
    }
}

void Path::cubic_to(float cx1, float cy1, float cx2, float cy2, float x, float y) {
    const Point p0 = current();
    const float len = distance(p0, Point{cx1, cy1}) + distance(Point{cx1, cy1}, Point{cx2, cy2}) +
                      distance(Point{cx2, cy2}, Point{x, y});
    const int n = std::clamp(static_cast<int>(std::ceil(len / 0.4f)), 2, 128);
    for (int i = 1; i <= n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(n);
        const float u = 1.0f - t;
        const float a = u * u * u;
        const float b = 3.0f * u * u * t;
        const float c = 3.0f * u * t * t;
        const float d = t * t * t;
        line_to(a * p0.x + b * cx1 + c * cx2 + d * x, a * p0.y + b * cy1 + c * cy2 + d * y);
    }
}

void Path::add_rect(Rect r, bool reverse) {
    std::vector<Point> pts{{r.x, r.y}, {r.x + r.w, r.y}, {r.x + r.w, r.y + r.h}, {r.x, r.y + r.h}};
    if (reverse) {
        std::reverse(pts.begin(), pts.end());
    }
    subpaths_.push_back(std::move(pts));
}

void Path::add_round_rect(Rect r, float radius, bool reverse) {
    const float rad = std::min({radius, r.w * 0.5f, r.h * 0.5f});
    std::vector<Point> pts;
    if (rad <= 0.01f) {
        pts = {{r.x, r.y}, {r.x + r.w, r.y}, {r.x + r.w, r.y + r.h}, {r.x, r.y + r.h}};
    } else {
        pts.push_back(Point{r.x + rad, r.y});
        pts.push_back(Point{r.x + r.w - rad, r.y});
        append_arc(pts, r.x + r.w - rad, r.y + rad, rad, -90.0f, 0.0f);
        pts.push_back(Point{r.x + r.w, r.y + r.h - rad});
        append_arc(pts, r.x + r.w - rad, r.y + r.h - rad, rad, 0.0f, 90.0f);
        pts.push_back(Point{r.x + rad, r.y + r.h});
        append_arc(pts, r.x + rad, r.y + r.h - rad, rad, 90.0f, 180.0f);
        pts.push_back(Point{r.x, r.y + rad});
        append_arc(pts, r.x + rad, r.y + rad, rad, 180.0f, 270.0f);
    }
    if (reverse) {
        std::reverse(pts.begin(), pts.end());
    }
    subpaths_.push_back(std::move(pts));
}

void Path::add_circle(float cx, float cy, float radius, bool reverse) {
    if (radius <= 0.0f) {
        return;
    }
    std::vector<Point> pts;
    const float sweep = 2.0f * kPi;
    const int segments =
        std::clamp(static_cast<int>(std::ceil(sweep / (2.0f * std::acos(1.0f - 0.15f / radius)))), 8, 256);
    for (int i = 0; i < segments; ++i) {
        const float t = sweep * static_cast<float>(i) / static_cast<float>(segments);
        pts.push_back(Point{cx + radius * std::cos(t), cy + radius * std::sin(t)});
    }
    if (reverse) {
        std::reverse(pts.begin(), pts.end());
    }
    subpaths_.push_back(std::move(pts));
}

void Path::clear() { subpaths_.clear(); }

void Path::build_edges(std::vector<Edge>& out) const {
    out.clear();
    for (const auto& sub : subpaths_) {
        const size_t n = sub.size();
        if (n < 2) {
            continue;
        }
        for (size_t i = 0; i < n; ++i) {
            const Point& a = sub[i];
            const Point& b = sub[(i + 1) % n];
            if (a.y == b.y) {
                continue;
            }
            out.push_back(Edge{a.x, a.y, b.x, b.y});
        }
    }
}

bool Rasterizer::rasterize(const Path& path, Recti clip, Mask& out) {
    out.x0 = 0;
    out.y0 = 0;
    out.w = 0;
    out.h = 0;
    out.a.clear();

    path.build_edges(edges_);
    if (edges_.empty()) {
        return false;
    }

    float minx = edges_[0].x0;
    float maxx = edges_[0].x0;
    float miny = edges_[0].y0;
    float maxy = edges_[0].y0;
    for (const Edge& e : edges_) {
        minx = std::min({minx, e.x0, e.x1});
        maxx = std::max({maxx, e.x0, e.x1});
        miny = std::min({miny, e.y0, e.y1});
        maxy = std::max({maxy, e.y0, e.y1});
    }

    const Recti box = intersect(cover(Rect{minx - 1.0f, miny - 1.0f, (maxx - minx) + 2.0f,
                                            (maxy - miny) + 2.0f}),
                                clip);
    if (box.empty()) {
        return false;
    }

    out.x0 = box.x;
    out.y0 = box.y;
    out.w = box.w;
    out.h = box.h;
    out.a.assign(static_cast<size_t>(box.w) * box.h, 0.0f);

    constexpr int kSubSamples = 8;
    constexpr float kWeight = 1.0f / static_cast<float>(kSubSamples);

    for (int py = box.y; py < box.bottom(); ++py) {
        float* row = out.row(py);
        for (int s = 0; s < kSubSamples; ++s) {
            const float sy = static_cast<float>(py) + (static_cast<float>(s) + 0.5f) * kWeight;

            crossings_.clear();
            for (const Edge& e : edges_) {
                const float lo = std::min(e.y0, e.y1);
                const float hi = std::max(e.y0, e.y1);
                if (sy < lo || sy >= hi) {
                    continue;
                }
                const float t = (sy - e.y0) / (e.y1 - e.y0);
                crossings_.push_back(Crossing{e.x0 + t * (e.x1 - e.x0), e.y1 > e.y0 ? 1 : -1});
            }
            if (crossings_.empty()) {
                continue;
            }

            std::sort(crossings_.begin(), crossings_.end(),
                      [](const Crossing& a, const Crossing& b) { return a.x < b.x; });

            int winding = 0;
            float span_start = 0.0f;
            for (const Crossing& c : crossings_) {
                const int prev = winding;
                winding += c.dir;
                if (prev == 0 && winding != 0) {
                    span_start = c.x;
                } else if (prev != 0 && winding == 0) {
                    add_span(row, box.x, box.w, span_start, c.x, kWeight);
                }
            }
        }
    }

    return true;
}

void blur_mask(Mask& mask, float sigma) {
    if (mask.empty() || sigma <= 0.0f) {
        return;
    }
    // 三次盒式模糊的等效标准差约等于盒半径，因此半径取 sigma 本身。
    const int radius = std::max(1, static_cast<int>(std::lround(sigma - 0.5f)));
    const int r = std::min({radius, std::max(0, mask.w - 1), std::max(0, mask.h - 1)});
    if (r <= 0) {
        return;
    }

    std::vector<float> tmp(mask.a.size());
    for (int pass = 0; pass < 3; ++pass) {
        box_blur_h(mask.a, tmp, mask.w, mask.h, r);
        box_blur_v(tmp, mask.a, mask.w, mask.h, r);
    }
}

}  // namespace aether
