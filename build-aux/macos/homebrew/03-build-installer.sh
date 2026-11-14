#!/bin/bash

# Deploy Qt into the QElectroTech .app built by 02-build-qet.sh, add the
# QET data (elements, titleblocks, translations...), sign, notarize and
# create the DMG.
#
# The deployment, signing and DMG steps are the ones of
# misc/MacQetDeploy_arm64_cmake.sh (proven, do not change them without a
# reason), made non-interactive: the questions of that script are the
# QET_SIGN / QET_NOTARIZE / QET_UPLOAD settings of config.sh here.
# It replaces digiKam's 04-build-installer.sh: no Packages .pkg installer,
# no package_lib.py, QET ships a drag-and-drop DMG.
#
# Adapted from the digiKam Homebrew scripts by Gilles Caulier.
# Copyright (c) 2015-2026, Gilles Caulier, <caulier dot gilles at gmail dot com>
# Copyright (c) 2026, The QElectroTech Team
#
# Redistribution and use is allowed according to the terms of the BSD license.

# No "set -e" here: the steps below come from MacQetDeploy_arm64_cmake.sh,
# which checks every important command itself, and some of its pipelines
# (grep without match in a "while read" loop) return non-zero on purpose.
trap 'PREVIOUS_COMMAND=$THIS_COMMAND; THIS_COMMAND=$BASH_COMMAND' DEBUG

#################################################################################################
# Manage script traces to log file

mkdir -p ./logs
exec > >(tee ./logs/build-installer.full.log) 2>&1

#################################################################################################

echo "03-build-installer.sh : build the QElectroTech DMG."
echo "---------------------------------------------------"

#################################################################################################
# Pre-processing checks

. ./config.sh
. ./common.sh
ChecksNotRoot
StartScript
ChecksXCodeCLI
ChecksCPUCores
ChecksHomebrew
HostAdjustments

ORIG_WD="`pwd`"

for f in VERSION REVISION SOURCEDIR BUILDDIR ; do
    if [[ ! -f "$WORK_DIR/$f.txt" ]] ; then
        echo "$WORK_DIR/$f.txt not found : run 02-build-qet.sh first."
        exit 1
    fi
done

VERSION=$(cat "$WORK_DIR/VERSION.txt")
HEAD=$(cat "$WORK_DIR/REVISION.txt")
BUILD_DIR=$(cat "$WORK_DIR/BUILDDIR.txt")

# Variable names of MacQetDeploy_arm64_cmake.sh
current_dir=$(cat "$WORK_DIR/SOURCEDIR.txt")
APPNAME='qelectrotech'
BUNDLE=$APPNAME.app
DMG_NAME="${APPNAME}-$VERSION-r$HEAD-$PKG_ARCH.dmg"
DMG_PATH="$ORIG_WD/bundle/$DMG_NAME"

RW_DMG="/tmp/qet_rw.dmg"
MOUNT_POINT="/tmp/qet_dmg_mount"
STAGING="/tmp/qet_dmg_staging"

if [[ ! -x "$BUILD_DIR/$BUNDLE/Contents/MacOS/$APPNAME" ]] ; then
    echo "$BUILD_DIR/$BUNDLE not found : run 02-build-qet.sh first."
    exit 1
fi

if [[ ! -x "$QT_PREFIX/bin/macdeployqt" ]] ; then
    echo "ERROR: macdeployqt not found in $QT_PREFIX/bin : run 01-build-homebrew.sh first."
    exit 1
fi

#################################################################################################
# Signing identity

if [[ $QET_SIGN = 1 ]] ; then

    IDENTITY="$QET_IDENTITY"

    if ! security find-identity -v -p codesigning | grep -q "$IDENTITY" ; then
        echo "ERROR: signing identity \"$IDENTITY\" not found in the keychain."
        echo "       Set QET_SIGN=0 in config.sh for a local, ad hoc signed build."
        exit 1
    fi

    SIGN_OPTS="--timestamp --options=runtime"
    DMG_SIGN_OPTS="--timestamp"

