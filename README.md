# AetherUI

**纯 CPU 光栅的 Fluent Design 桌面控件库（C++20）**：SDL3 只负责窗口与输入，FreeType 只负责字形栅格化，圆角、抗锯齿、软阴影、图标与文本合成都由自写光栅完成 —— 不依赖 GPU、Skia、wgpu，虚拟机与远程桌面里也能满帧绘制。

- 演示程序：`aetherui.exe`，左侧 NavigationView + 八个页面
- 平台：Windows（MSVC 2019 / 2022 已验证）
- 协议：MIT

## 特性

- **控件**：Button、TextBox、Dropdown、ToggleSwitch、NavigationView、ScrollBar / ScrollView、TitleBar、Label / PageHeader、Card、IconTile
- **控件自洽**：命中测试、事件分发、焦点轮转（`FocusManager`）、面板互斥（`DropdownGroup`）、主题切换（`ThemeManager`）、滚动与裁剪都在 `ui/` 层完成，演示窗口只负责组合与转发
- **主题**：亮 / 暗两套 Fluent Token（颜色、圆角、阴影、动效、排版），控件内不硬编码样式
- **输入框**：鼠标拖选、双击全选、Shift 扩选、按词跳转与删除、`Ctrl+A/C/X/V/Z/Y` 撤销重做、水平滚动裁剪、IME 候选窗跟随光标
- **图标**：126 个 Fluent 图标，构建期从 SVG 生成命令流，运行期栅格化成 alpha 位图缓存；浅色模式自动切换加粗变体
- **窗口**：标题与图标一次传给 `App`，图标可以是内置图标名（`"SPARKLE"`）或 SVG（`"sparkle.svg"`），同时用于自绘标题栏和任务栏 / Alt-Tab 图标
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
| 图标资源 | `assets/icons` | 构建期生成 + 运行期可选 | 126 个 Fluent 图标 × 常规/加粗 = 252 个 SVG（MIT）；图标内联进 exe，`App` 传入 `.svg` 时才在启动期解析 |

**刻意不依赖**：Skia / wgpu / Vulkan / OpenGL / Qt / ImGui / HarfBuzz / 图像解码库（工程不读 PNG、JPG）。

## 实现清单

