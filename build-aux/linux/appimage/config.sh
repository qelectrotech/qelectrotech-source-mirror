#!/bin/bash

# Configuration for the scripts that build the QElectroTech AppImage bundle.
#
# Adapted from the digiKam AppImage scripts by Gilles Caulier.
# Copyright (c) 2015-2019, Gilles Caulier, <caulier dot gilles at gmail dot com>
# Copyright (c) 2026, The QElectroTech Team
#
# Redistribution and use is allowed according to the terms of the BSD license.

########################################################################
# Working directories (nothing is installed in /usr, no root needed
# except for apt in 01-build-host.sh)

WORK_DIR="$HOME/qet-appimage-work"

# Tarballs and tools (appimagetool, excludelist)
DOWNLOAD_DIR="$WORK_DIR/download"

# QET source tree and build tree
BUILDING_DIR="$WORK_DIR/build"

# AppDir assembled before calling appimagetool
APP_IMG_DIR="$WORK_DIR/qelectrotech.appdir"

########################################################################
# Qt6 to bundle
#
#   aqt    : official Qt binaries installed with aqtinstall (recommended).
#            Ubuntu 24.04 only ships Qt 6.4.2.
#   system : Ubuntu 24.04 packages (Qt 6.4.2). Builds without KF6, since
#            KF 6.10 needs a newer Qt. QET master no longer compiles with
#            Qt 6.4 (qFuzzyCompare(QPointF) in sources/cable/cablemanager.cpp):
#            only for older QET branches.

QT_FROM=aqt
QT_VERSION=6.11.2
QT_ARCH=linux_gcc_64
QT_ROOT="$WORK_DIR/Qt"

# Extra aqt modules: qtpdf (PDF page import), qtimageformats (more image plugins)
QT_AQT_MODULES="qtpdf qtimageformats"

########################################################################
# QElectroTech source

QET_GITURL="https://github.com/qelectrotech/qelectrotech-source-mirror.git"

# Branch or tag to package, "master" for the current development code.
QET_VERSION=master

# Build an existing QET checkout instead of cloning QET_GITURL in
# $BUILDING_DIR (used by the CI, .github/workflows/linux-appimage.yml).
# The checkout is built as it is: no reset, no pull.
QET_SOURCE_DIR="${QET_SOURCE_DIR:-}"

# Build with KDE Frameworks: ECM, KCoreAddons and KWidgetsAddons are built
# statically against the bundled Qt by 02-build-qet.sh, in $KF_PREFIX.
# 0 = use QET's sources/ui/nokde replacements.
# Forced to 0 when QT_FROM=system.
QET_WITH_KF=1
KF_VERSION=v6.10.0
KF_GITBASE="https://invent.kde.org/frameworks"
KF_PREFIX="$WORK_DIR/kf6"

# 3D mouse support through hidapi (libhidapi-dev). 0 = disabled.
QET_SPACEMOUSE=0

# Keep debug symbols in the qelectrotech binary (bigger AppImage,
# readable backtraces with the "debug" AppRun option).
QET_DEBUG=0

########################################################################
# Publication

# Sign the AppImage with GPG (detached .sig, uses your gpg-agent).
QET_SIGN=0

# Upload with rsync over ssh. "server" is a Host alias in ~/.ssh/config.
QET_UPLOAD=0
QET_UPLOADURL="server"
QET_UPLOADDIR="download.qelectrotech.org/qet/builds/AppImage/0.200.0/X86_64/"
