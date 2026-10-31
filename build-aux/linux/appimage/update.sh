#!/bin/bash

# Rebuild only QElectroTech and the AppImage bundle (host already prepared).
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

. ./config.sh
. ./common.sh
StartScript
ChecksNotRoot

echo "+++++++++++++++++++++ Update QElectroTech AppImage bundle ++++++++++++++++++++++"

./02-build-qet.sh
./03-build-appimage.sh

echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

TerminateScript
