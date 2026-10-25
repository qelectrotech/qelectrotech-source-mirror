#!/bin/bash

# Bundle Qt6 and the libraries QElectroTech needs into the AppDir,
# then create the Linux AppImage bundle file.
#
# Adapted from the digiKam AppImage scripts by Gilles Caulier.
# Copyright (c) 2015-2019, Gilles Caulier, <caulier dot gilles at gmail dot com>
# Copyright (c) 2026, The QElectroTech Team
#
# Redistribution and use is allowed according to the terms of the BSD license.

# Halt and catch errors
set -eE
trap 'PREVIOUS_COMMAND=$THIS_COMMAND; THIS_COMMAND=$BASH_COMMAND' DEBUG
trap 'echo "FAILED COMMAND: $PREVIOUS_COMMAND"' ERR

#################################################################################################
# Manage script traces to log file

mkdir -p ./logs
exec > >(tee ./logs/build-appimage.full.log) 2>&1

#################################################################################################

echo "03-build-appimage.sh : build the QElectroTech AppImage bundle."
echo "---------------------------------------------------------------"

#################################################################################################
# Pre-processing checks

. ./config.sh
. ./common.sh
ChecksNotRoot
StartScript
ChecksCPUCores
HostAdjustments

ORIG_WD="`pwd`"
QET_RELEASEID=`cat $ORIG_WD/data/RELEASEID.txt`

if [[ ! -x "$APP_IMG_DIR/usr/bin/qelectrotech" ]] ; then
    echo "QElectroTech is not installed in $APP_IMG_DIR : run 02-build-qet.sh first."
    exit 1
fi

#################################################################################################

echo -e "---------- Prepare directories in bundle\n"

cd "$APP_IMG_DIR"

mkdir -p usr/lib usr/plugins usr/translations usr/share/metainfo

#################################################################################################

echo -e "---------- Copy Qt plugins in bundle\n"

# Only the plugins QET uses. Whole directories where every plugin may be
# needed depending on the desktop (platform, input methods, GL, Wayland).
QT_PLUGIN_DIRS="\
platforms \
platformthemes \
platforminputcontexts \
xcbglintegrations \
wayland-decoration-client \
wayland-graphics-integration-client \
wayland-shell-integration \
imageformats \
iconengines \
printsupport \
tls \
networkinformation \
"

for DIR in $QT_PLUGIN_DIRS ; do
    if [[ -d "$QT_PLUGINDIR/$DIR" ]] ; then
        cp -r "$QT_PLUGINDIR/$DIR" usr/plugins/
    fi
done

# SQLite driver only (projectDataBase)
mkdir -p usr/plugins/sqldrivers
cp "$QT_PLUGINDIR/sqldrivers/libqsqlite.so" usr/plugins/sqldrivers/

# Platforms not useful for a desktop application
rm -f usr/plugins/platforms/libqeglfs.so \
      usr/plugins/platforms/libqlinuxfb.so \
      usr/plugins/platforms/libqminimalegl.so \
      usr/plugins/platforms/libqvkkhrdisplay.so \
      usr/plugins/platforms/libqvnc.so

#################################################################################################

echo -e "---------- Copy Qt translations in bundle\n"

# QET loads qt_<lang>.qm, a meta catalog that pulls qtbase_<lang>.qm.
cp "$QT_TRANSDIR"/qt_*.qm     usr/translations/ 2>/dev/null || true
cp "$QT_TRANSDIR"/qtbase_*.qm usr/translations/ 2>/dev/null || true
rm -f usr/translations/qt_help_*.qm

cp "$ORIG_WD/data/qt.conf" usr/bin/

#################################################################################################

echo -e "---------- Scan dependencies recursively\n"

# Scan every ELF file until no new library is copied.
PASS=0

while true ; do

    PASS=$((PASS+1))
    BEFORE=$(ls usr/lib | wc -l)

    for FILE in $(find usr/bin usr/lib usr/plugins -type f) ; do
        if IsElf "$FILE" ; then
            CopyReccursiveDependencies "$FILE" "$APP_IMG_DIR/usr/lib"
        fi
    done

    AFTER=$(ls usr/lib | wc -l)
    echo "Pass $PASS : $AFTER libraries"

    [[ $BEFORE -eq $AFTER ]] && break

done

#################################################################################################

echo -e "---------- Clean-up Bundle Directory Contents\n"

# Libraries assumed to be part of the base system, from the AppImage
# community excludelist (glibc, libstdc++, libGL/libEGL, X11, fontconfig...).
# Bundling libGL/libEGL breaks the host graphics drivers (NVIDIA, Mesa).
EXCLUDE_FILES=$(grep -v "^#" "$DOWNLOAD_DIR/excludelist" | grep -v "^$" | awk '{print $1}')