else

    # Ad hoc signature: arm64 binaries must be signed to run, but nothing
    # can be notarized without a Developer ID.
    IDENTITY="-"
    SIGN_OPTS=""
    DMG_SIGN_OPTS=""
    QET_NOTARIZE=0
    echo "QET_SIGN=0 : ad hoc signature, no notarization."

fi

echo "Release ID      : $VERSION-r$HEAD ($PKG_ARCH)"
echo "Signing identity: $IDENTITY"

#################################################################################################
# hdiutil sometimes fails with "Resource busy" on CI runners (and now and
# then on a desktop, when Spotlight or an antivirus scans the image): try
# again a few times before giving up.

HdiutilRetry()
{

for _try in 1 2 3 4 5 ; do
    hdiutil "$@" && return 0
    echo "hdiutil $1 failed (try $_try), retrying in 10 s..."
    sleep 10
done

return 1

}

#################################################################################################
# Fresh copy of the built bundle

mkdir -p "$BUNDLE_DIR" "$ORIG_WD/bundle"
cd "$BUNDLE_DIR" || exit 1

rm -rf "$BUNDLE"
cp -R "$BUILD_DIR/$BUNDLE" "./$BUNDLE"

#################################################################################################
# From here: misc/MacQetDeploy_arm64_cmake.sh, "copy over frameworks" to the end.
#################################################################################################


### copy over frameworks ############################################
echo
echo "______________________________________________________________"
echo "Copy Qt libraries and private frameworks:"

if [ ! -d $BUNDLE ] ; then
    echo "ERROR: cannot find application bundle \"$BUNDLE\" in current directory"
    exit 1
fi

"$QT_PREFIX/bin/macdeployqt" $BUNDLE || { echo "ERROR: macdeployqt failed"; exit 1; }

### offscreen platform plugin (headless use, e.g. qet_mcp.py) ########
# macdeployqt only deploys libqcocoa. Without libqoffscreen, running
# QT_QPA_PLATFORM=offscreen aborts. macdeployqt has no option to add it,
# so copy it from the Homebrew Qt and make it point to the
# frameworks of the bundle. It is signed with the other plugins below.
OFFSCREEN="$BUNDLE/Contents/PlugIns/platforms/libqoffscreen.dylib"
if [ ! -f "$OFFSCREEN" ] ; then
    echo "libqoffscreen.dylib not deployed by macdeployqt, copying it:"
    SRC_OFFSCREEN=""
    for d in "$QT_PLUGINDIR" \
             "$(brew --prefix qtbase 2>/dev/null)/share/qt/plugins" \
             /opt/homebrew/share/qt/plugins ; do
        if [ -f "$d/platforms/libqoffscreen.dylib" ] ; then
            SRC_OFFSCREEN="$d/platforms/libqoffscreen.dylib"
            break
        fi
    done
    if [ -z "$SRC_OFFSCREEN" ] ; then
        echo "ERROR: libqoffscreen.dylib not found in the Qt plugins"
        exit 1
    fi
    mkdir -p "$BUNDLE/Contents/PlugIns/platforms"
    cp -L "$SRC_OFFSCREEN" "$OFFSCREEN"
    chmod u+w "$OFFSCREEN"
    # Qt frameworks: /opt/homebrew/.../QtGui.framework/Versions/A/QtGui
    # -> inside the bundle
    otool -L "$OFFSCREEN" | awk 'NR>1 { print $1 }' \
        | grep -E '/Qt[A-Za-z0-9]+\.framework/' | while read ref; do
        fwname=$(echo "$ref" | sed -E 's#.*/(Qt[A-Za-z0-9]+)\.framework/.*#\1#')
        install_name_tool -change "$ref" \
            "@executable_path/../Frameworks/$fwname.framework/Versions/A/$fwname" \
            "$OFFSCREEN"
        echo "  Fixed ref: $fwname"
    done
