#!/usr/bin/env bash
#
# build-aux/macos/package_dmg.sh
#
# Configure, build, bundle, (optionally) sign, (optionally) notarize, and
# package QElectroTech as a macOS DMG. Works identically whether called by
# CI or run by hand on a developer's own Mac.
#
# This script assumes the source tree is already at the state you want to
# build -- it does NOT check out, pull, or update anything. In CI that's
# actions/checkout's job. For local repeated use, see update_and_package.sh
# in this same directory, which pulls first and then calls this script.
#
# Usage:
#   ./package_dmg.sh [--arch=arm64|x86_64] [--qt-version=5|6] \
#                     [--sign] [--notarize] [--non-interactive]
#
# Examples:
#   ./package_dmg.sh                                       # quick local test, unsigned
#   ./package_dmg.sh --sign                                # signed, no notarization
#   ./package_dmg.sh --sign --notarize --non-interactive   # full CI pipeline
#
# Signing identity and notarization credentials are read from environment
# variables (see "Signing" and "Notarization" sections below) so this script
# never hardcodes anyone's personal identity. See README.md in this
# directory for the full list of variables and how to set them up, both
# locally and in CI.

set -euo pipefail

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------
ARCH="$(uname -m)"          # arm64 or x86_64 -- auto-detected unless overridden
QT_VERSION_MAJOR=6
DO_SIGN=false
DO_NOTARIZE=false
NON_INTERACTIVE=false

for arg in "$@"; do
  case "$arg" in
    --arch=*)          ARCH="${arg#*=}" ;;
    --qt-version=*)     QT_VERSION_MAJOR="${arg#*=}" ;;
    --sign)              DO_SIGN=true ;;
    --notarize)          DO_NOTARIZE=true ;;
    --non-interactive)   NON_INTERACTIVE=true ;;
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

if [ "$DO_NOTARIZE" = true ] && [ "$DO_SIGN" = false ]; then
  echo "ERROR: --notarize requires --sign (Apple will not notarize an unsigned app)." >&2
  exit 1
fi

case "$ARCH" in
  arm64|x86_64) ;;
  *) echo "ERROR: --arch must be arm64 or x86_64, got: $ARCH" >&2; exit 1 ;;
esac

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$SOURCE_DIR/build-macos-$ARCH"
INSTALL_DIR="$SOURCE_DIR/install-macos-$ARCH"
APPNAME="QElectroTech"

QT_SUFFIX=""
if [ "$QT_VERSION_MAJOR" = "5" ]; then
  QT_SUFFIX="-qt5"
fi
VERSION="$(tr -d '[:space:]' < "$SOURCE_DIR/QET_VERSION")"
RELEASE="$(tr -d '[:space:]' < "$SOURCE_DIR/QET_RELEASE")"
HEAD="$(git -C "$SOURCE_DIR" rev-parse --short HEAD)"

case "$RELEASE" in
  dev)
    SUFFIX="-r${HEAD}"
    ;;
  alpha1|alpha2|alpha3)
    SUFFIX="-${RELEASE}"
    ;;
  stable)
    SUFFIX=""
    ;;
  *)
    echo "ERROR: unrecognized RELEASE value '$RELEASE' (expected dev, alpha1, alpha2, alpha3, or stable)" >&2
    exit 1
    ;;
esac

DMG_NAME="${APPNAME}-${VERSION}${SUFFIX}-${ARCH}${QT_SUFFIX}.dmg"

echo "=== Packaging $APPNAME $VERSION r$HEAD ($ARCH, Qt$QT_VERSION_MAJOR) ==="
echo "    sign=$DO_SIGN  notarize=$DO_NOTARIZE  non-interactive=$NON_INTERACTIVE"

# ---------------------------------------------------------------------------
# Locate Qt (via Homebrew, matching macos-build.yml's own convention)
# ---------------------------------------------------------------------------
if [ "$QT_VERSION_MAJOR" = "5" ]; then
  QT_PREFIX="$(brew --prefix qt@5)"
else
  QT_PREFIX="$(brew --prefix qt)"
