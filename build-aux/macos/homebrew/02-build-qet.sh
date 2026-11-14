#!/bin/bash

# Build the libraries QET needs (once), then QElectroTech.
#
#   - QT_FROM=aqt : sqlite and hidapi, universal and static, in $DEPS_PREFIX
#   - KDE Frameworks : ECM, KCoreAddons, KWidgetsAddons, static, in $KF_PREFIX
#   - QElectroTech, for every architecture of OSX_ARCHS
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

echo "02-build-qet.sh : build the libraries and QElectroTech."
echo "-------------------------------------------------------"

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
    echo "ERROR: Qt6 not found in $QT_PREFIX : run 01-build-homebrew.sh first."
    exit 1
fi

ORIG_WD="`pwd`"

# Minimal macOS version of the Qt used: OSX_MIN_TARGET must not be lower,
# or macOS refuses to start QET (-10825 error, "minos" of the binary newer
# than LSMinimumSystemVersion).
QT_MINOS=$(MachoMinos "$QT_PREFIX/lib/QtCore.framework/QtCore")
echo "QtCore minos    : ${QT_MINOS:-unknown}"

if [[ -n "$QT_MINOS" ]] ; then
    _qt_major=${QT_MINOS%%.*}
    _min_major=${OSX_MIN_TARGET%%.*}
    if [[ $_qt_major -gt $_min_major ]] ; then
        echo "ERROR: this Qt needs macOS $QT_MINOS, but OSX_MIN_TARGET is $OSX_MIN_TARGET."
        echo "       Raise OSX_MIN_TARGET in config.sh (users of older macOS could not"
        echo "       start the DMG), or use QT_FROM=aqt."
        exit 1
    fi
fi

# Common to every CMake project built here. CMAKE_IGNORE_PREFIX_PATH keeps
# the Homebrew libraries out of a universal build (they are arm64 only).
COMMON_CMAKE_OPTIONS="\
-DCMAKE_OSX_DEPLOYMENT_TARGET=$OSX_MIN_TARGET \
-DCMAKE_OSX_ARCHITECTURES=$OSX_ARCHS \
-DCMAKE_PREFIX_PATH=$BUILD_PREFIX_PATH \
"

if [[ -n "$BUILD_IGNORE_PATH" ]] ; then
    COMMON_CMAKE_OPTIONS="$COMMON_CMAKE_OPTIONS -DCMAKE_IGNORE_PREFIX_PATH=$BUILD_IGNORE_PATH"
fi

# Rebuild the libraries when any of these settings changes
LIBS_STAMP="$QT_FROM-$QT_VERSION-$OSX_MIN_TARGET-$(echo $OSX_ARCHS_LIST | tr ' ' '-')"

#################################################################################################
# sqlite and hidapi, universal, with QT_FROM=aqt (Homebrew's are arm64 only).
# Static: nothing to deploy or sign in the bundle.

if [[ "$QT_FROM" = "aqt" ]] ; then

    DEPS_STAMP="$DEPS_PREFIX/.built-sqlite$SQLITE_VERSION-hidapi$HIDAPI_VERSION-$LIBS_STAMP"

    if [[ ! -f "$DEPS_STAMP" ]] ; then

        echo -e "\n---------- Build sqlite $SQLITE_VERSION and hidapi $HIDAPI_VERSION in $DEPS_PREFIX\n"

        rm -rf "$DEPS_PREFIX" "$BUILDING_DIR/deps"
        mkdir -p "$BUILDING_DIR/deps"

        ARCH_FLAGS=""
        for _a in $OSX_ARCHS_LIST ; do
            ARCH_FLAGS="$ARCH_FLAGS -arch $_a"
        done

        # sqlite: canonical sources from the GitHub mirror. Since 3.48 they
        # build with autosetup, without TCL. --enable-session: the session
        # extension (sqlite3session_*, sqlite3changeset_*) used by QetDbFile
        # for the .qetdb history, absent from the sqlite3.h of the macOS SDK.
        git clone --depth 1 --branch "version-$SQLITE_VERSION" \
            "$SQLITE_GITURL" "$BUILDING_DIR/deps/sqlite"

        cd "$BUILDING_DIR/deps/sqlite"

        CFLAGS="-O2 $ARCH_FLAGS -mmacosx-version-min=$OSX_MIN_TARGET" \
        ./configure --prefix="$DEPS_PREFIX" \
                    --enable-session \
                    --disable-shared \
                    --disable-tcl

        make -j$CPU_CORES
        make install

        cd "$ORIG_WD"

        # hidapi: CMake, darwin backend (IOKit)
        git clone --depth 1 --branch "hidapi-$HIDAPI_VERSION" \
            "$HIDAPI_GITURL" "$BUILDING_DIR/deps/hidapi"

        cmake -S "$BUILDING_DIR/deps/hidapi" -B "$BUILDING_DIR/deps/hidapi/build" -G Ninja \
              -DCMAKE_BUILD_TYPE=Release \
              -DCMAKE_INSTALL_PREFIX="$DEPS_PREFIX" \
              $COMMON_CMAKE_OPTIONS \
              -DBUILD_SHARED_LIBS=OFF \
              -DHIDAPI_BUILD_HIDTEST=OFF \
              -Wno-dev

        cmake --build "$BUILDING_DIR/deps/hidapi/build" -j$CPU_CORES
        cmake --install "$BUILDING_DIR/deps/hidapi/build"

        # Without its .pc file, QET's cmake/find_spacemouse.cmake uses hidapi's
        # CMake package, which carries the IOKit/CoreFoundation/AppKit
        # frameworks a static hidapi needs; pkg-config would not give them.
        rm -f "$DEPS_PREFIX"/lib/pkgconfig/hidapi*.pc

        for LIB in "$DEPS_PREFIX/lib/libsqlite3.a" "$DEPS_PREFIX"/lib/libhidapi*.a ; do
            ChecksArchs "$LIB" || { echo "ERROR: $LIB is not built for every architecture."; exit 1; }
        done

        touch "$DEPS_STAMP"

    else

        echo "sqlite $SQLITE_VERSION and hidapi $HIDAPI_VERSION already built in $DEPS_PREFIX, skipped."

    fi