fi
if [ ! -f "$OFFSCREEN" ] ; then
    echo "ERROR: libqoffscreen.dylib missing from the bundle"
    exit 1
fi

### fix Homebrew dependencies macdeployqt could not handle ##########
# Recent Homebrew bottles (brotli, webp, sharpyuv...) reference their own
# dependencies as @rpath/libX.dylib. macdeployqt only resolves @rpath in
# Contents/lib and in Qt's lib dir (-libpath does not help): it prints
# "Cannot resolve rpath" and leaves some references untouched, either
# absolute /opt/homebrew paths or @rpath libs missing from the bundle.
# Fix both here: every /opt/homebrew reference is rewritten to
# @rpath/libX.dylib (install ids too), and every @rpath/libX.dylib is
# copied into Contents/Frameworks, where the executable's LC_RPATH
# (@executable_path/../Frameworks) lets dyld find it. Everything is signed
# below with the rest of Frameworks/.
echo
echo "______________________________________________________________"
echo "Fix Homebrew dependencies left by macdeployqt:"

FW="$BUNDLE/Contents/Frameworks"
chmod -R u+w "$BUNDLE/Contents/MacOS" "$FW" "$BUNDLE/Contents/PlugIns" 2>/dev/null

# Every Mach-O candidate of the bundle (otool silently ignores the others)
list_macho() {
    find "$BUNDLE/Contents/MacOS" "$FW" "$BUNDLE/Contents/PlugIns" -type f \
        \( -name '*.dylib' -o -perm -u+x \) 2>/dev/null
}

# Dependencies of one binary, without its own install id (dylibs only)
list_deps() {
    _id=$(otool -D "$1" 2>/dev/null | sed -n 2p)
    otool -L "$1" 2>/dev/null | awk 'NR>1 { print $1 }' | while read _dep; do
        [ "$_dep" = "$_id" ] || echo "$_dep"
    done
}

# The @rpath/libX.dylib names referenced anywhere in the bundle
list_rpath_libs() {
    list_macho | while read bin; do
        list_deps "$bin" | sed -n 's#^@rpath/\([^/]*\.dylib\)$#\1#p'
    done | LC_ALL=C sort -u
}

if ! otool -l "$BUNDLE/Contents/MacOS/$APPNAME" | grep -q "@executable_path/../Frameworks" ; then
    install_name_tool -add_rpath "@executable_path/../Frameworks" "$BUNDLE/Contents/MacOS/$APPNAME"
    echo "  Added LC_RPATH @executable_path/../Frameworks to $APPNAME"
fi