fi
MACDEPLOYQT="$QT_PREFIX/bin/macdeployqt"

if [ ! -x "$MACDEPLOYQT" ]; then
  echo "ERROR: macdeployqt not found at $MACDEPLOYQT" >&2
  exit 1
fi

# ---------------------------------------------------------------------------
# Determine the real minimum macOS version from Qt itself, rather than a
# hardcoded guess. QtCore's own LC_BUILD_VERSION load command records the
# actual minimum OS the installed Qt build requires -- this is the real,
# binding floor for the whole bundle, since our own
# CMAKE_OSX_DEPLOYMENT_TARGET can't go lower than this and mean anything --
# dyld refuses to load Qt on anything older regardless of what we claim.
# ---------------------------------------------------------------------------
if [ -f "$QT_PREFIX/lib/QtCore.framework/QtCore" ]; then
  QTCORE_BINARY="$QT_PREFIX/lib/QtCore.framework/QtCore"
else
  QTCORE_BINARY="$(find "$QT_PREFIX/lib" -maxdepth 2 \( -name "libQt5Core.dylib" -o -name "libQt6Core.dylib" -o -name "libQtCore.dylib" \) -type f 2>/dev/null | head -1)"
fi

if [ -z "$QTCORE_BINARY" ] || [ ! -f "$QTCORE_BINARY" ]; then
  echo "ERROR: could not locate QtCore under $QT_PREFIX/lib -- cannot determine minimum macOS version" >&2
  echo "Contents of $QT_PREFIX/lib:" >&2
  find "$QT_PREFIX/lib" -maxdepth 2 >&2
  exit 1
fi

echo "Found QtCore at: $QTCORE_BINARY"

VTOOL_OUTPUT="$(vtool -show-build "$QTCORE_BINARY" 2>&1)" || {
  echo "ERROR: vtool failed on $QTCORE_BINARY:" >&2
  echo "$VTOOL_OUTPUT" >&2
  exit 1
}
echo "=== vtool output ==="
echo "$VTOOL_OUTPUT"

# old style still included for Qt5; can be safely removed in favor of commented line when Qt5 gets retired
# MACOS_DEPLOYMENT_TARGET="$(echo "$VTOOL_OUTPUT" | grep "minos" | awk '{print $2}' | sort -V | tail -1)"
MACOS_DEPLOYMENT_TARGET="$(echo "$VTOOL_OUTPUT" | awk '
  /^Load command/ { in_verminmacosx=0; in_buildversion=0 }
  /cmd LC_VERSION_MIN_MACOSX/ { in_verminmacosx=1 }
  /cmd LC_BUILD_VERSION/ { in_buildversion=1 }
  in_verminmacosx && /^[[:space:]]*version[[:space:]]/ { print $2; in_verminmacosx=0 }
  in_buildversion && /^[[:space:]]*minos[[:space:]]/ { print $2 }
' | sort -V | tail -1 || true)"

if [ -z "$MACOS_DEPLOYMENT_TARGET" ]; then
  echo "ERROR: could not determine minimum macOS version from $QTCORE_BINARY" >&2
  echo "(vtool ran successfully but produced no 'minos' line -- see vtool output above)" >&2
  exit 1
fi

echo "Detected minimum macOS version from Qt: $MACOS_DEPLOYMENT_TARGET"
# ---------------------------------------------------------------------------
# Configure, build (CMake -- replaces the old qmake -spec macx-clang / make)
# ---------------------------------------------------------------------------
cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$QT_PREFIX" \
  -DQT_VERSION_MAJOR="$QT_VERSION_MAJOR" \
  -DCMAKE_OSX_ARCHITECTURES="$ARCH" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="$MACOS_DEPLOYMENT_TARGET" \
  -DBUILD_WITH_KF=OFF \
  -DPACKAGE_TESTS=OFF \
  -DQET_EXPORT_PROJECT_DB=ON

cmake --build "$BUILD_DIR" -j"$(sysctl -n hw.ncpu)"
cmake --install "$BUILD_DIR" --prefix "$INSTALL_DIR"

