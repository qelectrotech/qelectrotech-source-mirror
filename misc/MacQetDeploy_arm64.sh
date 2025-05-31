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

# Emplacement du script
current_dir=$(dirname "$0")

# On se remet au depart 
cd "${current_dir}/../"

# Emplacement courant
current_dir=$(PWD)


### get system configuration ########################################

echo
echo "______________________________________________________________"
echo "This script prepares a Qt application bundle for deployment."
echo "Please see the \"Deploying an Application on Qt/Mac\""
echo "page in the Qt documentation for more information."
echo
echo "This script :"
echo "\t - up date the git depot"
echo "\t - built the application bundle,"
echo "\t - copy over required Qt frameworks,"
echo "\t - copy additional files: translations, titleblocks and elements,"
echo "\t - create image disk."
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

# Fait une mise à jour
git submodule init
git submodule update
git pull --recurse-submodules
git pull
#git checkout foliolist_position

# recupere le numero de la nouvelle revision

GITCOMMIT=$(git rev-parse --short HEAD)
A=$(git rev-list HEAD --count)
HEAD=$(($A+473))


VERSION=$(cat sources/qetversion.cpp | grep "return QVersionNumber{"| head -n 1| awk -F "{" '{ print $2 }' | awk -F "}" '{ print $1 }' | sed -e 's/,/./g' -e 's/ //g')
#VERSION=$(cat sources/qetversion.cpp | grep "return QVersionNumber{ 0, "| head -n 1| cut -c32-40| sed -e 's/,/./g' -e 's/ //g')   #Find major, minor, and micro version numbers in sources/qetversion.cp

# Tarball de la dernière revision déjà créé
if [ -e "build-aux/mac-osx/${APPNAME}-$VERSION-r$HEAD-arm64.zip" ] ; then
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

# pour effacer l’ancienne compilation
if [ -d $BUNDLE ] ; then
    echo "Removing hold bundle..."
    rm -rf $BUNDLE
fi
if [ -e Makefile ] ; then
    echo "Removing hold Makefile..."
    rm .qmake.stash
    make clean
fi

# genere le Makefile
echo "Generating new makefile..."
qmake -spec macx-clang 

# compilation
if [ -e Makefile.Release ] ; then
	START_TIME=$SECONDS
    
    # arret du script si erreur de compilation
    testSuccessBuild () {
        if [ $? -ne 0 ]; then 
            cleanVerionTag
            ELAPSED_TIME=$(($SECONDS - $START_TIME))
            echo
            echo "make failed - $(($ELAPSED_TIME/60)) min $(($ELAPSED_TIME%60)) sec"
            exit 1
        fi
    }

    # utilise tout les coeurs pour une compilation plus rapide
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
# On rajoute le numero de version pour "cmd + i"
/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString $VERSION r$HEAD" "qelectrotech.app/Contents/Info.plist"  # Version number


### copy over frameworks ############################################

echo
echo "______________________________________________________________"
echo "Copy Qt libraries and private frameworks:"

echo "Processing Mac deployment tool..." 
if [ ! -d $BUNDLE ] ; then
    echo "ERROR: cannot find application bundle \"$BUNDLE\" in current directory"
    exit
fi
#~/Qt/5.5/clang_64/bin/macdeployqt $BUNDLE
macdeployqt $BUNDLE

### add file missing #######################################

echo
echo "______________________________________________________________"
echo "Copy file missing:"

# Dossier à ajouter
QET_ELMT_DIR="${current_dir}/elements/"
QET_TBT_DIR="${current_dir}/titleblocks/"
QET_LANG_DIR="${current_dir}/lang/"
QET_EXAMPLES_DIR="${current_dir}/examples/"
QET_FONTS_DIR="${current_dir}/fonts/"
QET_LICENSES_DIR="${current_dir}/licenses/"


# Add new folder for Qt dialog translation see
## see <https://download.tuxfamily.org/qet/Qt_lang/>.

LANG_DIR="${current_dir}/lang1/"

if [ -d "${QET_ELMT_DIR}" ]; then
    echo "Copying add elements in the bundle..."
    #mkdir $BUNDLE/Contents/Resources/elements
    cp -R ${QET_ELMT_DIR} $BUNDLE/Contents/Resources/elements
fi

if [ -d "${QET_TBT_DIR}" ]; then
    echo "Copying titleblocks in the bundle..."
    #mkdir $BUNDLE/Contents/Resources/titleblocks
    cp -R ${QET_TBT_DIR} $BUNDLE/Contents/Resources/titleblocks
fi