# 3 passes, since each copied library can bring its own dependencies:
#  a. rewrite absolute /opt/homebrew references (install ids included),
#     copying the referenced library into Frameworks/ if needed
#  b. copy the @rpath/libX.dylib still missing from Frameworks/
for PASS in 1 2 3; do
    list_macho | while read bin; do
        _id=$(otool -D "$bin" 2>/dev/null | sed -n 2p)
        case "$_id" in
            /opt/homebrew/*)
                install_name_tool -id "@rpath/$(basename "$_id")" "$bin" 2>/dev/null
                echo "  Fixed id  (pass $PASS): $(basename "$bin")"
                ;;
        esac
        list_deps "$bin" | grep '^/opt/homebrew/' | while read ref; do
            name=$(basename "$ref")
            if [ ! -e "$FW/$name" ]; then
                cp -L "$ref" "$FW/$name" && chmod u+w "$FW/$name"
                echo "  Copied    (pass $PASS): $name"
            fi
            install_name_tool -change "$ref" "@rpath/$name" "$bin" 2>/dev/null
            echo "  Fixed ref (pass $PASS): $(basename "$bin") -> @rpath/$name"
        done
    done
    list_rpath_libs | while read lib; do
        if [ ! -e "$FW/$lib" ] && [ -e "/opt/homebrew/lib/$lib" ]; then
            cp -L "/opt/homebrew/lib/$lib" "$FW/$lib" && chmod u+w "$FW/$lib"
            echo "  Copied    (pass $PASS): $lib"
        fi
    done
done

# 3. Checks
UNRESOLVED=$(list_rpath_libs | while read lib; do [ -e "$FW/$lib" ] || echo "$lib"; done)
if [ -n "$UNRESOLVED" ]; then
    echo "ERROR: @rpath libraries still missing from Frameworks/:" $UNRESOLVED
    exit 1
fi
HOMEBREW_REFS=$(list_macho | while read bin; do
    otool -L "$bin" 2>/dev/null | awk 'NR>1 { print $1 }' | grep '^/opt/homebrew/' \
        | sed "s#^#  $(basename "$bin") -> #"
done)
if [ -n "$HOMEBREW_REFS" ]; then
    echo "ERROR: bundle still references Homebrew paths:"
    echo "$HOMEBREW_REFS"
    exit 1
fi
echo "All dependencies resolved inside the bundle."

### check the architectures of the bundle ##########################
# Every Mach-O file must hold every architecture of OSX_ARCHS: a library
# taken from Homebrew by mistake (arm64 only) would make the x86_64 part
# of a universal DMG crash at startup on Intel Macs.
echo
echo "______________________________________________________________"
echo "Check architectures ($OSX_ARCHS_LIST):"
ARCH_ERRORS=$(list_macho | while read bin; do
    if lipo -archs "$bin" >/dev/null 2>&1 ; then
        ChecksArchs "$bin" || true
    fi
done)
if [ -n "$ARCH_ERRORS" ]; then
    echo "ERROR: some files of the bundle miss an architecture:"
    echo "$ARCH_ERRORS"
    exit 1
fi
echo "All binaries of the bundle hold: $OSX_ARCHS_LIST"

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

# The files of the built bundle can come out read-only (Homebrew installs
# its templates 444 and CMake keeps their permissions), and depending on
# the CMake/Qt versions Contents/Resources may not exist yet: make the
# bundle writable and create the folder before copying into it. Stop on
# any failed copy instead of signing an incomplete bundle.
chmod -R u+w "$BUNDLE"
mkdir -p "$BUNDLE/Contents/Resources"
cp -f "${current_dir}/misc/Info.plist" "$BUNDLE/Contents/Info.plist" \
    || { echo "ERROR: cannot install Info.plist"; exit 1; }
cp -f "${current_dir}"/ico/mac_icon/*.icns "$BUNDLE/Contents/Resources/" \
    || { echo "ERROR: cannot install the .icns icons"; exit 1; }
/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString $VERSION r$HEAD" "$BUNDLE/Contents/Info.plist"
/usr/libexec/PlistBuddy -c "Set :LSMinimumSystemVersion $OSX_MIN_TARGET" "$BUNDLE/Contents/Info.plist"

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

# elements/ is a git submodule: an empty folder means it is not checked out.
if [ -z "$(ls -A "${QET_ELMT_DIR}" 2>/dev/null)" ]; then
    echo "ERROR: ${QET_ELMT_DIR} is empty or missing."
    echo "       Run: git submodule update --init elements"
    exit 1
fi
cp -R "${QET_ELMT_DIR}" "$BUNDLE/Contents/Resources/elements" \
    || { echo "ERROR: cannot copy the elements collection"; exit 1; }
if [ -d "${QET_TBT_DIR}" ]; then
    cp -R "${QET_TBT_DIR}" "$BUNDLE/Contents/Resources/titleblocks" \
        || { echo "ERROR: cannot copy the titleblocks"; exit 1; }
fi
# Translations: since PR #751 the .qm files are no longer in git; lrelease
# generates them in $BUILD_DIR/lang/. Expected set = the .ts files listed in
# TS_FILES (cmake/qet_compilation_vars.cmake): a missing .qm stops the
# script, a .ts in lang/ but not in TS_FILES only gives a WARNING.
# Temporary files rather than <(...): kept as in MacQetDeploy_arm64_cmake.sh,
# whose /bin/sh (bash 3.2 in POSIX mode) has no process substitution.
QM_SRC="${BUILD_DIR}/lang"
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

# Translations of Qt itself (OK/Cancel buttons, standard dialogs...): they
# come from qtbase_XX.qm, not from QET's .ts files, and macdeployqt does not
# deploy them. QETApp::setLanguage() loads "qt_XX" from Qt's translations
# path (absent from the bundle), then from QET's lang/ folder: so each
# qtbase_XX.qm is put there as qt_XX.qm. qtbase_XX.qm is self-contained,
# unlike Qt's qt_XX.qm files, which depend on all the modules.
# First folder holding qtbase_*.qm files: the one given by qtpaths, then the
# separate Homebrew formula qttranslations, then the other possible Homebrew
# locations. Homebrew makes these folders symbolic links
# (-> Cellar/qttranslations/...): find must follow them (-L), otherwise it
# only sees the link itself and finds no file in it.
QT_TR_DIR=""
for d in "$("$QT_PREFIX/bin/qtpaths" --query QT_INSTALL_TRANSLATIONS 2>/dev/null)" \
         "$(brew --prefix qttranslations 2>/dev/null)/share/qt/translations" \
         "$QT_PREFIX/share/qt/translations" \
         /opt/homebrew/share/qt/translations \
         /opt/homebrew/opt/*/share/qt/translations ; do
    if ls "$d"/qtbase_*.qm >/dev/null 2>&1 ; then
        QT_TR_DIR="$d"
        break
    fi