BUNDLE="$(find "$INSTALL_DIR" -maxdepth 2 -name "*.app" | head -1)"
if [ -z "$BUNDLE" ]; then
  echo "ERROR: no .app bundle found under $INSTALL_DIR" >&2
  exit 1
fi
echo "Bundle: $BUNDLE"

# ---------------------------------------------------------------------------
# Reorganize CMake's flat install output into the bundle structure
#
# CMakeLists.txt's install() rules for the resource directories (lang,
# elements, examples, titleblocks) and the icon are not yet bundle-aware --
# they land in a flat layout alongside the bundle rather than inside it.
# This is intentionally handled here rather than in CMake for now (see the
# "Option A vs B" discussion) -- easier to iterate on without a full
# reconfigure/rebuild each time.
# ---------------------------------------------------------------------------
RESOURCES="$BUNDLE/Contents/Resources"
mkdir -p "$RESOURCES"

# QET_LANG_PATH's "../Resources/lang/" is relative to $INSTALL_DIR itself
# (not the bundle), so it lands one directory above $INSTALL_DIR.
LANG_SRC="$(dirname "$INSTALL_DIR")/Resources/lang"
if [ -d "$LANG_SRC" ]; then
  mv "$LANG_SRC" "$RESOURCES/lang"
else
  echo "WARNING: expected lang files at $LANG_SRC, not found -- skipping" >&2
fi

# elements/examples/titleblocks land flat under $INSTALL_DIR/share/qelectrotech.
SHARE_QET="$INSTALL_DIR/share/qelectrotech"
for dir in elements examples titleblocks; do
  if [ -d "$SHARE_QET/$dir" ]; then
    mv "$SHARE_QET/$dir" "$RESOURCES/$dir"
  else
    echo "WARNING: expected $dir at $SHARE_QET/$dir, not found -- skipping" >&2
  fi
done

# The .icns is never installed by CMake at all yet -- Info.plist already
# references "qelectrotech.icns" as CFBundleIconFile, so it just needs to
# actually exist at this path.
ICNS_SRC="$SOURCE_DIR/ico/mac_icon/qelectrotech.icns"
if [ -f "$ICNS_SRC" ]; then
  cp "$ICNS_SRC" "$RESOURCES/qelectrotech.icns"
else
  echo "WARNING: icon not found at $ICNS_SRC -- bundle will use a default icon" >&2
fi

# Clean up now-empty leftover directories from the flat install.
rmdir "$(dirname "$INSTALL_DIR")/Resources" 2>/dev/null || true
rm -rf "$SHARE_QET" 2>/dev/null || true

# Patch the real version into Info.plist -- CFBundleShortVersionString ships
# empty in the source tree's Info.plist, filled in here at build time.
/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString $VERSION" \
  "$BUNDLE/Contents/Info.plist"
/usr/libexec/PlistBuddy -c "Set :LSMinimumSystemVersion $MACOS_DEPLOYMENT_TARGET" \
  "$BUNDLE/Contents/Info.plist"
  
  
# ---------------------------------------------------------------------------
# Bundle Qt frameworks
#
# NOTE: once CMakeLists.txt gains qt_generate_deploy_app_script(), this
# explicit macdeployqt call becomes unnecessary -- `cmake --install` above
# would trigger it automatically. Left explicit for now since that CMake
# change hasn't landed yet.
# ---------------------------------------------------------------------------
"$MACDEPLOYQT" "$BUNDLE"

# ---------------------------------------------------------------------------
# Signing
#
# Identity resolution, in order:
#   1. QET_SIGNING_IDENTITY env var, if set (CI, or a developer overriding it)
#   2. The first valid "Developer ID Application" identity already present
#      in whichever keychain is active -- covers a maintainer running this
#      locally with their own certificate already in their login keychain.
#
# If --sign was requested but no identity can be found, this is a hard
# error (you asked for signing, you should know if it silently didn't
# happen). If --sign was NOT requested, packaging proceeds unsigned with a
# clear warning -- this is what lets basic CI packaging work today, before
# any certificate is configured at all.
# ---------------------------------------------------------------------------
TEMP_KEYCHAIN=""

