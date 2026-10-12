#!/bin/sh

# Copyright 2026 The QElectroTech Team
# This file is part of QElectroTech.
# QElectroTech is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.

# Suppose que l'environnement a ete prepare via macos_homebrew_setup.sh
# (Homebrew, Qt6, cmake, ninja, kf6-kwidgetsaddons, kf6-kcoreaddons).
#
# Remplace l'ancien misc/MacQetDeploy_arm64.sh (qmake/Qt5) par un
# build CMake/Qt6/KF6. La chaine de signature/notarization/DMG est
# reprise a l'identique de l'ancien script (eprouvee, ne pas y toucher
# sans raison).

# configuration
APPNAME='qelectrotech'
BUNDLE=$APPNAME.app
IDENTITY="Developer ID Application: Laurent TRINQUES (Y73WZ6WZ5X)"

QT_MAJOR="${QT_MAJOR:-6}"
BUILD_WITH_KF="${BUILD_WITH_KF:-ON}"
BUILD_DIR="build-macos-arm64"

# Temp paths
RW_DMG="/tmp/qet_rw.dmg"
MOUNT_POINT="/tmp/qet_dmg_mount"
STAGING="/tmp/qet_dmg_staging"

# Script location
current_dir=$(dirname "$0")
cd "${current_dir}/../"
current_dir=$(PWD)

### get system configuration ########################################
echo
echo "______________________________________________________________"
echo "This script prepares a Qt6/KF6 application bundle for deployment."
echo "This script :"
echo "\t - update the git depot"
echo "\t - configure and build via CMake,"
echo "\t - copy over required Qt frameworks,"
echo "\t - copy additional files: translations, titleblocks and elements,"
echo "\t - notarize the .app, then create a signed DMG."
echo

QT_PREFIX=$(brew --prefix qt 2>/dev/null || true)
if [ -z "$QT_PREFIX" ] || [ ! -d "$QT_PREFIX/lib/QtCore.framework" ] ; then
    echo "ERROR: cannot find Qt6 via Homebrew. Run macos_homebrew_setup.sh first."
    exit 1
fi
export CMAKE_PREFIX_PATH="$QT_PREFIX:$CMAKE_PREFIX_PATH"

### GIT ####################################################
echo
echo "______________________________________________________________"
echo "Run GIT:"
git submodule init
git submodule update
git pull --recurse-submodules
git pull

GITCOMMIT=$(git rev-parse --short HEAD)
A=$(git rev-list HEAD --count)
HEAD=$(($A+473))
VERSION=$(cat sources/qetversion.cpp | grep "return QVersionNumber{"| head -n 1| awk -F "{" '{ print $2 }' | awk -F "}" '{ print $1 }' | sed -e 's/,/./g' -e 's/ //g')
DMG_NAME="${APPNAME}-$VERSION-r$HEAD-arm64.dmg"
DMG_PATH="build-aux/mac-osx/$DMG_NAME"

if [ -e "$DMG_PATH" ] ; then
    echo "There are not new updates, make disk image can"
    echo "take a lot of time (5 min). Can you continu?"
    echo "[y/n]"
    read userinput
    if [ "$userinput" == "n" ] ; then
        echo
        echo "Process is stopped."
        echo
        exit
    fi
fi

### build with CMake #################################################
echo
echo "______________________________________________________________"
echo "Run CMake configure + build (Qt${QT_MAJOR}, BUILD_WITH_KF=${BUILD_WITH_KF}):"

if [ -d $BUNDLE ] ; then
    echo "Removing old bundle..."
    rm -rf $BUNDLE
fi

if [ -d "$BUILD_DIR" ] ; then
    echo "Removing old build directory..."
    rm -rf "$BUILD_DIR"
fi

cmake -S . -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DQT_VERSION_MAJOR=$QT_MAJOR \
    -DBUILD_WITH_KF=$BUILD_WITH_KF \
    -DBUILD_KF=OFF \
    -DQET_EXPORT_PROJECT_DB=ON \
    -DPACKAGE_TESTS=OFF

if [ $? -ne 0 ]; then
    echo "ERROR: cmake configure failed."
    exit 1
fi

START_TIME=$SECONDS
coeur=$(sysctl hw.ncpu | awk '{print $2}')

cmake --build "$BUILD_DIR" -j$(($coeur + 1))

if [ $? -ne 0 ]; then
    ELAPSED_TIME=$(($SECONDS - $START_TIME))
    echo
    echo "cmake build failed - $(($ELAPSED_TIME/60)) min $(($ELAPSED_TIME%60)) sec"
    exit 1
fi

