//! 资源路径：可执行文件目录与 assets 搜索。

#pragma once

#include <string>

namespace aether {

/// 可执行文件所在目录（以路径分隔符结尾）。
std::string executable_dir();

/// 在可执行文件附近的 `assets/` 下查找资源（如 `"icons/sparkle.svg"`）；
/// 依次尝试可执行文件目录往上 0~3 层与当前工作目录，找不到返回空串。
std::string find_asset(const std::string& relative);

}  // namespace aether
