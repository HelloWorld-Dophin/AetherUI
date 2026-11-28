#!/usr/bin/env bash
# 打一个 .deb：二进制装到 /usr/bin，字体与图标装到 /usr/share/aetherui，
# 运行时依赖由 dpkg-shlibdeps 从 ELF 里自动推导。
#
# 用法：packaging/build-deb.sh [版本号]
#   版本号默认 0.1.0+<短 SHA>；产物：build-deb/aetherui_<版本>_<架构>.deb
# 依赖（Ubuntu）：build-essential cmake ninja-build dpkg-dev，以及 cpp/third_party 下的 SDL3 / FreeType。
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
VERSION=${1:-0.1.0+$(git -C "$ROOT" rev-parse --short HEAD 2>/dev/null || echo local)}
ARCH=$(dpkg --print-architecture)
BUILD=${BUILD_DIR:-$ROOT/build-deb}
STAGE=$BUILD/stage
PKG=$STAGE/aetherui_${VERSION}_${ARCH}

echo "==> 配置（$VERSION / $ARCH）"
cmake -S "$ROOT/cpp" -B "$BUILD" -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr \
      -DAETHERUI_FONT_DIR=/usr/share/aetherui/assets/fonts \
      -DAETHERUI_ASSET_DIR=/usr/share/aetherui

echo "==> 编译"
cmake --build "$BUILD" -j"$(nproc)"

echo "==> 装入临时目录"
rm -rf "$STAGE"
mkdir -p "$PKG/DEBIAN"
cmake --install "$BUILD" --prefix "$PKG/usr"
install -Dm644 "$ROOT/packaging/aetherui.desktop" \
        "$PKG/usr/share/applications/aetherui.desktop"
# 应用图标是独立配色的品牌图标；内置的 Fluent 图标（如 sparkle.svg）是 #212121 单色，
# 直接拿来当桌面图标在任务栏 / 开始菜单里会是黑的。
install -Dm644 "$ROOT/packaging/aetherui.svg" \
        "$PKG/usr/share/icons/hicolor/scalable/apps/aetherui.svg"
# 任务栏 / 启动器按实际像素尺寸取图标，位图版能让小尺寸更锐利（没有 rsvg-convert 就只用 SVG）。
if command -v rsvg-convert >/dev/null 2>&1; then
    for size in 16 24 32 48 64 128 256; do
        install -d "$PKG/usr/share/icons/hicolor/${size}x${size}/apps"
        rsvg-convert -w "$size" -h "$size" \
            "$ROOT/packaging/aetherui.svg" \
            -o "$PKG/usr/share/icons/hicolor/${size}x${size}/apps/aetherui.png"
    done
fi

echo "==> 推导运行时依赖"
DEPS_DIR=$BUILD/shlibdeps
rm -rf "$DEPS_DIR"
mkdir -p "$DEPS_DIR/debian"
printf 'Source: aetherui\n\nPackage: aetherui\nArchitecture: any\n' > "$DEPS_DIR/debian/control"
SHLIBS=$(cd "$DEPS_DIR" && dpkg-shlibdeps -O -e"$PKG/usr/bin/aetherui" 2>/dev/null \
         | sed -n 's/^shlibs:Depends=//p')
SHLIBS=${SHLIBS:-libc6, libstdc++6}

echo "==> 写 DEBIAN/control"
cat > "$PKG/DEBIAN/control" <<EOF
Package: aetherui
Version: $VERSION
Architecture: $ARCH
Maintainer: AetherUI <noreply@github.com>
Installed-Size: $(du -sk "$PKG" | cut -f1)
Depends: $SHLIBS
Section: graphics
Priority: optional
Homepage: https://github.com/HelloWorld-Dophin/AetherUI
Description: Fluent 风格 C++ 控件库演示
 AetherUI 是一个纯 CPU 光栅的 Fluent 风格控件库，
 演示按钮 / 输入框 / 下拉框 / 开关 / 字体 / 图标等控件。
EOF

echo "==> 生成 deb"
# 装完刷新图标缓存与桌面数据库，否则 DDE 会继续用旧图标（黑色的那个）。
cat > "$PKG/DEBIAN/postinst" <<'EOF'
#!/bin/sh
set -e
if [ -x "$(command -v gtk-update-icon-cache)" ]; then
    gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor || true
fi
if [ -x "$(command -v update-desktop-database)" ]; then
    update-desktop-database -q /usr/share/applications || true
fi
exit 0
EOF
chmod 755 "$PKG/DEBIAN/postinst"

rm -f "$BUILD/aetherui_${VERSION}_${ARCH}.deb"
dpkg-deb --build --root-owner-group "$PKG" "$BUILD/aetherui_${VERSION}_${ARCH}.deb"
ls -lh "$BUILD"/*.deb