for FILE in $EXCLUDE_FILES ; do
    if [[ -f usr/lib/${FILE} ]] ; then
        echo -e "   ==> ${FILE} will be removed from the bundle"
        rm -f usr/lib/${FILE}
    fi
done

# Developer files installed by the KF modules built by QET's CMake
rm -rf usr/include         || true
rm -rf usr/lib/cmake       || true
rm -rf usr/lib/pkgconfig   || true
rm -rf usr/lib/*.a         || true
rm -rf usr/lib/x86_64-linux-gnu || true
rm -rf usr/share/ECM       || true
rm -rf usr/mkspecs         || true
rm -rf usr/share/qlogging-categories6 || true

#################################################################################################

echo -e "---------- Set RPATH \n"

# Libraries are found through RPATH, not LD_LIBRARY_PATH: programs started
# from QET (xdg-open, browser, PDF viewer) must not inherit our libraries.
patchelf --set-rpath '$ORIGIN/../lib' usr/bin/qelectrotech

for FILE in $(find usr/lib -maxdepth 1 -type f -name "*.so*") ; do
    patchelf --set-rpath '$ORIGIN' "$FILE"
done

for FILE in $(find usr/plugins -type f -name "*.so") ; do
    patchelf --set-rpath '$ORIGIN/../../lib' "$FILE"
done

#################################################################################################
# See LFS instruction: http://www.linuxfromscratch.org/lfs/view/systemd/chapter05/stripping.html

echo -e "---------- Strip Binaries Files \n"

for FILE in $(find usr/bin usr/lib usr/plugins -type f) ; do
    if IsElf "$FILE" ; then
        if [[ $QET_DEBUG = 1 ]] && [[ "$(basename $FILE)" = "qelectrotech" ]] ; then
            continue
        fi
        strip --strip-debug "$FILE" || true
    fi
done

#################################################################################################

echo -e "---------- Desktop integration files \n"

cp "$ORIG_WD/data/AppRun" ./
chmod a+x AppRun

# appimagetool wants the .desktop file and its icon at the AppDir root.
cp usr/share/applications/org.qelectrotech.qelectrotech.desktop ./
cp usr/share/icons/hicolor/256x256/apps/qelectrotech.png ./
ln -sf qelectrotech.png .DirIcon

# AppStream metadata, named after the desktop file id.
if [[ -f usr/share/appdata/qelectrotech.appdata.xml ]] ; then
    mv usr/share/appdata/qelectrotech.appdata.xml \
       usr/share/metainfo/org.qelectrotech.qelectrotech.appdata.xml
    rmdir usr/share/appdata 2>/dev/null || true
fi

desktop-file-validate org.qelectrotech.qelectrotech.desktop || true

#################################################################################################

echo -e "---------- Create Bundle with appimagetool\n"

APP=QElectroTech
APPIMAGE="${APP}_${QET_RELEASEID}-${ARCH}.AppImage"

mkdir -p "$ORIG_WD/bundle"
rm -f "$ORIG_WD/bundle/"* || true

# --appimage-extract-and-run : appimagetool itself runs without FUSE.
# --no-appstream : the appdata id (org.qelectrotech.QElectroTech) does not
#                  match the desktop file id, appstreamcli would refuse it.
ARCH=$ARCH "$DOWNLOAD_DIR/appimagetool-x86_64.AppImage" --appimage-extract-and-run \
     --no-appstream \
     "$APP_IMG_DIR" "$ORIG_WD/bundle/$APPIMAGE"

chmod a+x "$ORIG_WD/bundle/$APPIMAGE"

#################################################################################################

echo -e "---------- Compute package checksums for QElectroTech $QET_RELEASEID\n"

cd "$ORIG_WD/bundle"

sha256sum "$APPIMAGE" > "$APPIMAGE-SHA256.txt"

if [[ $QET_SIGN = 1 ]] ; then
    gpg --batch --yes --detach-sign --armor -o "$APPIMAGE.sig" "$APPIMAGE"
    sha256sum "$APPIMAGE.sig" >> "$APPIMAGE-SHA256.txt"
fi

echo    "File       : $APPIMAGE"
echo -n "Size       : " ; du -h "$APPIMAGE" | awk '{print $1}'
echo -n "SHA256 sum : " ; awk '{print $1}' "$APPIMAGE-SHA256.txt" | head -n 1

if [[ $QET_UPLOAD = 1 ]] ; then

    echo -e "---------- Upload the AppImage to $QET_UPLOADURL:$QET_UPLOADDIR\n"

    rsync -e ssh -av --no-owner --no-g --chmod=g+w --progress \
          "$ORIG_WD/bundle/" "$QET_UPLOADURL:$QET_UPLOADDIR"

fi

#################################################################################################

TerminateScript
