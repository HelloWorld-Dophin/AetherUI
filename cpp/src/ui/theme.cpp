#include "ui/theme.h"

#include <algorithm>

namespace aether {
namespace {

float bezier_sample(float c1, float c2, float u) {
    const float v = 1.0f - u;
    return 3.0f * v * v * u * c1 + 3.0f * v * u * u * c2 + u * u * u;
}

}  // namespace

float CubicBezier::solve(float t) const {
    t = std::clamp(t, 0.0f, 1.0f);
    if (t <= 0.0f) {
        return 0.0f;
    }
    if (t >= 1.0f) {
        return 1.0f;
    }
    float low = 0.0f;
    float high = 1.0f;
    for (int i = 0; i < 24; ++i) {
        const float mid = (low + high) * 0.5f;
        if (bezier_sample(x1, x2, mid) < t) {
            low = mid;
        } else {
            high = mid;
        }
    }
    return bezier_sample(y1, y2, (low + high) * 0.5f);
}

Theme Theme::light() {
    Theme t;
    t.dark = false;

    t.accent_fill_default = color_hex(0x1C5CB4);
    t.accent_fill_secondary = color_hex(0x326BBB);
    t.accent_fill_tertiary = color_hex(0x326BBB);
    t.text_on_accent = color_hex(0xFFFFFF);
    t.text_on_accent_primary = color_hex(0xFFFFFF);

    t.text_primary = color_rgba8(0x00, 0x00, 0x00, 1.0f);
    t.text_secondary = color_rgba8(0x00, 0x00, 0x00, 0.76f);
    t.text_tertiary = color_rgba8(0x00, 0x00, 0x00, 0.58f);
    t.text_disabled = color_rgba8(0x00, 0x00, 0x00, 0.44f);
    // 文本选区：强调色 35% 透明度（Fluent 文本控件选区）。
    t.text_selection = color_rgba8(0x1C, 0x5C, 0xB4, 0.35f);

    t.control_fill_default = color_rgba8(0xFF, 0xFF, 0xFF, 0.70f);
    t.control_fill_secondary = color_rgba8(0xF9, 0xF9, 0xF9, 0.50f);
    t.control_fill_tertiary = color_rgba8(0xF9, 0xF9, 0xF9, 0.30f);
    t.control_fill_disabled = color_rgba8(0xF9, 0xF9, 0xF9, 0.30f);

    t.subtle_fill_secondary = color_rgba8(0x00, 0x00, 0x00, 0.035f);
    t.subtle_fill_tertiary = color_rgba8(0x00, 0x00, 0x00, 0.024f);
    t.selected_background = color_rgba8(0x00, 0x00, 0x00, 0.06f);

    t.control_stroke_default = color_rgba8(0x00, 0x00, 0x00, 0.059f);
    t.control_stroke_secondary = color_rgba8(0x00, 0x00, 0x00, 0.161f);
    t.control_stroke_disabled = color_rgba8(0x00, 0x00, 0x00, 0.03f);
    t.neutral_stroke_subtle = color_rgba8(0x00, 0x00, 0x00, 0.03f);
    t.card_stroke_default = color_rgba8(0x00, 0x00, 0x00, 0.059f);
    t.window_stroke = color_rgba8(0x00, 0x00, 0x00, 0.09f);

    t.card_background = color_rgba8(0xFF, 0xFF, 0xFF, 0.70f);
    t.card_background_secondary = color_rgba8(0xF6, 0xF6, 0xF6, 0.50f);
    t.layer_fill = color_rgba8(0xFF, 0xFF, 0xFF, 0.50f);
    t.solid_background_base = color_hex(0xF3F3F3);
    t.solid_background_secondary = color_hex(0xEEEEEE);
    t.neutral_background_1 = color_hex(0xFFFFFF);
    t.neutral_background_2 = color_hex(0xF9F9F9);

    t.toggle_thumb_stroke_off = color_hex(0x5A5A5A);
    t.toggle_thumb_fill_off = color_hex(0x5A5A5A);
    t.toggle_thumb_fill_on = color_hex(0xFFFFFF);
    t.toggle_track_on = color_hex(0x1C5CB4);
    t.toggle_track_on_focused = color_hex(0x326BBB);
    t.toggle_track_off = color_hex(0xF9F9F9);
    t.toggle_disabled = color_rgba8(0x00, 0x00, 0x00, 0.16f);

    t.scrollbar_thumb = color_rgba8(0x00, 0x00, 0x00, 0.35f);
    t.scrollbar_thumb_hover = color_rgba8(0x00, 0x00, 0x00, 0.50f);
    t.scrollbar_track = color_rgba8(0x00, 0x00, 0x00, 0.0f);

    t.focus_stroke_outer = color_hex(0x000000);

    t.caption_hover = color_rgba8(0x00, 0x00, 0x00, 0.06f);
    t.caption_pressed = color_rgba8(0x00, 0x00, 0x00, 0.03f);
    t.close_hover = color_hex(0xC42B1C);
    t.close_pressed = color_hex(0xA92416);

    t.shadow_card = ShadowSpec{color_rgba8(0x00, 0x00, 0x00, 0.14f), 4.0f, 0.0f, 2.0f};
    t.shadow_control_rest = ShadowSpec{color_rgba8(0x00, 0x00, 0x00, 0.10f), 2.0f, 0.0f, 1.0f};
    t.shadow_text_box = ShadowSpec{color_rgba8(0x00, 0x00, 0x00, 0.05f), 3.0f, 0.0f, 1.0f};

    t.caption = TypeStyle{12.0f, 16.0f};
    t.body = TypeStyle{14.0f, 20.0f};
    t.body_strong = TypeStyle{14.0f, 20.0f};
    t.body_large = TypeStyle{18.0f, 24.0f};
    t.subtitle = TypeStyle{20.0f, 28.0f};
    t.title = TypeStyle{28.0f, 36.0f};
    t.title_large = TypeStyle{40.0f, 52.0f};

    t.mica_base = color_hex(0xF3F3F3);
    t.mica_tint_a = color_rgba8(0xD8, 0xE7, 0xF7, 0.65f);
    t.mica_tint_b = color_rgba8(0xF3, 0xE6, 0xEF, 0.55f);
    return t;
}

Theme Theme::dark_theme() {
    Theme t;
    t.dark = true;

    t.accent_fill_default = color_hex(0x86C4EC);
    t.accent_fill_secondary = color_hex(0x7BB3D7);
    t.accent_fill_tertiary = color_hex(0x7BB3D7);
    t.text_on_accent = color_hex(0x000000);
    t.text_on_accent_primary = color_hex(0x000000);

    t.text_primary = color_rgba8(0xFF, 0xFF, 0xFF, 1.0f);
    t.text_secondary = color_rgba8(0xFF, 0xFF, 0xFF, 0.773f);
    t.text_tertiary = color_rgba8(0xFF, 0xFF, 0xFF, 0.529f);
    t.text_disabled = color_rgba8(0xFF, 0xFF, 0xFF, 0.365f);
    // 文本选区：暗色下用亮蓝强调色的半透明（Fluent 文本控件选区）。
    t.text_selection = color_rgba8(0x86, 0xC4, 0xEC, 0.40f);

    t.control_fill_default = color_rgba8(0xFF, 0xFF, 0xFF, 0.059f);
    t.control_fill_secondary = color_rgba8(0xFF, 0xFF, 0xFF, 0.082f);
    t.control_fill_tertiary = color_rgba8(0xFF, 0xFF, 0xFF, 0.031f);
    t.control_fill_disabled = color_rgba8(0xFF, 0xFF, 0xFF, 0.043f);

    t.subtle_fill_secondary = color_rgba8(0xFF, 0xFF, 0xFF, 0.059f);
    t.subtle_fill_tertiary = color_rgba8(0xFF, 0xFF, 0xFF, 0.039f);
    t.selected_background = color_rgba8(0xFF, 0xFF, 0xFF, 0.09f);

    t.control_stroke_default = color_rgba8(0xFF, 0xFF, 0xFF, 0.071f);
    t.control_stroke_secondary = color_rgba8(0xFF, 0xFF, 0xFF, 0.094f);
    t.control_stroke_disabled = color_rgba8(0xFF, 0xFF, 0xFF, 0.03f);
    t.neutral_stroke_subtle = color_rgba8(0xFF, 0xFF, 0xFF, 0.035f);
    t.card_stroke_default = color_rgba8(0x00, 0x00, 0x00, 0.098f);
    t.window_stroke = color_rgba8(0xFF, 0xFF, 0xFF, 0.10f);

    t.card_background = color_rgba8(0xFF, 0xFF, 0xFF, 0.051f);
    t.card_background_secondary = color_rgba8(0xFF, 0xFF, 0xFF, 0.031f);
    t.layer_fill = color_rgba8(0x3A, 0x3A, 0x3A, 0.30f);
    t.solid_background_base = color_hex(0x202020);
    t.solid_background_secondary = color_hex(0x1C1C1C);
    t.neutral_background_1 = color_hex(0x262626);
    t.neutral_background_2 = color_hex(0x2F2F2F);

    t.toggle_thumb_stroke_off = color_hex(0xCCCCCC);
    t.toggle_thumb_fill_off = color_hex(0xCCCCCC);
    t.toggle_thumb_fill_on = color_hex(0x000000);
    t.toggle_track_on = color_hex(0x86C4EC);
    t.toggle_track_on_focused = color_hex(0x7BB3D7);
    t.toggle_track_off = color_hex(0x2F2F2F);
    t.toggle_disabled = color_rgba8(0xFF, 0xFF, 0xFF, 0.16f);

    t.scrollbar_thumb = color_rgba8(0xFF, 0xFF, 0xFF, 0.35f);
    t.scrollbar_thumb_hover = color_rgba8(0xFF, 0xFF, 0xFF, 0.50f);
    t.scrollbar_track = color_rgba8(0xFF, 0xFF, 0xFF, 0.0f);

    t.focus_stroke_outer = color_hex(0xFFFFFF);

    t.caption_hover = color_rgba8(0xFF, 0xFF, 0xFF, 0.09f);
    t.caption_pressed = color_rgba8(0xFF, 0xFF, 0xFF, 0.04f);
    t.close_hover = color_hex(0xC42B1C);
    t.close_pressed = color_hex(0xA92416);

    t.shadow_card = ShadowSpec{color_rgba8(0x00, 0x00, 0x00, 0.36f), 4.0f, 0.0f, 2.0f};
    t.shadow_control_rest = ShadowSpec{color_rgba8(0x00, 0x00, 0x00, 0.28f), 2.0f, 0.0f, 1.0f};
    t.shadow_text_box = ShadowSpec{color_rgba8(0x00, 0x00, 0x00, 0.35f), 4.0f, 0.0f, 1.0f};

    t.caption = TypeStyle{12.0f, 16.0f};
    t.body = TypeStyle{14.0f, 20.0f};
    t.body_strong = TypeStyle{14.0f, 20.0f};
    t.body_large = TypeStyle{18.0f, 24.0f};
    t.subtitle = TypeStyle{20.0f, 28.0f};
    t.title = TypeStyle{28.0f, 36.0f};
    t.title_large = TypeStyle{40.0f, 52.0f};

    t.mica_base = color_hex(0x202020);
    t.mica_tint_a = color_rgba8(0x25, 0x2F, 0x3D, 0.70f);
    t.mica_tint_b = color_rgba8(0x33, 0x28, 0x33, 0.60f);
    return t;
}

}  // namespace aether