| 模块 | 文件 | 行数 | 已实现内容 |
|---|---|---|---|
| 像素表面 | [surface.h](cpp/src/gfx/surface.h) | 30 | 预乘 `0xAARRGGBB` 缓冲、直接映射窗口表面、裁剪矩形 |
| 光栅内核 | [raster.cpp](cpp/src/gfx/raster.cpp) | 274 | 扫描线 + 非零环绕、纵向 8 子扫描线、横向解析面积抗锯齿；路径（线 / 二次与三次贝塞尔 / 矩形 / 圆角 / 圆）；盒式模糊近似高斯 |
| 绘制层 | [painter.cpp](cpp/src/gfx/painter.cpp) | 208 | 矩形 / 圆角 / 圆填充、贴边描边、直线、径向色晕、软阴影、蒙版填充、字形叠加、裁剪栈 |
| 平台层 | [window.cpp](cpp/src/platform/window.cpp) + [distro.cpp](cpp/src/platform/distro.cpp) + [paths.cpp](cpp/src/platform/paths.cpp) + [clipboard.cpp](cpp/src/platform/clipboard.cpp) | 818 + 171 + 36 + 23 | 无边框窗口、自绘标题栏拖动与八向拉伸、DPI 缩放、最大化 / 最小化、原生窗口图标、IME 候选窗锚点、系统深浅查询与主题变更事件、发行版探测与缓存、assets 查找、剪贴板、事件（含修饰键与组字） |
| 文本 | [font.cpp](cpp/src/text/font.cpp) | 329 | 1024² 字形图集、7 族 / 字重回退栈、中英混排断行、避头尾标点、测量与排版 |
| 图标 | [icons.cpp](cpp/src/ui/icons.cpp) + [icon_data.cpp](cpp/src/ui/icon_data.cpp) | 239 + 4832 | 命令流数据表、运行期栅格化 alpha 位图缓存、着色、浅色自动用加粗变体 |
| 应用图标 | [app_icon.cpp](cpp/src/ui/app_icon.cpp) + [svg.cpp](cpp/src/ui/svg.cpp) | 91 + 402 | 图标来源统一为「内置图标名」或「SVG 文件」：启动时解析一次，既能自绘标题栏 logo，也能栅格化成任务栏图标；SVG 支持 M/L/H/V/C/S/Q/T/Z 与相对指令、viewBox 缩放 |
| 主题 | [theme.cpp](cpp/src/ui/theme.cpp) + [theme_manager.cpp](cpp/src/ui/theme_manager.cpp) | 192 + 37 | 亮 / 暗两套 Fluent Token（颜色、圆角、阴影、动效、排版、Mica 色）、三次贝塞尔缓动；跟随系统 / 显式浅深由 ThemeManager 统一决策 |
| 控件基类 | [widget.cpp](cpp/src/ui/widget.cpp) + [input.h](cpp/src/ui/input.h) + [focus.cpp](cpp/src/ui/focus.cpp) | 143 + 78 + 85 | `Widget` 命中 / 事件 / 焦点接口、事件批量转发、四态、Transition 与缓动、排版等级与文本颜色角色、`WidgetHost` 宿主能力、焦点轮转 |
| Label / PageHeader | [label.cpp](cpp/src/ui/label.cpp) | 61 | 按排版等级渲染的单行 / 自动换行文本、标题 + 说明组合 |
| Card / IconTile | [card.cpp](cpp/src/ui/card.cpp) + [icon_tile.cpp](cpp/src/ui/icon_tile.cpp) | 15 + 50 | 圆角卡片底盘；图标 + 名称 + 右下角动作按钮的图标卡片（自带命中） |
| Button | [button.cpp](cpp/src/ui/button.cpp) | 145 | Primary / Standard × 图标 / 文字 / 禁用、四态、自处理悬停 / 按下 / 点击回调 |
| ToggleSwitch | [toggle_switch.cpp](cpp/src/ui/toggle_switch.cpp) | 228 | 滑动动画、按压拉伸、拖拽吸附、禁用 |
| TextBox | [text_box.cpp](cpp/src/ui/text_box.cpp) | 717 | 光标闪烁、鼠标拖选、双击全选、Shift 扩选、按词跳转删除、快捷键与剪贴板清洗、撤销重做、水平滚动裁剪、IME 组字与候选窗锚点、数字过滤、清除按钮、占位符 |
| Dropdown | [dropdown.cpp](cpp/src/ui/dropdown.cpp) | 331 | 展开动画、上方 / 居中 / 下方三种摆放、带图标 / 禁用菜单项、选中回调、行级命中、`DropdownGroup` 互斥展开与点外收起 |
| NavigationView | [navigation.cpp](cpp/src/ui/navigation.cpp) | 328 | 汉堡折叠 56↔240、指示条冲刺 + 拉伸动画、悬停 / 选中态、页脚项、条目点击回调 |
| ScrollBar / ScrollView | [scroll_bar.cpp](cpp/src/ui/scroll_bar.cpp) + [scroll_view.cpp](cpp/src/ui/scroll_view.cpp) | 142 + 60 | 12px 占位 / 6px 可视厚度、悬浮渐显、拖动与滚轮；视口裁剪 + 内容高度 + 滚动条联动的可滚动区域 |
| TitleBar | [title_bar.cpp](cpp/src/ui/title_bar.cpp) | 167 | 可配置应用图标 + 标题、最小化 / 最大化 / 还原 / 关闭四键、悬停底色、关闭红、双击最大化、原生拖动，窗口操作经 `WidgetHost` 下发 |
| Gallery | [gallery.cpp](cpp/src/app/gallery.cpp) | 830 | 八个页面（主页 / 输入框 / 按钮 / 下拉框 / 开关 / 字体 / 图标 / 关于）+ 主题下拉切换；图标页带名称搜索（复用 TextBox，右侧内嵌放大镜，实时过滤图标卡片）；只做组合、摆放与事件转发 |
| App | [app.cpp](cpp/src/app/app.cpp) + [app.h](cpp/src/app/app.h) | 101 | 窗口持有、标题与图标唯一来源、主循环、动画时钟、事件轮询、60fps 帧节流、无表面时让出 CPU，界面只挂事件 / 绘制回调 |
| 入口 | [main.cpp](cpp/src/app/main.cpp) | 170 | 标题与图标常量、字体发现、挂载 App 回调、`--shot` 离屏截图（可指定明暗与页面） |

合计约 1.1 万行，其中 4.8k 行是自动生成的图标数据，手写约 6.2k 行。

## 尚未实现

| 类别 | 缺失项 |
|---|---|
| 控件 | Slider、ProgressBar、Flyout、ToolTip、ContentDialog、Menu、TabView、InfoBar、ListView |
| 文本 | 多行编辑、富文本、HarfBuzz 复杂整形（阿拉伯文 / 印度文）、右键菜单 |
| 窗口 | 真实 Mica / Acrylic 模糊、多显示器 DPI 热变更 |
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