done
LANG_DST="$BUNDLE/Contents/Resources/lang"
find -L "$QT_TR_DIR" -maxdepth 1 -name 'qtbase_*.qm' 2>/dev/null | while read f; do
    l=$(basename "$f" .qm | sed 's/^qtbase_//')
    cp "$f" "$LANG_DST/qt_$l.qm"
done
# QET languages with no exact Qt match (pt -> pt_PT, zh -> zh_CN...):
# QTranslator can only shorten a code (fr_FR -> fr), not lengthen it.
# Prefer the "main" variant (pt_PT), otherwise the first one found.
sed 's/^qet_//' "$QM_TMP/listed" | while read l; do
    if [ ! -e "$LANG_DST/qt_$l.qm" ]; then
        main="$QT_TR_DIR/qtbase_${l}_$(echo "$l" | tr 'a-z' 'A-Z').qm"
        if [ -e "$main" ]; then
            alt="$main"
        else
            alt=$(find -L "$QT_TR_DIR" -maxdepth 1 -name "qtbase_${l}_*.qm" 2>/dev/null | LC_ALL=C sort | head -1)
        fi
        [ -n "$alt" ] && cp "$alt" "$LANG_DST/qt_$l.qm"
    fi
done
QT_QM_COUNT=$(find "$LANG_DST" -maxdepth 1 -name 'qt_*.qm' | wc -l | tr -d ' ')
echo "${QT_QM_COUNT} Qt translation files (qt_*.qm) copied from ${QT_TR_DIR:-<not found>}"
if [ "${QT_QM_COUNT}" -eq 0 ]; then
    echo "ERROR: no qtbase_*.qm found (Qt translations not installed?)."
    echo "       Check with: find /opt/homebrew -name 'qtbase_fr.qm'"
    rm -rf "$QM_TMP"
    exit 1
fi
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

# The MCP server (misc/qet-mcp), as the Linux and Windows packages install it:
# from Contents/Resources/mcp it finds Contents/MacOS/qelectrotech and
# Contents/Resources/elements by itself.
mkdir -p $BUNDLE/Contents/Resources/mcp
cp "${current_dir}/misc/qet-mcp/qet_mcp.py" "${current_dir}/misc/qet-mcp/README.md" \
    $BUNDLE/Contents/Resources/mcp/ \
    || { echo "ERROR: cannot copy the MCP server"; exit 1; }

### Sign the bundle #################################################
# Sign in correct order: all dylibs first (including flat libs copied
# by macdeployqt from Homebrew), then frameworks, plugins, bundle last.
echo
echo "______________________________________________________________"
echo "Code signing (dylibs -> frameworks -> plugins -> bundle):"

