#!/bin/bash

# Configuration for the scripts that build the QElectroTech macOS bundle
# (signed and notarized DMG) with Homebrew, Qt6 and KF6.
#
# Adapted from the digiKam Homebrew scripts by Gilles Caulier.
# Copyright (c) 2013-2026, Gilles Caulier, <caulier dot gilles at gmail dot com>
# Copyright (c) 2026, The QElectroTech Team
#
# Redistribution and use is allowed according to the terms of the BSD license.

########################################################################
# Working directories. Nothing is installed in the Homebrew prefix except
# the Homebrew packages themselves: KDE Frameworks and QET are built and
# assembled in $WORK_DIR, and no root access is needed.

WORK_DIR="$HOME/qet-homebrew-work"

# QET source tree (when not using QET_SOURCE_DIR) and KF6 build trees
BUILDING_DIR="$WORK_DIR/build"

# Where the .app is assembled and signed before the DMG is created
BUNDLE_DIR="$WORK_DIR/bundle"

########################################################################
# Target

# Minimal macOS version, written in the binaries and in Info.plist.
# It must not be lower than the minimum of the Qt used (the "minos" of
# QtCore, see 02-build-qet.sh), or macOS refuses to start QET with the
# -10825 error. With QT_FROM=brew, check the bottles: Homebrew only builds
# them for its Tier 1 macOS versions.
OSX_MIN_TARGET="14.0"

########################################################################
# Qt6 to bundle
#
#   aqt  : official Qt binaries installed with aqtinstall (default).
#          They are universal (arm64 + x86_64) and target older macOS
#          versions than the Homebrew bottles: one DMG for every Mac.
#          sqlite and hidapi are then built universal by 02-build-qet.sh,
#          and Homebrew only provides the build tools.
#   brew : Homebrew Qt6 (BREW_QT below). arm64 only: Homebrew no longer
#          builds Intel bottles (Tier 3 since Sept. 2026).

QT_FROM=aqt
QT_VERSION=6.11.2
QT_ROOT="$WORK_DIR/Qt"

# Extra aqt modules: qtpdf (PDF page import), qtimageformats (more image
# plugins). qtsvg, qttools and qttranslations are in the base install.
QT_AQT_MODULES="qtpdf qtimageformats"

# Architectures of the bundle, with QT_FROM=aqt. Forced to arm64 with
# QT_FROM=brew. "arm64;x86_64" gives a universal DMG.
OSX_ARCHS="arm64;x86_64"

# sqlite (with the session extension used by the .qetdb history) and
# hidapi (3D mouse), built universal and static in $DEPS_PREFIX with
# QT_FROM=aqt. Homebrew's are arm64 only.
DEPS_PREFIX="$WORK_DIR/deps"
SQLITE_VERSION=3.54.1
SQLITE_GITURL="https://github.com/sqlite/sqlite.git"
HIDAPI_VERSION=0.15.0
HIDAPI_GITURL="https://github.com/libusb/hidapi.git"

########################################################################
# Homebrew packages
#
# Only what QET needs, instead of the ~40 formulae digiKam installs
# (boost, opencv, ffmpeg, exiv2, mariadb...). Qt is installed as split
# modules, not the "qt" meta formula that pulls qtwebengine,
# qtmultimedia, qt3d... If "qt" is already installed, nothing breaks:
# these modules are among its dependencies.
#
# BREW_TOOLS are always installed. BREW_LIBS, BREW_QT and
# BREW_QT_OPTIONAL only with QT_FROM=brew.

BREW_TOOLS="cmake ninja git ccache"
BREW_LIBS="sqlite hidapi pkgconf"
BREW_QT="qtbase qtsvg qttools qttranslations qtimageformats"

# Optional formula for the PDF page import (toolbar "Ajouter un PDF").
# Installed only if Homebrew has a formula with this name; QET builds
# without QtPdf otherwise (feature disabled, see CMakeLists.txt).
BREW_QT_OPTIONAL="qtpdf"

# Do not update Homebrew before installing (keeps the build environment
# stable between two builds). 1 = run "brew update" first.
BREW_UPDATE=0

########################################################################
# QElectroTech source

QET_GITURL="https://github.com/qelectrotech/qelectrotech-source-mirror.git"

# Branch or tag to package, "master" for the current development code.
QET_VERSION=master

# Build an existing QET checkout instead of cloning QET_GITURL in
# $BUILDING_DIR. The checkout is built as it is: no reset, no pull.
# Example: QET_SOURCE_DIR=~/qelectrotech-source-mirror ./update.sh
QET_SOURCE_DIR="${QET_SOURCE_DIR:-}"

# KDE Frameworks: ECM, KCoreAddons and KWidgetsAddons only, built
# statically against the Homebrew Qt by 02-build-qet.sh in $KF_PREFIX
# (nothing to deploy or sign in the bundle). Same version as the one
# pinned in cmake/fetch_kdeaddons.cmake.
# 0 = use QET's sources/ui/nokde replacements.
QET_WITH_KF=1
KF_VERSION=v6.10.0
KF_GITBASE="https://invent.kde.org/frameworks"
KF_PREFIX="$WORK_DIR/kf6"

# 3D mouse support through hidapi. 0 = disabled.
QET_SPACEMOUSE=1

# Keep debug symbols (RelWithDebInfo). 0 = Release.
QET_DEBUG=0

# ccache for the QET build (the KF6 build is done once).
QET_CCACHE=1

########################################################################
# Signing, notarization and publication
#
# QET_SIGN=0 signs ad hoc ("-"): the DMG runs on this Mac (arm64 binaries
# must be signed) but Gatekeeper refuses it elsewhere. Notarization needs
# QET_SIGN=1.

#
# These three settings can be given in the environment, as the CI does
# (.github/workflows/macos-dmg.yml: no signature, no upload):
#   QET_SIGN=0 QET_UPLOAD=0 ./makeall.sh

QET_SIGN=${QET_SIGN:-1}
QET_IDENTITY="Developer ID Application: Laurent TRINQUES (Y73WZ6WZ5X)"

# notarytool profile created once with:
#   xcrun notarytool store-credentials "org.qelectrotech" --apple-id ... --team-id Y73WZ6WZ5X
QET_NOTARIZE=${QET_NOTARIZE:-1}
QET_NOTARY_PROFILE="org.qelectrotech"

# Upload with rsync over ssh. "server" is a Host alias in ~/.ssh/config.
# The DMG is first copied to QET_UPLOAD_LOCALDIR, which is mirrored to the
# server (--delete-after), as misc/MacQetDeploy_arm64_cmake.sh did.
QET_UPLOAD=${QET_UPLOAD:-0}
QET_UPLOADURL="server"
QET_UPLOADDIR="download.qelectrotech.org/qet/builds/MAC_OS_X/arm64/"
QET_UPLOAD_LOCALDIR="$HOME/MAC_OS_X"
