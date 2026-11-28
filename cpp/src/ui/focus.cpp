#include "ui/focus.h"

#include <SDL3/SDL.h>

namespace aether {

void FocusManager::add(Widget* widget) {
    if (widget != nullptr) {
        widgets_.push_back(widget);
    }
}

void FocusManager::clear() { focus_on(nullptr); }

void FocusManager::focus_on(Widget* widget) {
    if (current_ == widget) {
        return;
    }
    if (current_ != nullptr) {
        current_->set_focus(false);
    }
    current_ = widget;
    if (current_ != nullptr) {
        current_->set_focus(true);
    }
}

void FocusManager::press(const Event& event) {
    if (event.type != Event::Type::MouseDown || event.key != 1) {
        return;
    }
    for (auto it = widgets_.rbegin(); it != widgets_.rend(); ++it) {
        Widget* widget = *it;
        if (widget->focusable() && widget->hit_test(event.x, event.y)) {
            focus_on(widget);
            return;
        }
    }
    clear();
}

void FocusManager::cycle(int direction) {
    if (widgets_.empty()) {
        return;
    }
    const int total = static_cast<int>(widgets_.size());
    int start = 0;
    for (int i = 0; i < total; ++i) {
        if (widgets_[static_cast<size_t>(i)] == current_) {
            start = i;
            break;
        }
    }
    for (int step = 1; step <= total; ++step) {
        const int index = ((start + direction * step) % total + total) % total;
        if (widgets_[static_cast<size_t>(index)]->focusable()) {
            focus_on(widgets_[static_cast<size_t>(index)]);
            return;
        }
    }
}

bool FocusManager::handle_event(const Event& event, const UiContext& ui) {
    if (event.type == Event::Type::KeyDown && event.key == SDLK_TAB &&
        (event.mods & SDL_KMOD_CTRL) == 0) {
        cycle((event.mods & SDL_KMOD_SHIFT) != 0 ? -1 : 1);
        return true;
    }
    // 指针事件由控件的正常转发路径处理，这里只接管键盘 / 文本 / 组字。
    if (event.type != Event::Type::KeyDown && event.type != Event::Type::Text &&
        event.type != Event::Type::Editing) {
        return false;
    }
    if (current_ == nullptr) {
        return false;
    }
    return current_->handle_event(event, ui);
}

void FocusManager::sync_ime(const UiContext& ui) const {
    if (current_ != nullptr) {
        current_->sync_ime(ui);
        return;
    }
    if (ui.host != nullptr) {
        ui.host->clear_text_input_area();
    }
}

}  // namespace aether