echo "-- Signing dylibs in Frameworks/..."
find "$BUNDLE/Contents/Frameworks" -name "*.dylib" | while read lib; do
    codesign --force --sign "$IDENTITY" $SIGN_OPTS "$lib"
done

echo "-- Signing .framework bundles..."
find "$BUNDLE/Contents/Frameworks" -maxdepth 1 -name "*.framework" | while read fw; do
    codesign --force --sign "$IDENTITY" $SIGN_OPTS "$fw"
done

echo "-- Signing plugins..."
find "$BUNDLE/Contents/PlugIns" \( -name "*.dylib" -o -name "*.so" \) | while read lib; do
    codesign --force --sign "$IDENTITY" $SIGN_OPTS "$lib"
done

echo "-- Signing dylibs in MacOS/..."
find "$BUNDLE/Contents/MacOS" -name "*.dylib" | while read lib; do
    codesign --force --sign "$IDENTITY" $SIGN_OPTS "$lib"
done

echo "-- Signing main executable..."
codesign --force --sign "$IDENTITY" $SIGN_OPTS \
    --entitlements "${current_dir}/misc/qelectrotech.entitlements" \
    "$BUNDLE/Contents/MacOS/$APPNAME"

echo "-- Signing bundle..."
codesign --force --sign "$IDENTITY" $SIGN_OPTS \
    --entitlements "${current_dir}/misc/qelectrotech.entitlements" "$BUNDLE"

echo
echo "Verifying bundle signature..."
codesign --verify --deep --strict --verbose=2 "$BUNDLE"
if [ $? -ne 0 ]; then
    echo "ERROR: bundle signature verification failed, aborting."
    exit 1
fi
spctl -a -vv "$BUNDLE" || true
echo "Bundle signature OK."

### Notarize the .app (via temporary ZIP) ###########################
echo
echo "______________________________________________________________"
echo "Create temporary ZIP for notarization:"
NOTARIZE_ZIP="/tmp/${APPNAME}-$VERSION-r$HEAD-$PKG_ARCH-notarize.zip"
/usr/bin/ditto -c -k --keepParent "$BUNDLE" "$NOTARIZE_ZIP"

if [[ $QET_NOTARIZE = 1 ]]; then
    echo
    echo "______________________________________________________________"
    echo "Notarizing .app:"
    xcrun notarytool submit "$NOTARIZE_ZIP" --keychain-profile "$QET_NOTARY_PROFILE" --wait
    if [ $? -ne 0 ]; then
        echo "ERROR: notarization failed. Check the log with:"
        echo " xcrun notarytool log <submission-id> --keychain-profile $QET_NOTARY_PROFILE"
        rm -f "$NOTARIZE_ZIP"
        exit 1
    fi
else
    echo "Skipped (see config.sh)."
fi
rm -f "$NOTARIZE_ZIP"

### Staple the .app #################################################
if [[ $QET_NOTARIZE = 1 ]]; then
    xcrun stapler staple -v "$BUNDLE"
    if [ $? -ne 0 ]; then
        echo "ERROR: stapling .app failed."
        exit 1
    fi
    xcrun stapler validate -v "$BUNDLE"
    spctl -a -vv "$BUNDLE" || true
    echo ".app stapled OK."
else
    echo "Skipped (see config.sh)."
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
HdiutilRetry create \
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
HdiutilRetry attach "$RW_DMG" -mountpoint "$MOUNT_POINT" -nobrowse -noverify
if [ $? -ne 0 ]; then
    echo "ERROR: failed to mount writable DMG."
    rm -f "$RW_DMG"
    rm -rf "$STAGING"
    exit 1
fi

echo "-- Re-signing dylibs inside DMG..."
find "$MOUNT_POINT/$BUNDLE/Contents/Frameworks" -name "*.dylib" | while read lib; do
    codesign --force --sign "$IDENTITY" $SIGN_OPTS "$lib"
