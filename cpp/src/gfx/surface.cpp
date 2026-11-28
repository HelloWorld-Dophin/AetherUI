#include "gfx/surface.h"

namespace aether {

void Surface::fill(Color c) {
    if (!valid()) {
        return;
    }
    // fill 语义是「铺底」，因此强制不透明，避免残留 alpha 让后续混合发灰。
    const uint32_t pixel = premultiply(with_alpha(c, 1.0f));
    for (int y = 0; y < height_; ++y) {
        uint32_t* dst = row(y);
        for (int x = 0; x < width_; ++x) {
            dst[x] = pixel;
        }
    }
}

}  // namespace aether
