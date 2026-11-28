#include "ui/svg.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>

namespace aether {
namespace svg {
namespace {

bool is_separator(char c) {
    return c == ' ' || c == ',' || c == '\t' || c == '\n' || c == '\r';
}

bool read_number(const char*& p, const char* end, float& out) {
    while (p < end && is_separator(*p)) {
        ++p;
    }
    if (p >= end) {
        return false;
    }

    bool negative = false;
    if (*p == '+' || *p == '-') {
        negative = *p == '-';
        ++p;
    }

    double value = 0.0;
    bool any = false;
    while (p < end && *p >= '0' && *p <= '9') {
        value = value * 10.0 + static_cast<double>(*p - '0');
        any = true;
        ++p;
    }
    if (p < end && *p == '.') {
        ++p;
        double factor = 0.1;
        while (p < end && *p >= '0' && *p <= '9') {
            value += static_cast<double>(*p - '0') * factor;
            factor *= 0.1;
            any = true;
            ++p;
        }
    }
    if (!any) {
        return false;
    }
    if (p < end && (*p == 'e' || *p == 'E')) {
        const char* mark = p;
        ++p;
        bool exp_negative = false;
        if (p < end && (*p == '+' || *p == '-')) {
            exp_negative = *p == '-';
            ++p;
        }
        int exponent = 0;
        bool exp_any = false;
        while (p < end && *p >= '0' && *p <= '9') {
            exponent = exponent * 10 + (*p - '0');
            exp_any = true;
            ++p;
        }
        if (!exp_any) {
            p = mark;
        } else {
            value *= std::pow(10.0, exp_negative ? -exponent : exponent);
        }
    }

    out = static_cast<float>(negative ? -value : value);
    return true;
}

/// 取标签内某个属性的值（不带引号）；找不到返回空串。
std::string attribute(const std::string& tag, const std::string& name) {
    const std::string needle = name + "=";
    size_t pos = tag.find(needle);
    while (pos != std::string::npos) {
        const bool boundary = pos == 0 || is_separator(tag[pos - 1]) || tag[pos - 1] == '<';
        if (boundary) {
            break;
        }
        pos = tag.find(needle, pos + 1);
    }
    if (pos == std::string::npos) {
        return std::string();
    }
    size_t at = pos + needle.size();
    if (at >= tag.size()) {
        return std::string();
    }
    const char quote = tag[at];
    if (quote != '"' && quote != '\'') {
        return std::string();
    }
    ++at;
    const size_t close = tag.find(quote, at);
    if (close == std::string::npos) {
        return std::string();
    }
    return tag.substr(at, close - at);
}

/// 去掉 `px` 等单位后缀后取数值。
float length_value(const std::string& text) {
    const char* p = text.c_str();
    const char* end = p + text.size();
    float value = 0.0f;
    return read_number(p, end, value) ? value : 0.0f;
}

bool parse_path_data(const std::string& data, std::vector<Segment>& out) {
    const char* p = data.c_str();
    const char* end = p + data.size();

    struct Point {
        float x = 0.0f;
        float y = 0.0f;
    };
    Point current{0.0f, 0.0f};
    Point start{0.0f, 0.0f};
    Point cubic_ctrl{0.0f, 0.0f};
    Point quad_ctrl{0.0f, 0.0f};
    bool has_cubic = false;
    bool has_quad = false;

    char command = '\0';
    while (true) {
        while (p < end && is_separator(*p)) {
            ++p;
        }
        if (p >= end) {
            break;
        }
        if (std::isalpha(static_cast<unsigned char>(*p)) != 0) {
            command = *p;
            ++p;
        } else if (command == '\0') {
            return false;
        }

        const bool relative = std::islower(static_cast<unsigned char>(command)) != 0;
        const char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(command)));
        const auto point = [&](float x, float y) {
            return relative ? Point{current.x + x, current.y + y} : Point{x, y};
        };
        const auto emit = [&](Segment::Kind kind, Point a, Point b = {}, Point c = {}) {
            Segment segment;
            segment.kind = kind;
            segment.p[0] = a.x;
            segment.p[1] = a.y;
            segment.p[2] = b.x;
            segment.p[3] = b.y;
            segment.p[4] = c.x;
            segment.p[5] = c.y;
            out.push_back(segment);
        };

