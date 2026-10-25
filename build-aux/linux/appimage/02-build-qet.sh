#!/bin/bash

# Build QElectroTech and install it into the AppDir.
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
exec > >(tee ./logs/build-qet.full.log) 2>&1

#################################################################################################

echo "02-build-qet.sh : build QElectroTech."
echo "-------------------------------------"

#################################################################################################
# Pre-processing checks

. ./config.sh
. ./common.sh
ChecksNotRoot
StartScript
ChecksCPUCores
HostAdjustments

ORIG_WD="`pwd`"
QET_SRC="$BUILDING_DIR/qelectrotech-$QET_VERSION"

#################################################################################################
# Checkout QElectroTech

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

git submodule update --init --recursive

#################################################################################################
# Release ID : major.minor.micro from sources/qetversion.cpp + revision, as in
# the former packaging_script_AppImage.sh

TAGNAME=$(grep "return QVersionNumber{" sources/qetversion.cpp | head -n 1 \
          | awk -F "{" '{ print $2 }' | awk -F "}" '{ print $1 }' | sed -e 's/,/./g' -e 's/ //g')
REVISION=$(( $(git rev-list HEAD --count) + 473 ))
GITCOMMIT=$(git rev-parse --short HEAD)

echo "$TAGNAME-r$REVISION" > "$ORIG_WD/data/RELEASEID.txt"
echo "$GITCOMMIT"          > "$ORIG_WD/data/GITCOMMIT.txt"

echo "Release ID     : $TAGNAME-r$REVISION ($GITCOMMIT)"

#################################################################################################
# Configure

echo -e "\n---------- Configure QElectroTech $QET_VERSION\n"

if [[ $QET_DEBUG = 1 ]] ; then
    BUILD_TYPE=RelWithDebInfo
else
    BUILD_TYPE=Release
fi

if [[ $QET_WITH_KF = 1 ]] ; then
    KF_OPTIONS="-DBUILD_WITH_KF=ON -DBUILD_KF=OFF"
else
    KF_OPTIONS="-DBUILD_WITH_KF=OFF"
fi

if [[ $QET_SPACEMOUSE = 1 ]] ; then
    SPACEMOUSE_OPTIONS="-DQET_ENABLE_SPACEMOUSE=ON -DQET_SPACEMOUSE_BACKEND=hid"
else
    SPACEMOUSE_OPTIONS="-DQET_ENABLE_SPACEMOUSE=OFF"
fi

#################################################################################################
# KDE Frameworks : built apart, as digiKam does with ext_kf6. QET's own
# BUILD_KF=ON (FetchContent) cannot work: kcoreaddons does not find the
# fetched ECM with find_package(ECM).

if [[ $QET_WITH_KF = 1 ]] ; then

    if [[ ! -f "$KF_PREFIX/.built-$KF_VERSION" ]] ; then

        echo -e "\n---------- Build KDE Frameworks $KF_VERSION in $KF_PREFIX\n"

        rm -rf "$KF_PREFIX" "$BUILDING_DIR/kf6"
        mkdir -p "$BUILDING_DIR/kf6"

        for MODULE in extra-cmake-modules kcoreaddons kwidgetsaddons ; do

            git clone --depth 1 --branch "$KF_VERSION" \
                "$KF_GITBASE/$MODULE.git" "$BUILDING_DIR/kf6/$MODULE"

            cmake -S "$BUILDING_DIR/kf6/$MODULE" -B "$BUILDING_DIR/kf6/$MODULE/build" -G Ninja \
                  -DCMAKE_BUILD_TYPE=Release \
                  -DCMAKE_INSTALL_PREFIX="$KF_PREFIX" \
                  -DCMAKE_PREFIX_PATH="$QT_PREFIX;$KF_PREFIX" \
                  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
                  -DBUILD_SHARED_LIBS=OFF \
                  -DBUILD_TESTING=OFF \
                  -DBUILD_QCH=OFF \
                  -DBUILD_DESIGNERPLUGIN=OFF \
                  -DBUILD_PYTHON_BINDINGS=OFF \
                  -DKCOREADDONS_USE_QML=OFF \
                  -DKDE_SKIP_TEST_SETTINGS=ON \
                  -Wno-dev

            cmake --build "$BUILDING_DIR/kf6/$MODULE/build" -j$CPU_CORES
            cmake --install "$BUILDING_DIR/kf6/$MODULE/build"

        done

        touch "$KF_PREFIX/.built-$KF_VERSION"

    fi

    KF_OPTIONS="-DBUILD_WITH_KF=ON -DBUILD_KF=OFF"

fi

#################################################################################################

cd "$QET_SRC"

rm -rf build
cmake -S . -B build -G Ninja \
      -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
      -DCMAKE_INSTALL_PREFIX=/usr \
      -DCMAKE_PREFIX_PATH="$QT_PREFIX;$KF_PREFIX" \
      -DPACKAGE_TESTS=OFF \
      -DBUILD_TESTING=OFF \
      -DQET_EXPORT_PROJECT_DB=ON \
      $KF_OPTIONS \
      $SPACEMOUSE_OPTIONS \
      -Wno-dev

#################################################################################################
# Build

echo -e "\n---------- Building QElectroTech $QET_VERSION\n"

cmake --build build -j$CPU_CORES

#################################################################################################
# Install into a fresh AppDir (prefix /usr inside the AppDir)

echo -e "\n---------- Installing QElectroTech into $APP_IMG_DIR\n"

rm -rf "$APP_IMG_DIR"
DESTDIR="$APP_IMG_DIR" cmake --install build

#################################################################################################

TerminateScript