fi

# The sqlite3.h of the macOS SDK has no session extension: QET must get
# Homebrew's (keg-only, given explicitly to CMake) or ours.
if [[ ! -f "$SQLITE_PREFIX/include/sqlite3.h" ]] ; then
    echo "ERROR: SQLite not found in $SQLITE_PREFIX : run 01-build-homebrew.sh first."
    exit 1
fi

#################################################################################################
# KDE Frameworks : only ECM, KCoreAddons and KWidgetsAddons, instead of the
# 25 modules of digiKam's 02-build-extralibs.sh. Built statically in their
# own prefix, outside Homebrew: nothing to deploy, fix or sign, and an old
# KF6 installed in /opt/homebrew by the digiKam scripts cannot interfere
# ($KF_PREFIX comes first in CMAKE_PREFIX_PATH).

if [[ $QET_WITH_KF = 1 ]] ; then

    KF_STAMP="$KF_PREFIX/.built-$KF_VERSION-$LIBS_STAMP"

    if [[ ! -f "$KF_STAMP" ]] ; then

        echo -e "\n---------- Build KDE Frameworks $KF_VERSION in $KF_PREFIX\n"

        rm -rf "$KF_PREFIX" "$BUILDING_DIR/kf6"
        mkdir -p "$BUILDING_DIR/kf6"

        for MODULE in extra-cmake-modules kcoreaddons kwidgetsaddons ; do

            git clone --depth 1 --branch "$KF_VERSION" \
                "$KF_GITBASE/$MODULE.git" "$BUILDING_DIR/kf6/$MODULE"

            cmake -S "$BUILDING_DIR/kf6/$MODULE" -B "$BUILDING_DIR/kf6/$MODULE/build" -G Ninja \
                  -DCMAKE_BUILD_TYPE=Release \
                  -DCMAKE_INSTALL_PREFIX="$KF_PREFIX" \
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

        touch "$KF_STAMP"

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

echo -e "\n---------- Building QElectroTech $QET_VERSION ($OSX_ARCHS_LIST)\n"

[[ -n "$CCACHE_BIN" ]] && "$CCACHE_BIN" -z >/dev/null 2>&1 || true

cmake --build "$BUILD_DIR" -j$CPU_CORES

[[ -n "$CCACHE_BIN" ]] && "$CCACHE_BIN" -s 2>/dev/null | grep -i -E 'hits|misses' || true

# add_executable(... MACOSX_BUNDLE) without RUNTIME_OUTPUT_DIRECTORY:
# the .app is at the root of the build tree.
QET_BIN="$BUILD_DIR/qelectrotech.app/Contents/MacOS/qelectrotech"

if [[ ! -x "$QET_BIN" ]] ; then
    echo "ERROR: $BUILD_DIR/qelectrotech.app not found after the build."
    exit 1
fi

ChecksArchs "$QET_BIN" || { echo "ERROR: qelectrotech is not built for every architecture."; exit 1; }
echo "qelectrotech    : $(lipo -archs "$QET_BIN"), minos $(MachoMinos "$QET_BIN")"

echo "$BUILD_DIR" > "$WORK_DIR/BUILDDIR.txt"

#################################################################################################

TerminateScript