cleanup_keychain() {
  if [ -n "$TEMP_KEYCHAIN" ]; then
    security delete-keychain "$TEMP_KEYCHAIN" 2>/dev/null || true
  fi
}
trap cleanup_keychain EXIT

if [ "$DO_SIGN" = true ]; then
  if [ -n "${MACOS_CERTIFICATE_P12_BASE64:-}" ]; then
    # --- CI mode: import the certificate into a dedicated, ephemeral keychain ---
    echo "Importing signing certificate into a temporary keychain..."
    TEMP_KEYCHAIN="qet-signing-$$.keychain-db"
    KEYCHAIN_PASSWORD="$(uuidgen)"

    security create-keychain -p "$KEYCHAIN_PASSWORD" "$TEMP_KEYCHAIN"
    security set-keychain-settings -lut 900 "$TEMP_KEYCHAIN"
    security unlock-keychain -p "$KEYCHAIN_PASSWORD" "$TEMP_KEYCHAIN"

    CERT_PATH="$(mktemp)"
    echo "$MACOS_CERTIFICATE_P12_BASE64" | base64 --decode > "$CERT_PATH"
    
    echo "=== Diagnostics: decoded cert file ==="
    ls -la "$CERT_PATH"
    echo "SHA256: $(shasum -a 256 "$CERT_PATH" | awk '{print $1}')"
    file "$CERT_PATH"
    openssl pkcs12 -in "$CERT_PATH" -info -noout -passin "pass:$MACOS_CERTIFICATE_PASSWORD" 2>&1 || echo "openssl could not parse the decoded file"    
    
    security import "$CERT_PATH" -k "$TEMP_KEYCHAIN" \
      -P "${MACOS_CERTIFICATE_PASSWORD:?MACOS_CERTIFICATE_PASSWORD must be set}" \
      -T /usr/bin/codesign
    rm -f "$CERT_PATH"

    security set-key-partition-list -S apple-tool:,apple: -s -k "$KEYCHAIN_PASSWORD" "$TEMP_KEYCHAIN"
    security list-keychains -d user -s "$TEMP_KEYCHAIN" $(security list-keychains -d user | tr -d '"')

    IDENTITY="$(security find-identity -v -p codesigning "$TEMP_KEYCHAIN" | grep "Developer ID Application" | head -1 | sed -E 's/.*"(.*)"/\1/')"
  else
    # --- Local mode: use whatever's already in the default keychain ---
    IDENTITY="${QET_SIGNING_IDENTITY:-$(security find-identity -v -p codesigning | grep "Developer ID Application" | head -1 | sed -E 's/.*"(.*)"/\1/')}"
  fi

  if [ -z "${IDENTITY:-}" ]; then
    echo "ERROR: --sign requested but no 'Developer ID Application' identity found." >&2
    exit 1
  fi
  echo "Signing with identity: $IDENTITY"

  # Sign every bundled dylib and framework individually before signing the
  # bundle itself -- macdeployqt's own signing isn't always sufficient for
  # nested frameworks, hence the explicit pass here.
  find "$BUNDLE/Contents/Frameworks" -name "*.dylib" -print0 2>/dev/null | \
    while IFS= read -r -d '' lib; do
      codesign --force --sign "$IDENTITY" --timestamp --options=runtime "$lib"
    done
  find "$BUNDLE/Contents/Frameworks" -name "*.framework" -maxdepth 1 -print0 2>/dev/null | \
    while IFS= read -r -d '' fw; do
      codesign --force --sign "$IDENTITY" --timestamp --options=runtime "$fw"
    done
  find "$BUNDLE/Contents/PlugIns" -name "*.dylib" -print0 2>/dev/null | \
    while IFS= read -r -d '' lib; do
      codesign --force --sign "$IDENTITY" --timestamp --options=runtime "$lib"
    done

  codesign --force --sign "$IDENTITY" --timestamp --options=runtime "$BUNDLE"
  codesign --verify --deep --strict --verbose=2 "$BUNDLE"
  echo "Signing OK."