        float v[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        int need = 0;
        switch (upper) {
            case 'M':
            case 'L':
            case 'T':
                need = 2;
                break;
            case 'H':
            case 'V':
                need = 1;
                break;
            case 'Q':
            case 'S':
                need = 4;
                break;
            case 'C':
                need = 6;
                break;
            case 'Z':
                need = 0;
                break;
            default:
                return false;
        }
        for (int i = 0; i < need; ++i) {
            if (!read_number(p, end, v[i])) {
                return false;
            }
        }

        switch (upper) {
            case 'M': {
                const Point target = point(v[0], v[1]);
                emit(Segment::Kind::Move, target);
                current = target;
                start = target;
                has_cubic = false;
                has_quad = false;
                command = relative ? 'l' : 'L';
                break;
            }
            case 'L': {
                const Point target = point(v[0], v[1]);
                emit(Segment::Kind::Line, target);
                current = target;
                has_cubic = false;
                has_quad = false;
                break;
            }
            case 'H': {
                const Point target{relative ? current.x + v[0] : v[0], current.y};
                emit(Segment::Kind::Line, target);
                current = target;
                has_cubic = false;
                has_quad = false;
                break;
            }
            case 'V': {
                const Point target{current.x, relative ? current.y + v[0] : v[0]};
                emit(Segment::Kind::Line, target);
                current = target;
                has_cubic = false;
                has_quad = false;
                break;
            }
            case 'C': {
                const Point c1 = point(v[0], v[1]);
                const Point c2 = point(v[2], v[3]);
                const Point target = point(v[4], v[5]);
                emit(Segment::Kind::Cubic, c1, c2, target);
                current = target;
                cubic_ctrl = c2;
                has_cubic = true;
                has_quad = false;
                break;
            }
            case 'S': {
                const Point c1 = has_cubic
                                     ? Point{current.x * 2.0f - cubic_ctrl.x,
                                             current.y * 2.0f - cubic_ctrl.y}
                                     : current;
                const Point c2 = point(v[0], v[1]);
                const Point target = point(v[2], v[3]);
                emit(Segment::Kind::Cubic, c1, c2, target);
                current = target;
                cubic_ctrl = c2;
                has_cubic = true;
                has_quad = false;
                break;
            }
            case 'Q': {
                const Point ctrl = point(v[0], v[1]);
                const Point target = point(v[2], v[3]);
                emit(Segment::Kind::Quad, ctrl, target);
                current = target;
                quad_ctrl = ctrl;
                has_quad = true;
                has_cubic = false;
                break;
            }
            case 'T': {
                const Point ctrl =
                    has_quad ? Point{current.x * 2.0f - quad_ctrl.x,
                                     current.y * 2.0f - quad_ctrl.y}
                             : current;
                const Point target = point(v[0], v[1]);
                emit(Segment::Kind::Quad, ctrl, target);
                current = target;
                quad_ctrl = ctrl;
                has_quad = true;
                has_cubic = false;
                break;
            }
            case 'Z': {
                Segment segment;
                segment.kind = Segment::Kind::Close;
                out.push_back(segment);
                current = start;
                has_cubic = false;
                has_quad = false;
                break;
            }
            default:
                return false;
        }
    }
    return true;
}

}  // namespace

void Image::build(Path& path, float x, float y, float size) const {
    if (!valid() || size <= 0.0f) {
        return;
    }
    const float scale = size / std::max(view_w, view_h);
    const float origin_x = x - view_x * scale + (size - view_w * scale) * 0.5f;
    const float origin_y = y - view_y * scale + (size - view_h * scale) * 0.5f;
    const auto sx = [&](float value) { return origin_x + value * scale; };
    const auto sy = [&](float value) { return origin_y + value * scale; };

    for (const Segment& segment : segments) {
        switch (segment.kind) {
            case Segment::Kind::Move:
                path.move_to(sx(segment.p[0]), sy(segment.p[1]));
                break;
            case Segment::Kind::Line:
                path.line_to(sx(segment.p[0]), sy(segment.p[1]));
                break;
            case Segment::Kind::Quad:
                path.quad_to(sx(segment.p[0]), sy(segment.p[1]), sx(segment.p[2]),
                             sy(segment.p[3]));
                break;
            case Segment::Kind::Cubic:
                path.cubic_to(sx(segment.p[0]), sy(segment.p[1]), sx(segment.p[2]),
                              sy(segment.p[3]), sx(segment.p[4]), sy(segment.p[5]));
                break;
            case Segment::Kind::Close:
                break;  // 光栅化时子路径隐式闭合
        }
    }
}

bool Image::parse(const std::string& text, Image& out) {
    Image image;

    // viewBox 优先，没有就退回 width / height。
    const size_t svg_at = text.find("<svg");
    if (svg_at == std::string::npos) {
        return false;
    }
    const size_t svg_end = text.find('>', svg_at);
    if (svg_end == std::string::npos) {
        return false;
    }
    const std::string root = text.substr(svg_at, svg_end - svg_at);
    const std::string view_box = attribute(root, "viewBox");
    if (!view_box.empty()) {
        const char* p = view_box.c_str();
        const char* end = p + view_box.size();
        float values[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        for (float& value : values) {
            if (!read_number(p, end, value)) {
                return false;
            }
        }
        image.view_x = values[0];
        image.view_y = values[1];
        image.view_w = values[2];
        image.view_h = values[3];
    } else {
        image.view_w = length_value(attribute(root, "width"));
        image.view_h = length_value(attribute(root, "height"));
    }

    size_t at = 0;
    while ((at = text.find("<path", at)) != std::string::npos) {
        const size_t tag_end = text.find('>', at);
        if (tag_end == std::string::npos) {
            break;
        }
        const std::string tag = text.substr(at, tag_end - at);
        const std::string data = attribute(tag, "d");
        if (!data.empty() && !parse_path_data(data, image.segments)) {
            return false;
        }
        at = tag_end;
    }

    if (image.view_w <= 0.0f || image.view_h <= 0.0f) {
        image.view_w = 24.0f;
        image.view_h = 24.0f;
    }
    if (image.segments.empty()) {
        return false;
    }
    out = std::move(image);
    return true;
}

bool Image::load(const char* file_path, Image& out) {
    if (file_path == nullptr) {
        return false;
    }
    FILE* file = std::fopen(file_path, "rb");
    if (file == nullptr) {
        return false;
    }

    std::string text;
    char buffer[8192];
    size_t read = 0;
    while ((read = std::fread(buffer, 1, sizeof(buffer), file)) > 0) {
        text.append(buffer, read);
    }
    std::fclose(file);
    return parse(text, out);
}

}  // namespace svg
}  // namespace aether
