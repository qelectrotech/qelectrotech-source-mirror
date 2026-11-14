#!/bin/bash

# Build the KDE Frameworks QET uses (once), then QElectroTech.
#
# Adapted from the digiKam Homebrew scripts by Gilles Caulier
# (02-build-extralibs.sh and 03-build-digikam.sh).
# Copyright (c) 2015-2026, Gilles Caulier, <caulier dot gilles at gmail dot com>
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
exec > >(tee ./logs/build-qet.full.log) 2>&1

#################################################################################################

echo "02-build-qet.sh : build KDE Frameworks and QElectroTech."
echo "--------------------------------------------------------"

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

if [[ ! -x "$QMAKE" ]] ; then
    echo "ERROR: Homebrew Qt6 not found : run 01-build-homebrew.sh first."
    exit 1
fi

SQLITE_PREFIX=$("$BREW" --prefix sqlite 2>/dev/null || true)

# The sqlite3.h of the macOS SDK has no session extension (sqlite3session_*,
# sqlite3changeset_*) used by QetDbFile for the .qetdb history. The sqlite
# formula is keg-only: it must be given to CMake explicitly.
if [[ ! -f "$SQLITE_PREFIX/include/sqlite3.h" ]] ; then
    echo "ERROR: Homebrew SQLite not found : run 01-build-homebrew.sh first."
    exit 1
fi

ORIG_WD="`pwd`"

COMMON_CMAKE_OPTIONS="\
-DCMAKE_OSX_DEPLOYMENT_TARGET=$OSX_MIN_TARGET \
-DCMAKE_OSX_ARCHITECTURES=$ARCH_TARGET \
"

#################################################################################################
# KDE Frameworks : only ECM, KCoreAddons and KWidgetsAddons, instead of the
# 25 modules of digiKam's 02-build-extralibs.sh. Built statically in their
# own prefix, outside Homebrew: nothing to deploy, fix or sign, and an old
# KF6 installed in /opt/homebrew by the digiKam scripts cannot interfere
# ($KF_PREFIX comes first in CMAKE_PREFIX_PATH).

if [[ $QET_WITH_KF = 1 ]] ; then

    if [[ ! -f "$KF_PREFIX/.built-$KF_VERSION-$OSX_MIN_TARGET" ]] ; then

        echo -e "\n---------- Build KDE Frameworks $KF_VERSION in $KF_PREFIX\n"

        rm -rf "$KF_PREFIX" "$BUILDING_DIR/kf6"
        mkdir -p "$BUILDING_DIR/kf6"

        for MODULE in extra-cmake-modules kcoreaddons kwidgetsaddons ; do

            git clone --depth 1 --branch "$KF_VERSION" \
                "$KF_GITBASE/$MODULE.git" "$BUILDING_DIR/kf6/$MODULE"

            cmake -S "$BUILDING_DIR/kf6/$MODULE" -B "$BUILDING_DIR/kf6/$MODULE/build" -G Ninja \
                  -DCMAKE_BUILD_TYPE=Release \
                  -DCMAKE_INSTALL_PREFIX="$KF_PREFIX" \
                  -DCMAKE_PREFIX_PATH="$KF_PREFIX;$BREW_PREFIX" \
                  $COMMON_CMAKE_OPTIONS \
                  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
                  -DBUILD_SHARED_LIBS=OFF \
                  -DBUILD_TESTING=OFF \
                  -DBUILD_QCH=OFF \
                  -DBUILD_DOC=OFF \
                  -DBUILD_DESIGNERPLUGIN=OFF \
                  -DBUILD_PYTHON_BINDINGS=OFF \
                  -DKCOREADDONS_USE_QML=OFF \
                  -DKDE_SKIP_TEST_SETTINGS=ON \
                  -DQT_MAJOR_VERSION=6 \
                  -Wno-dev

            cmake --build "$BUILDING_DIR/kf6/$MODULE/build" -j$CPU_CORES
            cmake --install "$BUILDING_DIR/kf6/$MODULE/build"

        done

        touch "$KF_PREFIX/.built-$KF_VERSION-$OSX_MIN_TARGET"

    else

        echo "KDE Frameworks $KF_VERSION already built in $KF_PREFIX, skipped."

    fi

    KF_OPTIONS="-DBUILD_WITH_KF=ON -DBUILD_KF=OFF"

else

    KF_OPTIONS="-DBUILD_WITH_KF=OFF"

fi

#################################################################################################
# Checkout QElectroTech

if [[ -n "$QET_SOURCE_DIR" ]] ; then

    QET_SRC="$(cd "$QET_SOURCE_DIR" && pwd -P)"
    echo "---------- Using the QElectroTech checkout $QET_SRC"
    cd "$QET_SRC"