else
  echo "WARNING: --sign not requested -- producing an UNSIGNED bundle."
fi

# ---------------------------------------------------------------------------
# Notarize the .app (before it goes into the DMG)
# ---------------------------------------------------------------------------
notarize() {
  local target="$1"
  local zip_path
  zip_path="$(mktemp -t qet-notarize).zip"
  /usr/bin/ditto -c -k --keepParent "$target" "$zip_path"

  if [ -n "${NOTARY_API_KEY_BASE64:-}" ]; then
    # --- CI mode: App Store Connect API key, passed explicitly ---
    local key_path
    key_path="$(mktemp -t qet-notary-key).p8"
    echo "$NOTARY_API_KEY_BASE64" | base64 --decode > "$key_path"
    xcrun notarytool submit "$zip_path" \
      --key "$key_path" \
      --key-id "${NOTARY_KEY_ID:?NOTARY_KEY_ID must be set}" \
      --issuer "${NOTARY_ISSUER_ID:?NOTARY_ISSUER_ID must be set}" \
      --wait
    local status=$?
    rm -f "$key_path"
  else
    # --- Local mode: relies on a keychain profile already stored via
    #     `xcrun notarytool store-credentials` on this machine ---
    xcrun notarytool submit "$zip_path" \
      --keychain-profile "${QET_NOTARIZE_PROFILE:-org.qelectrotech}" \
      --wait
    local status=$?
  fi

  rm -f "$zip_path"
  return $status
}

if [ "$DO_NOTARIZE" = true ]; then
  if [ "$NON_INTERACTIVE" = false ]; then
    echo -e "\033[1;31mNotarize the .app \"${APPNAME}-${VERSION}-r${HEAD}\"? n/Y\033[m"
    read -r ans
    [ "$ans" = "n" ] && DO_NOTARIZE_APP=false || DO_NOTARIZE_APP=true
  else
    DO_NOTARIZE_APP=true
  fi

  if [ "$DO_NOTARIZE_APP" = true ]; then
    echo "Notarizing .app..."
    if ! notarize "$BUNDLE"; then
      echo "ERROR: app notarization failed. Check the log with:" >&2
      echo "  xcrun notarytool log <submission-id> --keychain-profile ${QET_NOTARIZE_PROFILE:-org.qelectrotech}" >&2
      exit 1
    fi
    xcrun stapler staple "$BUNDLE"
    echo "App notarized and stapled OK."
  fi
fi

# ---------------------------------------------------------------------------
# Create the DMG
# ---------------------------------------------------------------------------
DMG_STAGING="$(mktemp -d)"
cp -R "$BUNDLE" "$DMG_STAGING/"
ln -s /Applications "$DMG_STAGING/Applications"

for attempt in 1 2 3; do
  if hdiutil create -ov -srcfolder "$DMG_STAGING" -format UDBZ -volname "$APPNAME" "$DMG_NAME"; then
    break
  fi
  echo "WARNING: hdiutil create failed (attempt $attempt/3), retrying..." >&2
  sleep 5
  [ "$attempt" = 3 ] && exit 1
done
rm -rf "$DMG_STAGING"

echo "DMG created: $DMG_NAME"

# ---------------------------------------------------------------------------
# Sign + notarize the DMG itself
# ---------------------------------------------------------------------------
if [ "$DO_SIGN" = true ]; then
  codesign --sign "$IDENTITY" --timestamp "$DMG_NAME"
fi

if [ "$DO_NOTARIZE" = true ] && [ "${DO_NOTARIZE_APP:-false}" = true ]; then
  echo "Notarizing DMG..."
  if ! notarize "$DMG_NAME"; then
    echo "ERROR: DMG notarization failed. Check the log with:" >&2
    echo "  xcrun notarytool log <submission-id> --keychain-profile ${QET_NOTARIZE_PROFILE:-org.qelectrotech}" >&2
    exit 1
  fi
  xcrun stapler staple "$DMG_NAME"
  echo "DMG notarized and stapled OK."
fi

echo "=== Done: $DMG_NAME ==="
