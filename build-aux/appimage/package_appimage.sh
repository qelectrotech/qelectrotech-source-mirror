#!/usr/bin/env bash
#
# build-aux/appimage/package_appimage.sh
#
# Configure, build, bundle, and package QElectroTech as an AppImage.
# Works identically whether called by CI or run by hand.
#
# This script assumes the source tree is already at the state you want to
# build -- it does NOT check out, pull, or reset anything. That's
# deliberate: the scripts this replaces did a destructive `git reset --hard`
# on the sources/ directory and a `git pull`, which is fine for a
# maintainer's own repeatedly-reused local checkout but wrong for CI (which
# already checks out the exact right commit) and risky for anyone else
# running this by hand who might have uncommitted local changes.
#
# Currently Qt6 only, on Ubuntu 22.04 (x86_64) / 22.04-arm (aarch64),
# matching linux-build-check.yml's already-proven environment. Raspberry Pi
# / embedded Wayland targets are deliberately out of scope for now -- the
# old aarch64 script this replaces was tuned for that specific case
# (EXTRA_PLATFORM_PLUGINS, EXTRA_QT_MODULES for Wayland compositor support);
# revisit as its own separate effort if that's still needed.
#
# Usage:
#   ./package_appimage.sh [--arch=x86_64|aarch64]
#
# Examples:
#   ./package_appimage.sh                  # native arch, auto-detected
#   ./package_appimage.sh --arch=aarch64   # explicit

set -euo pipefail

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------
ARCH="$(uname -m)"   # x86_64 or aarch64 on Linux -- NOT "arm64" (that's macOS's uname -m spelling)

for arg in "$@"; do
  case "$arg" in
    --arch=*) ARCH="${arg#*=}" ;;
    -h|--help)
      sed -n '2,20p' "$0"
      exit 0
      ;;
    *)
      echo "ERROR: unknown argument: $arg" >&2
      exit 1
      ;;
  esac
done

case "$ARCH" in
  x86_64|aarch64) ;;
  *) echo "ERROR: --arch must be x86_64 or aarch64, got: $ARCH" >&2; exit 1 ;;
esac

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$SOURCE_DIR/build-appimage-$ARCH"
INSTALL_DIR="$SOURCE_DIR/install-appimage-$ARCH"
APPDIR="$SOURCE_DIR/AppDir-$ARCH"
TOOLS_DIR="$SOURCE_DIR/appimage-tools-$ARCH"
APPNAME="QElectroTech"

