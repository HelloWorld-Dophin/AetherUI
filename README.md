# AetherUI

**纯 CPU 光栅的 Fluent Design 桌面控件库（C++20）**：SDL3 只负责窗口与输入，FreeType 只负责字形栅格化，圆角、抗锯齿、软阴影、图标与文本合成都由自写光栅完成 —— 不依赖 GPU、Skia、wgpu，虚拟机与远程桌面里也能满帧绘制。

- 演示程序：`aetherui.exe`，左侧 NavigationView + 八个页面
- 平台：Windows（MSVC 2019 / 2022 已验证）
- 协议：MIT

## 特性

- **控件**：Button、TextBox、Dropdown、ToggleSwitch、NavigationView、ScrollBar / ScrollView、TitleBar
- **主题**：亮 / 暗两套 Fluent Token（颜色、圆角、阴影、动效、排版），控件内不硬编码样式
- **输入框**：鼠标拖选、双击全选、Shift 扩选、按词跳转与删除、`Ctrl+A/C/X/V/Z/Y` 撤销重做、水平滚动裁剪、IME 候选窗跟随光标
- **图标**：126 个 Fluent 图标，构建期从 SVG 生成命令流，运行期栅格化成 alpha 位图缓存；浅色模式自动切换加粗变体
- **字体**：内嵌更纱黑体 / JetBrains Mono / Noto Symbols / Noto Emoji，中英混排断行，中文标点不落行首
- **交互**：四态（Normal / PointerOver / Pressed / Disabled）、过渡动画、下拉面板三种摆放、开关拖拽吸附、无边框自绘标题栏

## 依赖

| 依赖 | 来源 | 链接方式 | 用途 | 备注 |
|---|---|---|---|---|
| SDL3 | `cpp/third_party/SDL` | 静态（`SDL3::SDL3`） | 窗口、鼠标/键盘事件、输入法与剪贴板、窗口像素表面 | 只做窗口与输入，**不创建 GPU 渲染器** |
| FreeType | `cpp/third_party/freetype` | 静态 | 字形栅格化、字宽与基线查询 | 关闭 zlib / bzip2 / png / harfbuzz / brotli |
| C++20 标准库 | MSVC 2019 | 系统 | 容器、字符串、数学、算法 | 无其他 C++ 第三方库 |
| Windows 系统 DLL | 随 SDL 内部链接 | 静态间接 | user32 / gdi32 / imm32 等（DPI、IME 窗口、键盘布局） | 业务代码不直接调用 Win32 |
| Node.js | 本机 | 仅**构建期**工具 | `cpp/tools/gen_icons.js` 把 SVG 转成 `icon_data.cpp` | 不参与编译产物 |
| 字体资源 | `assets/fonts` | 运行时加载 | 更纱黑体 UI SC ×3、JetBrains Mono ×2、Noto Symbols 2、Noto Emoji（OFL） | 约 71 MB，不打包进 exe |
| 图标资源 | `assets/icons` | 构建期生成 | 126 个 Fluent 图标 × 常规/加粗 = 252 个 SVG（MIT） | 运行期不读 SVG |

**刻意不依赖**：Skia / wgpu / Vulkan / OpenGL / Qt / ImGui / HarfBuzz / 图像解码库（工程不读 PNG、JPG）。

## 实现清单