ELAPSED_TIME=$(($SECONDS - $START_TIME))
echo
echo "The time of compilation is $(($ELAPSED_TIME/60)) min $(($ELAPSED_TIME%60)) sec"

# Le .app sort a la racine de $BUILD_DIR : add_executable(... MACOSX_BUNDLE)
# sans RUNTIME_OUTPUT_DIRECTORY dans CMakeLists.txt.
echo "Copying built bundle into place..."
cp -R "$BUILD_DIR/qelectrotech.app" "./$BUNDLE"

if [ ! -d "$BUNDLE" ] ; then
    echo "ERROR: expected bundle \"$BUNDLE\" not found after cmake build."
    exit 1
fi

### copy over frameworks ############################################
echo
echo "______________________________________________________________"
echo "Copy Qt libraries and private frameworks:"

if [ ! -d $BUNDLE ] ; then
    echo "ERROR: cannot find application bundle \"$BUNDLE\" in current directory"
    exit 1
fi

macdeployqt $BUNDLE

### fix @rpath dependencies macdeployqt could not resolve ###########
# Recent Homebrew bottles (brotli, webp, sharpyuv...) reference their own
# dependencies as @rpath/libX.dylib. macdeployqt only resolves @rpath in
# Contents/lib and in Qt's lib dir (-libpath does not help), so it prints
# "Cannot resolve rpath" and leaves them out. Copy them from
# /opt/homebrew/lib into Contents/Frameworks: the executable's LC_RPATH
# (@executable_path/../Frameworks) lets dyld find them there. They are
# signed below with everything else in Frameworks/.
echo
echo "______________________________________________________________"
echo "Fix unresolved @rpath dependencies:"

FW="$BUNDLE/Contents/Frameworks"

# List the @rpath/libX.dylib names referenced by every Mach-O in the bundle
list_rpath_libs() {
    find "$BUNDLE/Contents/MacOS" "$FW" "$BUNDLE/Contents/PlugIns" -type f \
        \( -name '*.dylib' -o -perm -u+x \) 2>/dev/null | while read bin; do
        otool -L "$bin" 2>/dev/null \
            | awk 'NR>1 && $1 ~ /^@rpath\/[^\/]+\.dylib$/ { sub("^@rpath/", "", $1); print $1 }'
    done | LC_ALL=C sort -u
}

if ! otool -l "$BUNDLE/Contents/MacOS/$APPNAME" | grep -q "@executable_path/../Frameworks" ; then
    install_name_tool -add_rpath "@executable_path/../Frameworks" "$BUNDLE/Contents/MacOS/$APPNAME"
    echo "  Added LC_RPATH @executable_path/../Frameworks to $APPNAME"
fi

for PASS in 1 2 3; do
    list_rpath_libs | while read lib; do
        if [ ! -e "$FW/$lib" ] && [ -e "/opt/homebrew/lib/$lib" ]; then
            cp -L "/opt/homebrew/lib/$lib" "$FW/$lib"
            chmod u+w "$FW/$lib"
            echo "  Copied (pass $PASS): $lib"
        fi
    done
done

UNRESOLVED=$(list_rpath_libs | while read lib; do [ -e "$FW/$lib" ] || echo "$lib"; done)
if [ -n "$UNRESOLVED" ]; then
    echo "ERROR: @rpath libraries still missing from Frameworks/:" $UNRESOLVED
    exit 1
fi
HOMEBREW_REFS=$(find "$BUNDLE/Contents/MacOS" "$FW" "$BUNDLE/Contents/PlugIns" -type f \
    \( -name '*.dylib' -o -perm -u+x \) -exec otool -L {} \; 2>/dev/null | grep "/opt/homebrew/" || true)
if [ -n "$HOMEBREW_REFS" ]; then
    echo "ERROR: bundle still references Homebrew paths:"
    echo "$HOMEBREW_REFS"
    exit 1
fi
echo "All @rpath dependencies resolved inside the bundle."

### install Info.plist and app icon #################################
# NOTE: this must run AFTER macdeployqt, not before. macdeployqt
# rewrites/regenerates parts of Contents/Resources, and files copied
# there beforehand (e.g. the .icns icons) do not reliably survive it
# and end up missing from the final bundle, causing the app to show
# the generic placeholder icon instead of the real one. Info.plist
# itself lives directly in Contents/ and happens to survive either
# way, but keep it here too so this whole "final metadata" step stays
# in one place, after macdeployqt is done touching the bundle.
echo
echo "______________________________________________________________"
echo "Install Info.plist and app icon:"