done
find "$MOUNT_POINT/$BUNDLE/Contents/Frameworks" -maxdepth 1 -name "*.framework" | while read fw; do
    codesign --force --sign "$IDENTITY" $SIGN_OPTS "$fw"
done
find "$MOUNT_POINT/$BUNDLE/Contents/PlugIns" \( -name "*.dylib" -o -name "*.so" \) | while read lib; do
    codesign --force --sign "$IDENTITY" $SIGN_OPTS "$lib"
done
codesign --force --sign "$IDENTITY" $SIGN_OPTS \
    --entitlements "${current_dir}/misc/qelectrotech.entitlements" \
    "$MOUNT_POINT/$BUNDLE/Contents/MacOS/$APPNAME"
codesign --force --sign "$IDENTITY" $SIGN_OPTS \
    --entitlements "${current_dir}/misc/qelectrotech.entitlements" \
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
mkdir -p "$(dirname "$DMG_PATH")"
rm -f "$DMG_PATH"
HdiutilRetry convert "$RW_DMG" \
    -format UDZO \
    -ov \
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
codesign --sign "$IDENTITY" $DMG_SIGN_OPTS "$DMG_PATH"

### Notarize and staple the final DMG ###############################
if [[ $QET_NOTARIZE = 1 ]]; then
    echo
    echo "______________________________________________________________"
    echo "Notarizing DMG:"
    xcrun notarytool submit "$DMG_PATH" --keychain-profile "$QET_NOTARY_PROFILE" --wait
    if [ $? -ne 0 ]; then
        echo "ERROR: DMG notarization failed. Check the log with:"
        echo " xcrun notarytool log <submission-id> --keychain-profile $QET_NOTARY_PROFILE"
        exit 1
    fi
    echo "Stapling DMG..."
    xcrun stapler staple "$DMG_PATH"
    if [ $? -ne 0 ]; then
        echo "ERROR: stapling DMG failed."
        exit 1
    fi
    echo "DMG notarized and stapled OK."
    spctl -a -vv -t open --context context:primary-signature "$DMG_PATH" || true
else
    echo "Skipped (see config.sh)."
fi

### Checksums ######################################################
echo
echo "______________________________________________________________"
echo "Compute package checksums for QElectroTech $VERSION-r$HEAD:"
( cd "$(dirname "$DMG_PATH")" && shasum -a 256 "$DMG_NAME" > "$DMG_NAME-SHA256.txt" )
echo    "File       : $DMG_NAME"
echo -n "Size       : " ; du -h "$DMG_PATH" | awk '{print $1}'
echo -n "SHA256 sum : " ; awk '{print $1}' "$DMG_PATH-SHA256.txt"

### Clean up bundle #################################################
rm -rf "$BUNDLE"

echo
echo "______________________________________________________________"
echo "The process is done."
echo "DMG is in the folder $(dirname "$DMG_PATH")."

### Upload via rsync ################################################
if [[ $QET_UPLOAD = 1 ]]; then
    mkdir -p "$QET_UPLOAD_LOCALDIR"
    cp -Rf "$DMG_PATH" "$DMG_PATH-SHA256.txt" "$QET_UPLOAD_LOCALDIR/"
    rsync -e ssh -av --delete-after --no-owner --no-g --chmod=g+w \
        --progress --exclude='.DS_Store' \
        "$QET_UPLOAD_LOCALDIR/" \
        "$QET_UPLOADURL:$QET_UPLOADDIR"
    if [ $? != 0 ]; then
        echo "RSYNC ERROR: problem syncing ${DMG_NAME}, retrying..."
        rsync -e ssh -av --delete-after --no-owner --no-g --chmod=g+w \
            --progress --exclude='.DS_Store' \
            "$QET_UPLOAD_LOCALDIR/" \
            "$QET_UPLOADURL:$QET_UPLOADDIR"
    fi
else
    echo "Skipped (see config.sh)."
fi

#################################################################################################

TerminateScript