| 模块 | 文件 | 行数 | 已实现内容 |
|---|---|---|---|
| 像素表面 | [surface.h](cpp/src/gfx/surface.h) | 30 | 预乘 `0xAARRGGBB` 缓冲、直接映射窗口表面、裁剪矩形 |
| 光栅内核 | [raster.cpp](cpp/src/gfx/raster.cpp) | 274 | 扫描线 + 非零环绕、纵向 8 子扫描线、横向解析面积抗锯齿；路径（线 / 二次与三次贝塞尔 / 矩形 / 圆角 / 圆）；盒式模糊近似高斯 |
| 绘制层 | [painter.cpp](cpp/src/gfx/painter.cpp) | 208 | 矩形 / 圆角 / 圆填充、贴边描边、直线、径向色晕、软阴影、蒙版填充、字形叠加、裁剪栈 |
| 平台层 | [window.cpp](cpp/src/platform/window.cpp) | 382 | 无边框窗口、自绘标题栏拖动与八向拉伸、DPI 缩放、最大化 / 最小化、IME 候选窗锚点、事件（含修饰键与组字） |
| 文本 | [font.cpp](cpp/src/text/font.cpp) | 329 | 1024² 字形图集、7 族 / 字重回退栈、中英混排断行、避头尾标点、测量与排版 |
| 图标 | [icons.cpp](cpp/src/ui/icons.cpp) + [icon_data.cpp](cpp/src/ui/icon_data.cpp) | 212 + 4573 | 命令流数据表、运行期栅格化 alpha 位图缓存、着色、浅色自动用加粗变体 |
| 主题 | [theme.cpp](cpp/src/ui/theme.cpp) | 156 | 亮 / 暗两套 Fluent Token（颜色、圆角、阴影、动效、排版、Mica 色）、三次贝塞尔缓动 |
| 状态与动画 | [widget.cpp](cpp/src/ui/widget.cpp) | 91 | 四态（Normal / Hover / Pressed / Disabled）、Transition 与缓动 |
| Button | [button.cpp](cpp/src/ui/button.cpp) | 98 | Primary / Standard × 图标 / 文字 / 禁用、四态、点击计数 |
| ToggleSwitch | [toggle_switch.cpp](cpp/src/ui/toggle_switch.cpp) | 175 | 滑动动画、按压拉伸、拖拽吸附、禁用 |
| TextBox | [text_box.cpp](cpp/src/ui/text_box.cpp) | 446 | 光标闪烁、鼠标拖选、双击全选、Shift 扩选、按词跳转删除、剪贴板与撤销重做、水平滚动裁剪、IME 组字、数字过滤、清除按钮、占位符 |
| Dropdown | [dropdown.cpp](cpp/src/ui/dropdown.cpp) | 200 | 展开动画、上方 / 居中 / 下方三种摆放、带图标 / 禁用菜单项、选中回调、互斥展开 |
| NavigationView | [navigation.cpp](cpp/src/ui/navigation.cpp) | 250 | 汉堡折叠 56↔240、指示条冲刺 + 拉伸动画、悬停 / 选中态、页脚项 |
| ScrollBar / ScrollView | [scroll_bar.cpp](cpp/src/ui/scroll_bar.cpp) | 88 | 12px 占位 / 6px 可视厚度、悬浮渐显、拖动与滚轮 |
| TitleBar | [title_bar.cpp](cpp/src/ui/title_bar.cpp) | 79 | 标题、最小化 / 最大化 / 还原 / 关闭四键、悬停底色、关闭红、双击最大化 |
| Gallery | [gallery.cpp](cpp/src/app/gallery.cpp) | 1190 | 八个页面（主页 / 输入框 / 按钮 / 下拉框 / 开关 / 字体 / 图标 / 关于）+ 主题下拉切换 |
| 入口 | [main.cpp](cpp/src/app/main.cpp) | 176 | 事件循环、字体发现、`--shot` 离屏截图（可指定明暗与页面） |

合计约 1.0 万行，其中 4.6k 行是自动生成的图标数据，手写约 5.4k 行。

## 尚未实现

| 类别 | 缺失项 |
|---|---|
| 控件 | Slider、ProgressBar、Flyout、ToolTip、ContentDialog、Menu、TabView、InfoBar、ListView |
| 文本 | 多行编辑、富文本、HarfBuzz 复杂整形（阿拉伯文 / 印度文）、右键菜单 |
| 窗口 | 真实 Mica / Acrylic 模糊、窗口圆角、系统主题跟随、多显示器 DPI 热变更 |
| 输入 | 悬停光标形状（I 型 / 手型）、触摸与笔、拖放文件 |

## 渲染方案