cp -R ${current_dir}/misc/Info.plist $BUNDLE/Contents/
cp -R ${current_dir}/ico/mac_icon/*.icns $BUNDLE/Contents/Resources/
/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString $VERSION r$HEAD" "$BUNDLE/Contents/Info.plist"

### add missing files ###############################################
echo
echo "______________________________________________________________"
echo "Copy missing files:"

QET_ELMT_DIR="${current_dir}/elements/"
QET_TBT_DIR="${current_dir}/titleblocks/"
QET_LANG_DIR="${current_dir}/lang/"
QET_EXAMPLES_DIR="${current_dir}/examples/"
QET_FONTS_DIR="${current_dir}/fonts/"
QET_LICENSES_DIR="${current_dir}/licenses/"

if [ -d "${QET_ELMT_DIR}" ]; then
    cp -R ${QET_ELMT_DIR} $BUNDLE/Contents/Resources/elements
fi
if [ -d "${QET_TBT_DIR}" ]; then
    cp -R ${QET_TBT_DIR} $BUNDLE/Contents/Resources/titleblocks
fi
# Traductions : depuis la PR #751, les .qm ne sont plus versionnes ; lrelease
# les genere dans $BUILD_DIR/lang/. Jeu attendu = les .ts listes dans TS_FILES
# (cmake/qet_compilation_vars.cmake) : un .qm manquant arrete le script, un
# .ts present dans lang/ mais absent de TS_FILES donne seulement un WARNING.
# Fichiers temporaires plutot que <(...) : /bin/sh de macOS (bash 3.2 en mode
# POSIX) n'a pas la substitution de processus.
QM_SRC="${current_dir}/${BUILD_DIR}/lang"
QM_TMP=$(mktemp -d /tmp/qet_qm.XXXXXX)
grep -o 'lang/qet_[A-Za-z_]*\.ts' "${current_dir}/cmake/qet_compilation_vars.cmake" \
    | sed -e 's#^lang/##' -e 's#\.ts$##' | LC_ALL=C sort -u > "$QM_TMP/listed"
if [ ! -s "$QM_TMP/listed" ]; then
    echo "ERROR: cannot read TS_FILES from cmake/qet_compilation_vars.cmake"
    rm -rf "$QM_TMP"
    exit 1
fi
find "${QET_LANG_DIR}" -maxdepth 1 -name 'qet_*.ts' -exec basename {} .ts \; | LC_ALL=C sort > "$QM_TMP/present"
UNLISTED=$(LC_ALL=C comm -13 "$QM_TMP/listed" "$QM_TMP/present")
if [ -n "$UNLISTED" ]; then
    echo "WARNING: .ts files not in TS_FILES, no .qm built:" $UNLISTED
fi
mkdir -p $BUNDLE/Contents/Resources/lang
find "${QM_SRC}" -maxdepth 1 -name 'qet_*.qm' -exec cp {} $BUNDLE/Contents/Resources/lang/ \; 2>/dev/null
find $BUNDLE/Contents/Resources/lang -maxdepth 1 -name 'qet_*.qm' -exec basename {} .qm \; | LC_ALL=C sort > "$QM_TMP/built"
MISSING=$(LC_ALL=C comm -23 "$QM_TMP/listed" "$QM_TMP/built")
echo "$(wc -l < "$QM_TMP/built" | tr -d ' ') .qm files copied to Contents/Resources/lang (expected: $(wc -l < "$QM_TMP/listed" | tr -d ' '))"
rm -rf "$QM_TMP"
if [ -n "$MISSING" ]; then
    echo "ERROR: missing translations:" $MISSING
    exit 1
fi
if [ -d "${QET_EXAMPLES_DIR}" ]; then
    mkdir $BUNDLE/Contents/Resources/examples
    cp ${current_dir}/examples/*.qet $BUNDLE/Contents/Resources/examples
fi
if [ -d "${QET_FONTS_DIR}" ]; then
    mkdir $BUNDLE/Contents/Resources/fonts
    cp ${current_dir}/fonts/*.ttf $BUNDLE/Contents/Resources/fonts
fi
if [ -d "${QET_LICENSES_DIR}" ]; then
    cp -R -L ${QET_LICENSES_DIR} $BUNDLE/Contents/Resources/licenses
fi

### Sign the bundle #################################################
# Sign in correct order: all dylibs first (including flat libs copied
# by macdeployqt from Homebrew), then frameworks, plugins, bundle last.
echo
echo "______________________________________________________________"
echo "Code signing (dylibs -> frameworks -> plugins -> bundle):"

echo "-- Signing dylibs in Frameworks/..."
find "$BUNDLE/Contents/Frameworks" -name "*.dylib" | while read lib; do
    codesign --force --sign "$IDENTITY" --timestamp --options=runtime "$lib"
done

echo "-- Signing .framework bundles..."
find "$BUNDLE/Contents/Frameworks" -maxdepth 1 -name "*.framework" | while read fw; do
    codesign --force --sign "$IDENTITY" --timestamp --options=runtime "$fw"
done

echo "-- Signing plugins..."
find "$BUNDLE/Contents/PlugIns" \( -name "*.dylib" -o -name "*.so" \) | while read lib; do
    codesign --force --sign "$IDENTITY" --timestamp --options=runtime "$lib"
done

echo "-- Signing dylibs in MacOS/..."
find "$BUNDLE/Contents/MacOS" -name "*.dylib" | while read lib; do
    codesign --force --sign "$IDENTITY" --timestamp --options=runtime "$lib"
done

echo "-- Signing main executable..."
codesign --force --sign "$IDENTITY" --timestamp --options=runtime \
    "$BUNDLE/Contents/MacOS/$APPNAME"

echo "-- Signing bundle..."
codesign --force --sign "$IDENTITY" --timestamp --options=runtime "$BUNDLE"

echo
echo "Verifying bundle signature..."
codesign --verify --deep --strict --verbose=2 "$BUNDLE"
if [ $? -ne 0 ]; then
    echo "ERROR: bundle signature verification failed, aborting."
    exit 1
fi
spctl -a -vv "$BUNDLE"
echo "Bundle signature OK."

### Notarize the .app (via temporary ZIP) ###########################
echo
echo "______________________________________________________________"
echo "Create temporary ZIP for notarization:"
NOTARIZE_ZIP="/tmp/${APPNAME}-$VERSION-r$HEAD-arm64-notarize.zip"
/usr/bin/ditto -c -k --keepParent "$BUNDLE" "$NOTARIZE_ZIP"

echo -e "\033[1;31mWould you like to notarize the .app \"${APPNAME}-${VERSION}-r${HEAD}\", n/Y?\033[m"
read a
if [[ $a == "Y" || $a == "y" ]]; then
    echo
    echo "______________________________________________________________"
    echo "Notarizing .app:"
    xcrun notarytool submit "$NOTARIZE_ZIP" --keychain-profile "org.qelectrotech" --wait
    if [ $? -ne 0 ]; then
        echo "ERROR: notarization failed. Check the log with:"
        echo " xcrun notarytool log <submission-id> --keychain-profile org.qelectrotech"
        rm -f "$NOTARIZE_ZIP"
        exit 1
    fi
else
    echo -e "\033[1;33mExit.\033[m"
fi
rm -f "$NOTARIZE_ZIP"

### Staple the .app #################################################
echo -e "\033[1;31mWould you like to staple the .app \"${APPNAME}-${VERSION}-r${HEAD}\", n/Y?\033[m"
read a
if [[ $a == "Y" || $a == "y" ]]; then
    xcrun stapler staple -v "$BUNDLE"
    if [ $? -ne 0 ]; then
        echo "ERROR: stapling .app failed."
        exit 1
    fi
    xcrun stapler validate -v "$BUNDLE"
    spctl -a -vv "$BUNDLE"
    echo ".app stapled OK."
else
    echo -e "\033[1;33mExit.\033[m"
fi

### Create staging folder with Applications symlink #################
echo
echo "______________________________________________________________"
echo "Preparing DMG staging folder:"
rm -rf "$STAGING"
mkdir -p "$STAGING"
cp -R "$BUNDLE" "$STAGING/"
ln -s /Applications "$STAGING/Applications"

### Create writable DMG (UDRW) ######################################
echo
echo "______________________________________________________________"
echo "Create writable DMG (UDRW) and re-sign .app inside:"
rm -f "$RW_DMG"
hdiutil create \
    -volname "QElectroTech $VERSION" \
    -srcfolder "$STAGING" \
    -ov \
    -format UDRW \
    -fs HFS+ \
    "$RW_DMG"
if [ $? -ne 0 ]; then
    echo "ERROR: hdiutil failed to create writable DMG."
    rm -rf "$STAGING"
    exit 1
fi

rm -rf "$MOUNT_POINT"
mkdir -p "$MOUNT_POINT"
hdiutil attach "$RW_DMG" -mountpoint "$MOUNT_POINT" -nobrowse -noverify
if [ $? -ne 0 ]; then
    echo "ERROR: failed to mount writable DMG."
    rm -f "$RW_DMG"
    rm -rf "$STAGING"
    exit 1
fi

echo "-- Re-signing dylibs inside DMG..."
find "$MOUNT_POINT/$BUNDLE/Contents/Frameworks" -name "*.dylib" | while read lib; do
    codesign --force --sign "$IDENTITY" --timestamp --options=runtime "$lib"
done
find "$MOUNT_POINT/$BUNDLE/Contents/Frameworks" -maxdepth 1 -name "*.framework" | while read fw; do
    codesign --force --sign "$IDENTITY" --timestamp --options=runtime "$fw"
done
find "$MOUNT_POINT/$BUNDLE/Contents/PlugIns" \( -name "*.dylib" -o -name "*.so" \) | while read lib; do
    codesign --force --sign "$IDENTITY" --timestamp --options=runtime "$lib"
done
codesign --force --sign "$IDENTITY" --timestamp --options=runtime \
    "$MOUNT_POINT/$BUNDLE/Contents/MacOS/$APPNAME"
codesign --force --sign "$IDENTITY" --timestamp --options=runtime \
    "$MOUNT_POINT/$BUNDLE"

echo "Verifying bundle signature inside DMG..."
codesign --verify --deep --strict --verbose=2 "$MOUNT_POINT/$BUNDLE"
if [ $? -ne 0 ]; then
    echo "ERROR: bundle signature invalid inside DMG, aborting."
    hdiutil detach "$MOUNT_POINT"
    rm -f "$RW_DMG"
    rm -rf "$STAGING" "$MOUNT_POINT"
    exit 1
fi
echo "Bundle signature inside DMG OK."

hdiutil detach "$MOUNT_POINT"

### Convert UDRW to final compressed UDZO ###########################
echo
echo "______________________________________________________________"
echo "Convert to final compressed DMG (UDZO):"
mkdir -p "build-aux/mac-osx"
rm -f "$DMG_PATH"
hdiutil convert "$RW_DMG" \
    -format UDZO \
    -o "$DMG_PATH"
if [ $? -ne 0 ]; then
    echo "ERROR: hdiutil convert failed."
    rm -f "$RW_DMG"
    rm -rf "$STAGING" "$MOUNT_POINT"
    exit 1
fi
rm -f "$RW_DMG"
rm -rf "$STAGING" "$MOUNT_POINT"

### Sign the final DMG ##############################################
echo "Signing final DMG..."
codesign --sign "$IDENTITY" --timestamp "$DMG_PATH"

### Notarize and staple the final DMG ###############################
echo -e "\033[1;31mWould you like to notarize the DMG \"${DMG_NAME}\", n/Y?\033[m"
read a
if [[ $a == "Y" || $a == "y" ]]; then
    echo
    echo "______________________________________________________________"
    echo "Notarizing DMG:"
    xcrun notarytool submit "$DMG_PATH" --keychain-profile "org.qelectrotech" --wait
    if [ $? -ne 0 ]; then
        echo "ERROR: DMG notarization failed. Check the log with:"
        echo " xcrun notarytool log <submission-id> --keychain-profile org.qelectrotech"
        exit 1
    fi
    echo "Stapling DMG..."
    xcrun stapler staple "$DMG_PATH"
    if [ $? -ne 0 ]; then
        echo "ERROR: stapling DMG failed."
        exit 1
    fi
    echo "DMG notarized and stapled OK."
    spctl -a -vv "$DMG_PATH"
else
    echo -e "\033[1;33mExit.\033[m"
fi

### Clean up bundle #################################################
rm -rf "$BUNDLE"

echo
echo "______________________________________________________________"
echo "The process is done."
echo "DMG is in the folder 'build-aux/mac-osx'."

### Upload via rsync ################################################
echo -e "\033[1;31mWould you like to upload MacOS package \"${DMG_NAME}\", n/Y?\033[m"
read a
if [[ $a == "Y" || $a == "y" ]]; then
    cp -Rf "$DMG_PATH" /Users/laurent/MAC_OS_X/
    rsync -e ssh -av --delete-after --no-owner --no-g --chmod=g+w \
        --progress --exclude='.DS_Store' \
        /Users/laurent/MAC_OS_X/ \
        server:download.qelectrotech.org/qet/builds/MAC_OS_X/arm64/
    if [ $? != 0 ]; then
        echo "RSYNC ERROR: problem syncing ${DMG_NAME}, retrying..."
        rsync -e ssh -av --delete-after --no-owner --no-g --chmod=g+w \
            --progress --exclude='.DS_Store' \
            /Users/laurent/MAC_OS_X/ \
            server:download.qelectrotech.org/qet/builds/MAC_OS_X/arm64/
    fi
else
    echo -e "\033[1;33mExit.\033[m"
fi