VERSION="$(cat "$SOURCE_DIR/sources/qetversion.cpp" | grep "return QVersionNumber{" | head -n 1 | awk -F "{" '{ print $2 }' | awk -F "}" '{ print $1 }' | sed -e 's/,/./g' -e 's/ //g')"
HEAD="$(git -C "$SOURCE_DIR" rev-parse --short HEAD)"
APPIMAGE_NAME="${APPNAME}-${VERSION}-r${HEAD}-${ARCH}.AppImage"

echo "=== Packaging $APPNAME $VERSION r$HEAD ($ARCH) ==="

# ---------------------------------------------------------------------------
# Fetch linuxdeploy + linuxdeploy-plugin-qt
#
# linuxdeployqt (the tool the original scripts used) is unmaintained --
# linuxdeploy + linuxdeploy-plugin-qt is its actively-maintained successor,
# explicitly recommended by the linuxdeploy project itself.
#
# NOTE: the "continuous" release tag is confirmed, documented, and working
# for x86_64. I could NOT confirm with confidence that aarch64 builds are
# published under the same "continuous" tag -- the error handling below is
# deliberately specific about this possibility rather than a generic
# download failure, since if this is the problem, the fix is a different
# (pinned) release tag, not a retry.
# ---------------------------------------------------------------------------
mkdir -p "$TOOLS_DIR"
cd "$TOOLS_DIR"

fetch_tool() {
  local name="$1" url="$2"
  if [ ! -f "$name" ]; then
    echo "Fetching $name..."
    if ! wget -q -O "$name" "$url"; then
      echo "ERROR: failed to download $name from:" >&2
      echo "  $url" >&2
      echo "If this is the aarch64 leg: linuxdeploy's 'continuous' release may not" >&2
      echo "publish aarch64 builds the same way it does for x86_64 (unconfirmed" >&2
      echo "either way at the time this script was written). Check" >&2
      echo "https://github.com/linuxdeploy/linuxdeploy/releases and" >&2
      echo "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases" >&2
      echo "directly for whatever tag actually has an aarch64 asset, and pin to" >&2
      echo "that specific tag instead of 'continuous' if so." >&2
      exit 1
    fi
    chmod +x "$name"
  fi
}

fetch_tool "linuxdeploy-$ARCH.AppImage" \
  "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-$ARCH.AppImage"
fetch_tool "linuxdeploy-plugin-qt-$ARCH.AppImage" \
  "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-$ARCH.AppImage"

cd "$SOURCE_DIR"

# ---------------------------------------------------------------------------
# Configure, build (CMake -- replaces the old qmake ../qelectrotech.pro / make)
# ---------------------------------------------------------------------------
cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DQT_VERSION_MAJOR=6 \
  -DBUILD_WITH_KF=OFF \
  -DPACKAGE_TESTS=OFF \
  -DCMAKE_POLICY_DEFAULT_CMP0077=NEW \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5

cmake --build "$BUILD_DIR" -j"$(nproc)"
cmake --install "$BUILD_DIR" --prefix "$INSTALL_DIR"

# ---------------------------------------------------------------------------
# Build the AppDir structure fresh each run
# ---------------------------------------------------------------------------
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/share/applications" \
  "$APPDIR/usr/share/icons/hicolor/256x256/apps"

EXE="$(find "$INSTALL_DIR" -maxdepth 3 -type f -name "qelectrotech" | head -1)"
if [ -z "$EXE" ]; then
  echo "ERROR: qelectrotech executable not found under $INSTALL_DIR" >&2
  exit 1
fi
cp "$EXE" "$APPDIR/usr/bin/"

cp "$SOURCE_DIR/build-aux/linux/org.qelectrotech.qelectrotech.desktop" \
  "$APPDIR/usr/share/applications/qelectrotech.desktop"

cp "$SOURCE_DIR/ico/breeze-icons/256x256/apps/"*.png \
  "$APPDIR/usr/share/icons/hicolor/256x256/apps/" 2>/dev/null || \
  echo "WARNING: no icons found under ico/breeze-icons/256x256/apps/" >&2

for dir in elements examples titleblocks; do
  src="$SOURCE_DIR/$dir"
  if [ -d "$src" ]; then
    cp -r "$src" "$APPDIR/usr/share/"
  else
    echo "WARNING: expected $src, not found -- skipping" >&2
  fi
done

mkdir -p "$APPDIR/usr/share/lang"
if [ -d "$SOURCE_DIR/lang" ]; then
  find "$SOURCE_DIR/lang" -maxdepth 1 -name "*.qm" -exec cp {} "$APPDIR/usr/share/lang/" \;
else
  echo "WARNING: expected $SOURCE_DIR/lang, not found -- skipping" >&2
fi

# ---------------------------------------------------------------------------
# Deploy Qt dependencies via linuxdeploy + its Qt plugin
# ---------------------------------------------------------------------------
# linuxdeploy-plugin-qt calls qmake internally to locate Qt's paths -- on
# Ubuntu, Qt6's qmake is named "qmake6" (not plain "qmake", which may not
# exist at all, or may be a stray/non-functional stub from something
# unrelated). Point the plugin at the real one explicitly rather than let
# it guess.
if [ -x /usr/lib/qt6/bin/qmake6 ]; then
  QMAKE_BINARY=/usr/lib/qt6/bin/qmake6
else
  QMAKE_BINARY="$(find /usr -maxdepth 4 -name "qmake6" -type f -executable 2>/dev/null | head -1)"
fi
if [ -z "$QMAKE_BINARY" ] || [ ! -x "$QMAKE_BINARY" ]; then
  echo "ERROR: qmake6 not found under /usr -- is qt6-tools-dev-tools installed?" >&2
  exit 1
fi
echo "Using qmake6 at: $QMAKE_BINARY"
export QMAKE="$QMAKE_BINARY"

DESKTOP_FILE="$APPDIR/usr/share/applications/qelectrotech.desktop"
export PATH="$TOOLS_DIR:$PATH"
"$TOOLS_DIR/linuxdeploy-$ARCH.AppImage" --appdir "$APPDIR" --desktop-file "$DESKTOP_FILE" --plugin qt

# linuxdeploy generates its own generic AppRun; replace it with one that
# passes QElectroTech's own resource-path flags explicitly, so the binary
# finds its bundled elements/titleblocks/lang wherever the AppImage happens
# to be run from (AppImages are relocatable -- there's no fixed install
# path to rely on).
rm -f "$APPDIR/AppRun"
cat > "$APPDIR/AppRun" << 'APPRUN_EOF'
#!/bin/sh
cd "$(dirname "$(readlink -f "$0")")/usr/bin/"
exec ./qelectrotech --common-elements-dir=../share/elements/ --common-tbt-dir=../share/titleblocks/ --lang-dir=../share/lang/ "$@"
APPRUN_EOF
chmod +x "$APPDIR/AppRun"

# ---------------------------------------------------------------------------
# Build the AppImage itself
# ---------------------------------------------------------------------------
cd "$SOURCE_DIR"
VERSION="$VERSION-r$HEAD" "$TOOLS_DIR/linuxdeploy-$ARCH.AppImage" --appdir "$APPDIR" --desktop-file "$DESKTOP_FILE" --output appimage

PRODUCED="$(find "$SOURCE_DIR" -maxdepth 1 -iname "QElectroTech*-$ARCH.AppImage" | head -1)"
if [ -z "$PRODUCED" ]; then
  echo "ERROR: no AppImage produced -- expected something matching QElectroTech*-$ARCH.AppImage" >&2
  exit 1
fi
if [ "$PRODUCED" != "$SOURCE_DIR/$APPIMAGE_NAME" ] && [ "$(basename "$PRODUCED")" != "$APPIMAGE_NAME" ]; then
  mv "$PRODUCED" "$APPIMAGE_NAME"
fi

shasum -a 256 "$APPIMAGE_NAME" > "$APPIMAGE_NAME-SHA256.txt"

echo "=== Done: $APPIMAGE_NAME ==="
