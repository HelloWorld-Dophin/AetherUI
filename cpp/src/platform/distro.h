//! 发行版识别：目前只需要区分「是不是 deepin」，用于后续取 DDE 主题色。
//!
//! 判定顺序：`/etc/os-release` 的 `ID` → `/etc/deepin-version` 是否存在 →
//! `XDG_CURRENT_DESKTOP` 是否含 `Deepin`。结果写入缓存文件，下次启动直接读取，
//! 系统升级或缓存损坏时自动重探。

#pragma once

namespace aether {

enum class Distro { Other, Deepin };

/// 读缓存；缓存缺失或失效时实时探测并写回。
Distro detect_distro();

/// 强制重新探测并覆盖缓存（调试 / 排障用）。
Distro probe_distro_uncached();

const char* distro_name(Distro distro);

}  // namespace aether
