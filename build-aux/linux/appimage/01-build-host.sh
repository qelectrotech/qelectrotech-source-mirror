#!/bin/bash

# Prepare an Ubuntu/Xubuntu 24.04 host to build the QElectroTech AppImage.
# Installs the build packages with apt (sudo) and, with QT_FROM=aqt,
# the official Qt6 binaries with aqtinstall.
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
exec > >(tee ./logs/build-host.full.log) 2>&1

#################################################################################################

echo "01-build-host.sh : prepare the Ubuntu 24.04 host to build the QElectroTech AppImage."
echo "-------------------------------------------------------------------------------------"

#################################################################################################
# Pre-processing checks

. ./config.sh
. ./common.sh
ChecksNotRoot
StartScript
ChecksCPUCores
HostAdjustments

mkdir -p "$DOWNLOAD_DIR" "$BUILDING_DIR"

#################################################################################################

echo -e "---------- Update Linux Host\n"

sudo apt-get update
sudo apt-get -y dist-upgrade

#################################################################################################

echo -e "---------- Install Development Packages\n"

# Base tools and libraries needed by QET and by the Qt xcb/wayland plugins.
sudo apt-get install -y \
     build-essential \
     cmake \
     ninja-build \
     git \
     wget \
     file \
     patchelf \
     desktop-file-utils \
     python3-pip \
     python3-venv \
     pkg-config \
     gdb \
     libsqlite3-dev \
     libgl-dev \
     libegl-dev \
     libfontconfig1-dev \
     libfreetype-dev \
     libxkbcommon-dev \
     libxkbcommon-x11-0 \
     libxcb-cursor0 \
     libxcb-icccm4 \
     libxcb-image0 \
     libxcb-keysyms1 \
     libxcb-randr0 \
     libxcb-render-util0 \
     libxcb-shape0 \
     libxcb-xinerama0 \
     libxcb-xkb1 \
     libwayland-client0 \
     libwayland-cursor0 \
     libwayland-egl1 \
     libcups2-dev \
     libssl-dev

if [[ $QET_SPACEMOUSE = 1 ]] ; then
    sudo apt-get install -y libhidapi-dev
fi

if [[ "$QT_FROM" = "system" ]] ; then

    echo -e "---------- Install Qt6 from Ubuntu packages\n"

    sudo apt-get install -y \
         qt6-base-dev \
         qt6-base-private-dev \
         qt6-svg-dev \
         qt6-tools-dev \
         qt6-tools-dev-tools \
         qt6-l10n-tools \
         qt6-declarative-dev \
         qt6-pdf-dev \
         qt6-translations-l10n \
         qt6-image-formats-plugins \
         qt6-wayland \
         qt6-qpa-plugins

else

    echo -e "---------- Install Qt $QT_VERSION with aqtinstall\n"

    python3 -m venv "$WORK_DIR/aqt-venv"
    "$WORK_DIR/aqt-venv/bin/pip" install --upgrade aqtinstall

    "$WORK_DIR/aqt-venv/bin/aqt" install-qt linux desktop "$QT_VERSION" "$QT_ARCH" \
         -m $QT_AQT_MODULES \
         -O "$QT_ROOT"

fi

#################################################################################################

echo -e "---------- Download AppImage tools\n"

cd "$DOWNLOAD_DIR"

# appimagetool with the static type2 runtime: the AppImage does not need
# libfuse2 on the target (Ubuntu 24.04 no longer installs it by default).
wget -q -N https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage
chmod a+x appimagetool-x86_64.AppImage

# Libraries that are part of the base system and must not be bundled.
wget -q -N https://raw.githubusercontent.com/AppImageCommunity/pkg2appimage/master/excludelist

#################################################################################################

TerminateScript
