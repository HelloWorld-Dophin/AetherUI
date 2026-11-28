//! 剪贴板：控件只与本接口打交道，平台细节留在 platform 层。

#pragma once

#include <string>

namespace aether {

/// 读取系统剪贴板文本（无内容时返回空串）。
std::string clipboard_text();
/// 写入系统剪贴板文本（空串不写入）。
void set_clipboard_text(const std::string& value);

}  // namespace aether
