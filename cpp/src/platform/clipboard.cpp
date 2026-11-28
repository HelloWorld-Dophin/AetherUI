#include "platform/clipboard.h"

#include <SDL3/SDL.h>

namespace aether {

std::string clipboard_text() {
    char* raw = SDL_GetClipboardText();
    if (raw == nullptr) {
        return std::string();
    }
    std::string value(raw);
    SDL_free(raw);
    return value;
}

void set_clipboard_text(const std::string& value) {
    if (!value.empty()) {
        SDL_SetClipboardText(value.c_str());
    }
}

}  // namespace aether
