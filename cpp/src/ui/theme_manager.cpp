#include "ui/theme_manager.h"

namespace aether {

void ThemeManager::apply() {
    const bool dark = preference_ == kFollowSystem ? system_dark_ : preference_ == kDark;
    if (theme_.dark == dark) {
        return;
    }
    theme_ = dark ? Theme::dark_theme() : Theme::light();
}

void ThemeManager::set_system_dark(bool dark) {
    system_dark_ = dark;
    apply();
}

void ThemeManager::set_preference(int preference) {
    preference_ = preference < kFollowSystem ? kFollowSystem
                                             : (preference > kDark ? kDark : preference);
    apply();
}

void ThemeManager::toggle() {
    set_preference(theme_.dark ? kLight : kDark);
}

bool ThemeManager::handle_event(const Event& event) {
    if (event.type != Event::Type::SystemTheme) {
        return false;
    }
    const bool before = theme_.dark;
    set_system_dark(event.dark);
    return theme_.dark != before;
}

}  // namespace aether