if [ -d "${QET_LANG_DIR}" ]; then
    echo "Copying translations in the bundle... "
    mkdir $BUNDLE/Contents/Resources/lang
    cp ${current_dir}/lang/*.qm $BUNDLE/Contents/Resources/lang
fi

if [ -d "${LANG_DIR}" ]; then
   echo "Copying translations in the bundle... "
   cp ${current_dir}/lang1/*.qm $BUNDLE/Contents/Resources/lang

fi

if [ -d "${QET_EXAMPLES_DIR}" ]; then
   echo "Copying examples in the bundle... "
   mkdir $BUNDLE/Contents/Resources/examples
   cp ${current_dir}/examples/*.qet $BUNDLE/Contents/Resources/examples

fi

if [ -d "${QET_FONTS_DIR}" ]; then
   echo "Copying fonts in the bundle... "
   mkdir $BUNDLE/Contents/Resources/fonts
   cp ${current_dir}/fonts/*.ttf $BUNDLE/Contents/Resources/fonts

fi

if [ -d "${QET_LICENSES_DIR}" ]; then
    echo "Copying licenses in the bundle..."
    mkdir $BUNDLE/Contents/Resources/licenses
    cp -R ${QET_LICENSES_DIR} $BUNDLE/Contents/Resources/licenses
fi

codesign  --force --deep --sign --timestamp -s "Developer ID Application: Laurent TRINQUES (Y73WZ6WZ5X)" --options=runtime $BUNDLE
### create zip tarball ###############################################

echo
echo "______________________________________________________________"
echo "Create zip tarball:"

/usr/bin/ditto -c -k --keepParent $BUNDLE "build-aux/mac-osx/${APPNAME}-$VERSION-r$HEAD-arm64.zip"

### notarize zip tarball ###############################################
echo  -e "\033[1;31mWould you like to upload for Notarize packages "${APPNAME}"-"$VERSION"-"r$HEAD-arm64.zip", n/Y?.\033[m"
read a
if [[ $a == "Y" || $a == "y" ]]; then
echo
echo "______________________________________________________________"
echo "Notarize zip tarball:"

xcrun notarytool submit build-aux/mac-osx/${APPNAME}-$VERSION-r$HEAD-arm64.zip --keychain-profile "org.qelectrotech" --wait 
else
echo  -e "\033[1;33mExit.\033[m"

fi

### The end, process is done ##########################################

echo
echo "______________________________________________________________"
echo "The process of creating deployable application zip is done."
echo The disque image is in the folder \'build-aux/mac-osx\'.
# Affiche les mise à jour depuis l'ancienne revision 
#if [ ! $(($HEAD - $revAv)) -eq 0 ] ; then
#    echo
#    echo "There are new updates. This numero of revision is $HEAD."
 #   svn log -l $(($HEAD - $revAv))
#else
 #   echo
#    echo "There are not new updates. This numero of revision is $HEAD."
# fi
# echo

# La version en local n'est pas conforme à la dernière version svn
# svnversion | grep -q '[MS:]' ; if [ $? -eq 0 ] ; then 
#     echo Please note that the latest \local version is $(svnversion).
#     echo This is not the same version as the deposit. 
#     echo You can use \'svn diff\' to see the differences. 
#     echo And use \'svn revert \<fichier\>\' to delete the difference. 
#     echo To go back, you can use svn update -r 360 
#     echo to go to revision number 360.
#     echo
#fi 

# Clean up disk folder
echo 'Cleaning up... '
rm "build-aux/mac-osx/${APPNAME}-$VERSION-r$HEAD-arm64.zip"

# staple the app
echo  -e "\033[1;31mWould you like to  staple the app MacOS packages "${APPNAME}"-"$VERSION"-"r$HEAD", n/Y?.\033[m"
read a
if [[ $a == "Y" || $a == "y" ]]; then
xcrun stapler staple -v $BUNDLE
else
echo  -e "\033[1;33mExit.\033[m"

fi


echo
echo "______________________________________________________________"
echo "Re Create zip tarball:"

/usr/bin/ditto -c -k --sequesterRsrc --keepParent $BUNDLE "build-aux/mac-osx/${APPNAME}-$VERSION-r$HEAD-arm64.zip"


# Clean up disk folder
echo 'Cleaning up... '
rm -rf $BUNDLE


#rsync to TF DMG builds
echo  -e "\033[1;31mWould you like to upload MacOS packages "${APPNAME}"-"$VERSION"-"r$HEAD-arm64.zip", n/Y?.\033[m"
read a
if [[ $a == "Y" || $a == "y" ]]; then
cp -Rf "build-aux/mac-osx/${APPNAME}-$VERSION-r$HEAD-arm64.zip" /Users/laurent/MAC_OS_X/
rsync -e ssh -av --delete-after --no-owner --no-g --chmod=g+w --progress --exclude='.DS_Store' /Users/laurent/MAC_OS_X/ server:download.qelectrotech.org/qet/builds/MAC_OS_X/arm64/
if [ $? != 0 ]; then
{
echo "RSYNC ERROR: problem syncing ${APPNAME}-$VERSION-r$HEAD-arm64.zip"
rsync -e ssh -av --delete-after --no-owner --no-g --chmod=g+w --progress --exclude='.DS_Store' /Users/laurent/MAC_OS_X/ server:download.qelectrotech.org/qet/builds/MAC_OS_X/arm64/

} fi

else
echo  -e "\033[1;33mExit.\033[m"

fi