### 打包 deb（Linux）

网页上就能出包：仓库 **Actions → build-deb → Run workflow**，版本号留空即 `0.1.0+<短 SHA>`；
跑完在 job 的 Artifacts 里下载 `aetherui-deb`。推送 `v*` tag 时同一个工作流会把 deb 挂到对应 Release。

工作流内容见 [deb.yml](.github/workflows/deb.yml)：装构建依赖 → 拉取 SDL3 3.2.0 与 FreeType 2.13.3
（`cpp/third_party` 不入库）→ 编译 → 冒烟测试（`SDL_VIDEODRIVER=offscreen` 离屏渲染一帧）→ 打包。

本机（Ubuntu）也可以直接跑同一个脚本：

```bash
sudo apt install build-essential cmake ninja-build dpkg-dev
packaging/build-deb.sh 0.1.0        # 产物：build-deb/aetherui_0.1.0_amd64.deb
sudo apt install ./build-deb/aetherui_0.1.0_amd64.deb
```

安装后的布局：`/usr/bin/aetherui`、`/usr/share/aetherui/assets/{fonts,icons}`、
`/usr/share/applications/aetherui.desktop`、`/usr/share/icons/hicolor/scalable/apps/aetherui.svg`。
运行时依赖由 `dpkg-shlibdeps` 扫 ELF 自动推导，字体与图标路径通过
`-DAETHERUI_FONT_DIR` / `-DAETHERUI_ASSET_DIR` 编译期写死为安装路径。

### Linux / X11 排查开关

| 环境变量 | 作用 |
| --- | --- |
| `AETHERUI_LOG=1` | 启动时把 `driver / present / opaque / size / scale / distro` 打到 stderr |
| `AETHERUI_X11_NO_ALPHA=1` | 退回不透明窗口：不发 `SDL_WINDOW_TRANSPARENT`、不做 XShape 裁剪，代价是 X11 上窗口变直角 |

默认会关掉 SDL 的 `SDL_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR` 提示——它会给窗口打
`_NET_WM_BYPASS_COMPOSITOR`，DDE（deepin 25 X11）会因此把任务栏与桌面从合成层里摘掉，
表现为「程序运行时任务栏和桌面消失」。

## 目录结构

```text
aetherui/
├─ assets/
│  ├─ fonts/                 # 内嵌字体（更纱黑体 / JetBrains Mono / Noto Symbols / Noto Emoji）
│  └─ icons/                 # Fluent SVG 图标（常规 + 加粗，共 252 个）
├─ packaging/                # deb 打包：build-deb.sh + aetherui.desktop
├─ .github/workflows/        # build-deb（网页 Run workflow 出包，tag 时挂 Release）
└─ cpp/
   ├─ CMakeLists.txt
   ├─ tools/gen_icons.js     # SVG → src/ui/icon_data.cpp
   └─ src/
      ├─ app/                # app.cpp 主循环与帧节流、main.cpp 入口、gallery.cpp 八个演示页（只做组合与事件转发）
      ├─ gfx/                # surface / raster（扫描线光栅）/ painter
      ├─ platform/           # SDL3 窗口、输入法锚点、边框拖动与拉伸、系统深浅与发行版探测、assets 查找、剪贴板
      ├─ text/               # FreeType 字形图集与断行
      └─ ui/                 # 输入契约 + 主题 + 各控件 + 图标
```

## 主题与 Token

- 颜色、圆角、阴影、动效与排版集中在 [theme.h](cpp/src/ui/theme.h)（`Theme::light()` / `Theme::dark_theme()`）。
- 演示窗口右上角的下拉框可选「跟随系统 / 浅色 / 深色」。
- 「跟随系统」完全封装在平台层：启动时 `Window::system_dark()` 读 `SDL_GetSystemTheme()`（Windows 个性化、Linux portal），运行中由 `SDL_EVENT_SYSTEM_THEME_CHANGED` 转成 `Event::Type::SystemTheme` 通知界面；界面层不做任何平台判断。
- 发行版在窗口创建时探测一次并写入缓存（Windows `%LOCALAPPDATA%\AetherUI\platform.cache`、Linux `$XDG_CACHE_HOME/aetherui/platform.cache`），当前只需区分 deepin，由 `Window::distro()` 提供给上层取 DDE 主题色。
- 手动选择「浅色 / 深色」后进入显式模式，系统主题变化不再覆盖。
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
