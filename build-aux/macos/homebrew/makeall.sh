#!/bin/bash

# Build the whole QElectroTech macOS DMG from scratch.
#
# Adapted from the digiKam Homebrew scripts by Gilles Caulier.
# Copyright (c) 2015-2026, Gilles Caulier, <caulier dot gilles at gmail dot com>
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

echo "This script will build from scratch the QElectroTech DMG for macOS."
echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

./01-build-homebrew.sh
./02-build-qet.sh
./03-build-installer.sh

echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

TerminateScript
