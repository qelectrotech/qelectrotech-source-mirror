#!/bin/sh
    # Copyright 2023 The QElectroTech Team
    # This file is part of QElectroTech.
    
    # QElectroTech is free software: you can redistribute it and/or modify
    # it under the terms of the GNU General Public License as published by
    # the Free Software Foundation, either version 2 of the License, or
    # (at your option) any later version.
    
    # QElectroTech is distributed in the hope that it will be useful,
    # but WITHOUT ANY WARRANTY; without even the implied warranty of
    # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    # GNU General Public License for more details.
    
    # You should have received a copy of the GNU General Public License
    # along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
 # Need homebrew and coreutils installed see <http://brew.sh>.

#Force MacOSX12.3.sdk
#see: https://www.downtowndougbrown.com/2023/08/how-to-create-a-qt-5-arm-intel-universal-binary-for-mac/

export DEVELOPER_DIR=/Applications/Xcode_14.01.app/Contents/Developer

# configuration
APPNAME='qelectrotech'
BUNDLE=$APPNAME.app
APPBIN="$BUNDLE/Contents/MacOS/$APPNAME"

# Script location
current_dir=$(dirname "$0")

# Go back to repo root
cd "${current_dir}/../"

# Current directory
current_dir=$(PWD)


### get system configuration ########################################

echo
echo "______________________________________________________________"
echo "This script prepares a Qt application bundle for deployment."
echo "Please see the \"Deploying an Application on Qt/Mac\""
echo "page in the Qt documentation for more information."
echo
echo "This script :"
echo "\t - update the git depot"
echo "\t - build the application bundle,"
echo "\t - copy over required Qt frameworks,"
echo "\t - copy additional files: translations, titleblocks and elements,"
echo "\t - create DMG disk image."
echo
echo "Enjoy ;-)"
echo

# as long as we can find qmake, we don't need QTDIR
FWPATH=`qmake -query QT_INSTALL_LIBS`
if [ ! -d $FWPATH/QtGui.framework ] ; then
    echo "ERROR: cannot find the Qt frameworks. Make sure Qt is installed"
    echo "and qmake is in your environment path."
    exit
fi


### GIT ####################################################

echo
echo "______________________________________________________________"
echo "Run GIT:"

git submodule init
git submodule update
git pull --recurse-submodules
git pull

# Get revision number and version
GITCOMMIT=$(git rev-parse --short HEAD)
A=$(git rev-list HEAD --count)
HEAD=$(($A+473))

VERSION=$(cat sources/qetversion.cpp | grep "return QVersionNumber{"| head -n 1| awk -F "{" '{ print $2 }' | awk -F "}" '{ print $1 }' | sed -e 's/,/./g' -e 's/ //g')

DMG_NAME="${APPNAME}-$VERSION-r$HEAD-arm64.dmg"
DMG_PATH="build-aux/mac-osx/$DMG_NAME"

# Check if already built
if [ -e "$DMG_PATH" ] ; then
    echo "There are not new updates, make disk image can"
    echo "take a lot of time (5 min). Can you continu?"
    echo  "[y/n]"
    read userinput
    if  [ "$userinput" == "n" ] ; then
        echo
        echo "Process is stopped."
        echo
        exit
    fi
fi


### make install ####################################################

echo
echo "______________________________________________________________"
echo "Run make install:"

# Remove old bundle
if [ -d $BUNDLE ] ; then
    echo "Removing old bundle..."
    rm -rf $BUNDLE
fi
if [ -e Makefile ] ; then
    echo "Removing old Makefile..."
    rm .qmake.stash
    make clean
fi

# Generate Makefile
echo "Generating new makefile..."
qmake -spec macx-clang

