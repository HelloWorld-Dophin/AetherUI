#include "platform/distro.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

namespace aether {
namespace {

// 缓存格式版本：字段或判定规则变化时递增，旧缓存自动作废。
constexpr int kCacheSchema = 1;

/// 缓存文件路径：Windows 用 %LOCALAPPDATA%，其他平台用 XDG 缓存目录。
std::string cache_path() {
    namespace fs = std::filesystem;
#ifdef _WIN32
    const char* base = std::getenv("LOCALAPPDATA");
    if (base == nullptr) {
        base = std::getenv("APPDATA");
    }
    if (base == nullptr) {
        return std::string();
    }
    return (fs::path(base) / "AetherUI" / "platform.cache").string();
#else
    const char* base = std::getenv("XDG_CACHE_HOME");
    if (base != nullptr && base[0] != '\0') {
        return (fs::path(base) / "aetherui" / "platform.cache").string();
    }
    const char* home = std::getenv("HOME");
    if (home == nullptr) {
        return std::string();
    }
    return (fs::path(home) / ".cache" / "aetherui" / "platform.cache").string();
#endif
}

/// 探测 /etc/os-release 的 ID 字段。
const char* read_os_release_id() {
    static char value[64];
    value[0] = '\0';
    FILE* file = std::fopen("/etc/os-release", "r");
    if (file == nullptr) {
        return value;
    }
    char line[256];
    while (std::fgets(line, sizeof line, file) != nullptr) {
        if (std::strncmp(line, "ID=", 3) != 0) {
            continue;
        }
        const char* p = line + 3;
        size_t n = 0;
        for (; *p != '\0' && *p != '\n' && *p != '"' && n + 1 < sizeof value; ++p) {
            value[n++] = *p;
        }
        value[n] = '\0';
        break;
    }
    std::fclose(file);
    return value;
}

bool file_exists(const char* path) {
    FILE* file = std::fopen(path, "rb");
    if (file == nullptr) {
        return false;
    }
    std::fclose(file);
    return true;
}

/// 桌面环境变量中是否出现 Deepin（容器 / 缺少 os-release 时的兜底）。
bool desktop_mentions_deepin() {
    const char* desktop = std::getenv("XDG_CURRENT_DESKTOP");
    return desktop != nullptr && std::strstr(desktop, "Deepin") != nullptr;
}

std::string serialize(Distro distro) {
    std::string out = "schema=" + std::to_string(kCacheSchema) + "\n";
    out += "distro=";
    out += distro_name(distro);
    out += "\n";
    return out;
}

/// 读缓存，返回 false 表示需要重新探测。
bool load_cache(Distro& out) {
    const std::string path = cache_path();
    if (path.empty()) {
        return false;
    }
    FILE* file = std::fopen(path.c_str(), "r");
    if (file == nullptr) {
        return false;
    }
    int schema = 0;
    char distro[32] = {};
    char line[128];
    while (std::fgets(line, sizeof line, file) != nullptr) {
        if (std::sscanf(line, "schema=%d", &schema) == 1) {
            continue;
        }
        if (std::sscanf(line, "distro=%31s", distro) == 1) {
            continue;
        }
    }
    std::fclose(file);
    if (schema != kCacheSchema || distro[0] == '\0') {
        return false;
    }
    out = std::strcmp(distro, "deepin") == 0 ? Distro::Deepin : Distro::Other;
    return true;
}

void save_cache(Distro distro) {
    const std::string path = cache_path();
    if (path.empty()) {
        return;
    }
    std::error_code ec;
    const std::filesystem::path parent = std::filesystem::path(path).parent_path();
    std::filesystem::create_directories(parent, ec);
    // 先写临时文件再改名，避免读到半截内容。
    const std::string temp = path + ".tmp";
    FILE* file = std::fopen(temp.c_str(), "wb");
    if (file == nullptr) {
        return;
    }
    const std::string text = serialize(distro);
    std::fwrite(text.data(), 1, text.size(), file);
    std::fclose(file);
    std::filesystem::rename(temp, path, ec);
    if (ec) {
        std::filesystem::remove(path, ec);
        std::filesystem::rename(temp, path, ec);
    }
}

}  // namespace

Distro probe_distro_uncached() {
    const char* id = read_os_release_id();
    if (std::strcmp(id, "deepin") == 0) {
        return Distro::Deepin;
    }
    if (file_exists("/etc/deepin-version")) {
        return Distro::Deepin;
    }
    if (desktop_mentions_deepin()) {
        return Distro::Deepin;
    }
    return Distro::Other;
}

Distro detect_distro() {
    Distro cached = Distro::Other;
    if (load_cache(cached)) {
        return cached;
    }
    const Distro detected = probe_distro_uncached();
    save_cache(detected);
    return detected;
}

const char* distro_name(Distro distro) {
    return distro == Distro::Deepin ? "deepin" : "other";
}

}  // namespace aether