else

    QET_SRC="$BUILDING_DIR/qelectrotech-$QET_VERSION"

    if [[ -d "$QET_SRC/.git" ]] ; then

        echo "---------- Updating existing $QET_SRC"

        cd "$QET_SRC"
        git reset --hard
        git fetch origin
        git checkout "$QET_VERSION"
        git pull --ff-only || true

    else

        echo "---------- Downloading QElectroTech $QET_VERSION"

        mkdir -p "$BUILDING_DIR"
        git clone --progress "$QET_GITURL" "$QET_SRC"
        cd "$QET_SRC"
        git checkout "$QET_VERSION"

    fi

fi

git submodule update --init --recursive

#################################################################################################
# Release ID : major.minor.micro from sources/qetversion.cpp + revision, as in
# misc/MacQetDeploy_arm64_cmake.sh

TAGNAME=$(grep "return QVersionNumber{" sources/qetversion.cpp | head -n 1 \
          | awk -F "{" '{ print $2 }' | awk -F "}" '{ print $1 }' | sed -e 's/,/./g' -e 's/ //g')
REVISION=$(( $(git rev-list HEAD --count) + 473 ))
GITCOMMIT=$(git rev-parse --short HEAD)

mkdir -p "$WORK_DIR"
echo "$TAGNAME"   > "$WORK_DIR/VERSION.txt"
echo "$REVISION"  > "$WORK_DIR/REVISION.txt"
echo "$GITCOMMIT" > "$WORK_DIR/GITCOMMIT.txt"
echo "$QET_SRC"   > "$WORK_DIR/SOURCEDIR.txt"

echo "Release ID      : $TAGNAME-r$REVISION ($GITCOMMIT)"

#################################################################################################
# Configure

echo -e "\n---------- Configure QElectroTech $QET_VERSION\n"

if [[ $QET_DEBUG = 1 ]] ; then
    BUILD_TYPE=RelWithDebInfo
else
    BUILD_TYPE=Release
fi

if [[ $QET_SPACEMOUSE = 1 ]] ; then
    SPACEMOUSE_OPTIONS="-DQET_ENABLE_SPACEMOUSE=ON -DQET_SPACEMOUSE_BACKEND=hid"
else
    SPACEMOUSE_OPTIONS="-DQET_ENABLE_SPACEMOUSE=OFF"
fi

# An empty launcher means "no launcher" for CMake.
CCACHE_BIN=""
if [[ $QET_CCACHE = 1 ]] ; then
    CCACHE_BIN=$(command -v ccache || true)
fi

# Build tree inside the checkout, removed on every run like the
# former build-macos-arm64 of MacQetDeploy_arm64_cmake.sh.
BUILD_DIR="$QET_SRC/build-macos-homebrew"
rm -rf "$BUILD_DIR"

cmake -S . -B "$BUILD_DIR" -G Ninja \
      -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
      -DCMAKE_PREFIX_PATH="$KF_PREFIX;$BREW_PREFIX" \
      -DCMAKE_C_COMPILER_LAUNCHER="$CCACHE_BIN" \
      -DCMAKE_CXX_COMPILER_LAUNCHER="$CCACHE_BIN" \
      $COMMON_CMAKE_OPTIONS \
      -DQT_VERSION_MAJOR=6 \
      -DPACKAGE_TESTS=OFF \
      -DBUILD_TESTING=OFF \
      -DQET_EXPORT_PROJECT_DB=ON \
      -DSQLite3_ROOT="$SQLITE_PREFIX" \
      $KF_OPTIONS \
      $SPACEMOUSE_OPTIONS \
      -Wno-dev

#################################################################################################
# Build

echo -e "\n---------- Building QElectroTech $QET_VERSION\n"

[[ -n "$CCACHE_BIN" ]] && "$CCACHE_BIN" -z >/dev/null 2>&1 || true

cmake --build "$BUILD_DIR" -j$CPU_CORES

[[ -n "$CCACHE_BIN" ]] && "$CCACHE_BIN" -s 2>/dev/null | grep -i -E 'hits|misses' || true

# add_executable(... MACOSX_BUNDLE) without RUNTIME_OUTPUT_DIRECTORY:
# the .app is at the root of the build tree.
if [[ ! -x "$BUILD_DIR/qelectrotech.app/Contents/MacOS/qelectrotech" ]] ; then
    echo "ERROR: $BUILD_DIR/qelectrotech.app not found after the build."
    exit 1
fi

echo "$BUILD_DIR" > "$WORK_DIR/BUILDDIR.txt"

#################################################################################################

TerminateScript