# Compile
if [ -e Makefile.Release ] ; then
    START_TIME=$SECONDS

    testSuccessBuild () {
        if [ $? -ne 0 ]; then
            cleanVerionTag
            ELAPSED_TIME=$(($SECONDS - $START_TIME))
            echo
            echo "make failed - $(($ELAPSED_TIME/60)) min $(($ELAPSED_TIME%60)) sec"
            exit 1
        fi
    }

    coeur=$(sysctl hw.ncpu | awk '{print $2}')
    if [ $? -ne 0 ]; then
        make -f Makefile.Release
        testSuccessBuild
    else
        make -j$(($coeur + 1)) -f Makefile.Release
        testSuccessBuild
    fi

    ELAPSED_TIME=$(($SECONDS - $START_TIME))
    echo
    echo "The time of compilation is $(($ELAPSED_TIME/60)) min $(($ELAPSED_TIME%60)) sec"
else
    echo "ERROR: Makefile not found. This script requires the macx-clang makespec"
    exit
fi

cp -R ${current_dir}/misc/Info.plist qelectrotech.app/Contents/
cp -R ${current_dir}/ico/mac_icon/*.icns qelectrotech.app/Contents/Resources/
/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString $VERSION r$HEAD" "qelectrotech.app/Contents/Info.plist"


### copy over frameworks ############################################

echo
echo "______________________________________________________________"
echo "Copy Qt libraries and private frameworks:"

echo "Processing Mac deployment tool..."
if [ ! -d $BUNDLE ] ; then
    echo "ERROR: cannot find application bundle \"$BUNDLE\" in current directory"
    exit
fi
macdeployqt $BUNDLE

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
LANG_DIR="${current_dir}/lang1/"

if [ -d "${QET_ELMT_DIR}" ]; then
    echo "Copying elements in the bundle..."
    cp -R ${QET_ELMT_DIR} $BUNDLE/Contents/Resources/elements
fi

if [ -d "${QET_TBT_DIR}" ]; then
    echo "Copying titleblocks in the bundle..."
    cp -R ${QET_TBT_DIR} $BUNDLE/Contents/Resources/titleblocks
fi

if [ -d "${QET_LANG_DIR}" ]; then
    echo "Copying translations in the bundle..."
    mkdir $BUNDLE/Contents/Resources/lang
    cp ${current_dir}/lang/*.qm $BUNDLE/Contents/Resources/lang
fi

if [ -d "${LANG_DIR}" ]; then
    echo "Copying extra translations in the bundle..."
    cp ${current_dir}/lang1/*.qm $BUNDLE/Contents/Resources/lang
fi

if [ -d "${QET_EXAMPLES_DIR}" ]; then
    echo "Copying examples in the bundle..."
    mkdir $BUNDLE/Contents/Resources/examples
    cp ${current_dir}/examples/*.qet $BUNDLE/Contents/Resources/examples
fi

if [ -d "${QET_FONTS_DIR}" ]; then
    echo "Copying fonts in the bundle..."
    mkdir $BUNDLE/Contents/Resources/fonts
    cp ${current_dir}/fonts/*.ttf $BUNDLE/Contents/Resources/fonts
fi

if [ -d "${QET_LICENSES_DIR}" ]; then
    echo "Copying licenses in the bundle..."
    mkdir $BUNDLE/Contents/Resources/licenses
    cp -R -L ${QET_LICENSES_DIR} $BUNDLE/Contents/Resources/licenses
fi

### Sign the bundle #################################################

echo
echo "______________________________________________________________"
echo "Code signing bundle:"

codesign --force --deep --sign --timestamp \
    -s "Developer ID Application: Laurent TRINQUES (Y73WZ6WZ5X)" \
    --options=runtime $BUNDLE

### Create zip for notarization only ################################
# This ZIP is temporary — used only to submit to notarytool.
# The final deliverable will be a DMG (see below).

echo
echo "______________________________________________________________"
echo "Create temporary zip for notarization:"

NOTARIZE_ZIP="/tmp/${APPNAME}-$VERSION-r$HEAD-arm64-notarize.zip"
/usr/bin/ditto -c -k --keepParent $BUNDLE "$NOTARIZE_ZIP"

### Notarize ########################################################

echo -e "\033[1;31mWould you like to upload for notarization \"${APPNAME}-${VERSION}-r${HEAD}-arm64\", n/Y?\033[m"
read a
if [[ $a == "Y" || $a == "y" ]]; then
    echo
    echo "______________________________________________________________"
    echo "Notarizing:"
    xcrun notarytool submit "$NOTARIZE_ZIP" --keychain-profile "org.qelectrotech" --wait
else
    echo -e "\033[1;33mExit.\033[m"
fi

# Clean up temporary notarization zip
echo "Cleaning up temporary notarization zip..."
rm -f "$NOTARIZE_ZIP"

### Staple ##########################################################

echo -e "\033[1;31mWould you like to staple the app \"${APPNAME}-${VERSION}-r${HEAD}\", n/Y?\033[m"
read a
if [[ $a == "Y" || $a == "y" ]]; then
    xcrun stapler staple -v $BUNDLE
    # Verify staple is correctly applied
    echo "Verifying staple..."
    xcrun stapler validate -v $BUNDLE
    spctl -a -vv $BUNDLE
else
    echo -e "\033[1;33mExit.\033[m"
fi

### Create final DMG ################################################
# A DMG is used instead of a ZIP because it correctly preserves the
# Gatekeeper staple when downloaded via Chrome or any other browser.
# ZIP extraction via Archive Utility can strip extended attributes,
# causing Gatekeeper to block the app with an "unverified developer"
# warning. A signed and notarized DMG does not have this issue.

echo
echo "______________________________________________________________"
echo "Create final DMG (Gatekeeper-compatible with Chrome and Safari):"

mkdir -p "build-aux/mac-osx"

# Create a temporary writable DMG from the stapled .app bundle
hdiutil create \
    -volname "QElectroTech $VERSION" \
    -srcfolder "$BUNDLE" \
    -ov \
    -format UDRW \
    -fs HFS+ \
    "/tmp/qet_tmp.dmg"

# Convert to compressed read-only DMG
hdiutil convert "/tmp/qet_tmp.dmg" \
    -format UDZO \
    -o "$DMG_PATH"

rm -f "/tmp/qet_tmp.dmg"

# Sign the DMG itself
codesign \
    --sign "Developer ID Application: Laurent TRINQUES (Y73WZ6WZ5X)" \
    --timestamp \
    "$DMG_PATH"

echo "DMG created and signed: $DMG_PATH"

### Notarize the DMG ################################################

echo -e "\033[1;31mWould you like to notarize the DMG \"${DMG_NAME}\", n/Y?\033[m"
read a
if [[ $a == "Y" || $a == "y" ]]; then
    echo
    echo "______________________________________________________________"
    echo "Notarizing DMG:"
    xcrun notarytool submit "$DMG_PATH" --keychain-profile "org.qelectrotech" --wait

    echo "Stapling notarization ticket to DMG..."
    xcrun stapler staple "$DMG_PATH"
    echo "DMG notarized and stapled OK."
else
    echo -e "\033[1;33mExit.\033[m"
fi

### Clean up bundle #################################################

echo "Cleaning up bundle..."
rm -rf $BUNDLE

### The end #########################################################

echo
echo "______________________________________________________________"
echo "The process is done."
echo "DMG is in the folder 'build-aux/mac-osx'."

### Upload via rsync ################################################

echo -e "\033[1;31mWould you like to upload MacOS package \"${DMG_NAME}\", n/Y?\033[m"
read a
if [[ $a == "Y" || $a == "y" ]]; then
    cp -Rf "$DMG_PATH" /Users/laurent/MAC_OS_X/
    rsync -e ssh -av --delete-after --no-owner --no-g --chmod=g+w \
        --progress --exclude='.DS_Store' \
        /Users/laurent/MAC_OS_X/ \
        server:download.qelectrotech.org/qet/builds/MAC_OS_X/arm64/
    if [ $? != 0 ]; then
        echo "RSYNC ERROR: problem syncing ${DMG_NAME}, retrying..."
        rsync -e ssh -av --delete-after --no-owner --no-g --chmod=g+w \
            --progress --exclude='.DS_Store' \
            /Users/laurent/MAC_OS_X/ \
            server:download.qelectrotech.org/qet/builds/MAC_OS_X/arm64/
    fi
else
    echo -e "\033[1;33mExit.\033[m"
fi