| 能力 | 实现 |
|---|---|
| 抗锯齿 | 扫描线 + 非零环绕，纵向 8 子扫描线、横向解析面积 |
| 圆角矩形 | 解析式圆角路径 → 覆盖率蒙版 |
| 柔和阴影 | 圆角蒙版 + 三次盒式模糊（近似高斯） |
| 描边 | 外轮廓顺时针 + 内轮廓逆时针挖环带，贴控件外沿画 1px 硬边 |
| 文本 | FreeType 栅格化 → 1024² 单通道字形图集，字宽累加断行 |
| 图标 | SVG 路径 → 命令流 → 蒙版缓存 → 着色叠加 |
| 背景 | 底色 + 低频色晕模拟 Mica |

窗口表面就是最终像素缓冲，逐帧由 CPU 合成后直接呈现，因此关闭 3D 加速的环境也能正常渲染。

## 快速开始

### 环境要求

| 项目 | 要求 |
|---|---|
| 编译器 | MSVC 2019 / 2022（C++20） |
| CMake | 3.20+ |
| 依赖源码 | SDL3 与 FreeType 放在 `cpp/third_party/`（未纳入版本库），随工程静态编译 |
| 字体 | `assets/fonts/` 下的更纱黑体 / JetBrains Mono / Noto 字体文件 |

### 构建与运行

```powershell
$cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
& $cmake -S cpp -B cpp/build
& $cmake --build cpp/build --config Release --target aetherui -- /m
.\cpp\build\Release\aetherui.exe
```

### 离屏截图

用于像素核对，不开窗口：

```powershell
.\aetherui.exe --shot out.ppm dark dropdown
.\aetherui.exe --shot out.ppm light icons
```

参数：`--shot <文件> [light|dark] [home|textbox|button|dropdown|toggle|fonts|icons|about]`，
可选第 6 个参数强制展开某个面板（`theme` / `d0`~`d3` / `p0`~`p2` / `collapse`）。

## 目录结构

```text
aetherui/
├─ assets/
│  ├─ fonts/                 # 内嵌字体（更纱黑体 / JetBrains Mono / Noto Symbols / Noto Emoji）
│  └─ icons/                 # Fluent SVG 图标（常规 + 加粗，共 252 个）
└─ cpp/
   ├─ CMakeLists.txt
   ├─ tools/gen_icons.js     # SVG → src/ui/icon_data.cpp
   └─ src/
      ├─ app/                # main.cpp 窗口循环、gallery.cpp 八个演示页
      ├─ gfx/                # surface / raster（扫描线光栅）/ painter
      ├─ platform/           # SDL3 窗口、输入法锚点、边框拖动与拉伸
      ├─ text/               # FreeType 字形图集与断行
      └─ ui/                 # theme + 各控件 + 图标
```

## 主题与 Token

- 颜色、圆角、阴影、动效与排版集中在 [theme.h](cpp/src/ui/theme.h)（`Theme::light()` / `Theme::dark_theme()`）。
- 演示窗口右上角的下拉框可切换亮色 / 暗色。
- 输入框聚焦、悬停、按下等状态全部从 Token 取值，样式改动只需改一处。

## 图标

- 来源：Fluent UI System Icons（MIT），统一 24px 视框。
- 图标清单与顺序在 `cpp/tools/gen_icons.js` 的 `ICON_NAMES` 中；新增图标把 SVG 放进 `assets/icons/` 并追加名称，然后重新生成：

```powershell
node cpp/tools/gen_icons.js
```

- 运行期通过 `icons::index_of("HOME")` 取下标，`icons::draw(...)` 绘制。

## 字体

| 用途 | 内嵌字体（OFL） | 字体栈下标 |
|---|---|---|
| 界面西文 + 中文 | Sarasa UI SC（Light / Regular / SemiBold） | 1 / 0 / 2 |
| 符号 | Noto Sans Symbols 2 | 3 |
| 表情 | Noto Emoji | 4 |
| 代码等宽 | JetBrains Mono（Regular / SemiBold） | 5 / 6 |

- 中英混排按字宽累加断行，中文按字断行、西文按空格断行。
- 只使用真实字重，不做伪粗体合成。

## 许可

[MIT](LICENSE)
