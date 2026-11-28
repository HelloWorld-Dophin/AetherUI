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
install -Dm644 "$ROOT/assets/icons/sparkle.svg" \
        "$PKG/usr/share/icons/hicolor/scalable/apps/aetherui.svg"

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
rm -f "$BUILD/aetherui_${VERSION}_${ARCH}.deb"
dpkg-deb --build --root-owner-group "$PKG" "$BUILD/aetherui_${VERSION}_${ARCH}.deb"
ls -lh "$BUILD"/*.deb
