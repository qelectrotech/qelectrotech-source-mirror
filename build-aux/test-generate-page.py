#!/usr/bin/env bash
#
# test-generate-page.sh
#
# Sets every environment variable generate-page.py currently reads to a
# realistic, randomized-where-it-matters value, then runs the script --
# lets you preview gh-pages/index.html locally without a real CI run.
#
# Usage:
#   ./test-generate-page.sh
#   open gh-pages/index.html      # macOS
#   xdg-open gh-pages/index.html  # Linux
#
# To test the "some platforms didn't build" case, comment out (or blank)
# any of the optional URL exports below before running -- each one's
# corresponding card/button is only rendered when its URL is non-empty.

set -euo pipefail

REPO="qelectrotech/qelectrotech-source-mirror"
VERSION="0.200.1"
TAG="development-test"

SHA="$(openssl rand -hex 20)"
SHORT="${SHA:0:7}"
RUN_NUMBER=$(( RANDOM % 900 + 100 ))

export REPO
export DATE="$(date -u '+%Y-%m-%d %H:%M UTC')"
export SHORT
export SHA
export RUN_URL="https://github.com/${REPO}/actions/runs/$(( RANDOM * RANDOM ))"
export RUN_NUMBER
export RELEASE_TAG="$TAG"

BASE="https://github.com/${REPO}/releases/download/${TAG}"

# --- Windows (single track, Qt6 only) -----------------------------------
export INSTALLER_URL="${BASE}/QElectroTech-${VERSION}_x86_64-win64.exe"
export PORTABLE_URL="${BASE}/qelectrotech-${VERSION}-x86-win64-readytouse.zip"
export MSI_URL="${BASE}/QElectroTech-${VERSION}_x86_64-win64.msi"

# --- macOS (single track, Qt6 only) -------------------------------------
export DMG_ARM64_URL="${BASE}/QElectroTech-${VERSION}-arm64.dmg"
export DMG_X8664_URL="${BASE}/QElectroTech-${VERSION}-x86_64.dmg"

# --- Linux AppImage (Qt6 only) -------------------------------------------
export APPIMAGE_X8664_URL="${BASE}/QElectroTech-${VERSION}-x86_64.AppImage"
export APPIMAGE_AARCH64_URL="${BASE}/QElectroTech-${VERSION}-aarch64.AppImage"

# --- Linux Snap (unsigned, not yet on the Snap Store) --------------------
# Note the underscore-based naming -- Snapcraft's own convention, distinct
# from the hyphenated names everything else uses.
export SNAP_AMD64_URL="${BASE}/qelectrotech_${VERSION}_amd64.snap"
export SNAP_ARM64_URL="${BASE}/qelectrotech_${VERSION}_arm64.snap"

# --- Linux Flatpak ---------------------------------------------------------
export FLATPAK_X8664_URL="${BASE}/qelectrotech-${VERSION}-x86_64.flatpak"
export FLATPAK_AARCH64_URL="${BASE}/qelectrotech-${VERSION}-aarch64.flatpak"

echo "=== Environment set, running generate-page.py ==="
echo "REPO         : $REPO"
echo "RELEASE_TAG  : $RELEASE_TAG"
echo "SHA / SHORT  : $SHA / $SHORT"
echo "RUN_NUMBER   : $RUN_NUMBER"
echo ""

python3 generate-page.py

echo ""
echo "Done -- open gh-pages/index.html to preview."
