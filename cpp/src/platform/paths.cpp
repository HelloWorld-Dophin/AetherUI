#include "platform/paths.h"

#include <cstdio>
#include <cstring>

#include <SDL3/SDL.h>

#ifndef AETHERUI_ASSET_DIR
#define AETHERUI_ASSET_DIR ""
#endif

namespace aether {

std::string executable_dir() {
    // SDL3 的 SDL_GetBasePath 返回内部缓存，调用方不得释放。
    const char* base = SDL_GetBasePath();
    return base != nullptr ? std::string(base) : std::string();
}

std::string find_asset(const std::string& relative) {
    if (relative.empty()) {
        return std::string();
    }
    const std::string base = executable_dir();
    const char* prefixes[] = {"", "../", "../../", "../../../"};
    for (const char* prefix : prefixes) {
        const std::string path = base + prefix + "assets/" + relative;
        if (FILE* probe = std::fopen(path.c_str(), "rb")) {
            std::fclose(probe);
            return path;
        }
    }
    const std::string local = "assets/" + relative;
    if (FILE* probe = std::fopen(local.c_str(), "rb")) {
        std::fclose(probe);
        return local;
    }
    // 安装后的布局（deb 里是 /usr/share/aetherui/assets/...）。
    if (std::strlen(AETHERUI_ASSET_DIR) > 0) {
        const std::string installed = std::string(AETHERUI_ASSET_DIR) + "/assets/" + relative;
        if (FILE* probe = std::fopen(installed.c_str(), "rb")) {
            std::fclose(probe);
            return installed;
        }
    }
    return std::string();
}

}  // namespace aether
