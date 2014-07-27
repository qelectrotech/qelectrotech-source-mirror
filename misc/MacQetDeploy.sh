#!/bin/sh
    # Copyright 2014 The QElectroTech Team
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

# configuration
APPNAME='qelectrotech'
BUNDLE=$APPNAME.app
APPBIN="$BUNDLE/Contents/MacOS/$APPNAME"

# Emplacement du script
current_dir=$(dirname "$0")

# On se remet au depart 
cd ${current_dir}/../

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
echo "\t - up date the svn depot"
echo "\t - built the application bundle,"
echo "\t - copy over required Qt frameworks,"
echo "\t - copy additionnal files: translations, titleblocks and elements,"
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


### subversion ####################################################

echo
echo "______________________________________________________________"
echo "Run subversion:"

# recupere le numero de la dernière revision
# Si il y a ':' garde la 2e partie
# Remplace les lettres par 'rien'
revAv=$(svnversion | cut -d : -f 2 | tr -d '[:alpha:]')

# Fait une mise à jour
svn up

# recupere le numero de la nouvelle revision
revAp=$(svnversion | cut -d : -f 2 | tr -d '[:alpha:]')

# Dmg de la dernière revision déjà créé
if [ -e "packaging/mac-osx/${APPNAME}_$revAp.dmg" ] ; then
    echo "There are not new updates, make disk image can"
    echo "take a lot of time (5 min). Can you continu?"
    echo  "[y/n]"
    read userinput
    if  [ "$userinput" == "n" ] ; then
        echo
        echo "Process is stoped."
        echo
        exit
    fi
fi


### Version tag ####################################################

echo
echo "______________________________________________________________"
echo "Add version tag:"

echo "Adding the version tag..."

# On sauve l'orginale
mkdir temp
cp -Rf "sources/qet.h" "temp/qet.h"

#cd 
# On modifie l'originale avec le numero de version
sed -i "" "s/const QString displayedVersion = \"0.4-dev\"/const QString displayedVersion = \"0.4-dev-r$revAp\"/" sources/qet.h

# Apres la compilation 
cleanVerionTag () {
    echo
    echo "______________________________________________________________"
    echo "Clean version tag:"

    # On remet le code source comme il etait
    echo "Cleaning version tag..."

    # On supprime le fichier modifier
    rm -rf "sources/qet.h"

    # On remet l'ancien original
    cp -Rf "temp/qet.h" "sources/qet.h"

    # On suprime l'ancienne copie
    rm -rf "temp"
}


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
    make clean
fi

# genere le Makefile
echo "Generating new makefile..."
qmake -spec macx-g++ 

# compilation
if [ -e Makefile.Release ] ; then
	START_TIME=$SECONDS
	make -f Makefile.Release
    if [ $? -ne 0 ]; then 
        cleanVerionTag
        ELAPSED_TIME=$(($SECONDS - $START_TIME))
        echo
        echo "make failed - $(($ELAPSED_TIME/60)) min $(($ELAPSED_TIME%60)) sec"
        exit 1
    fi
    cleanVerionTag
	ELAPSED_TIME=$(($SECONDS - $START_TIME))
	echo
	echo "The time of compilation is $(($ELAPSED_TIME/60)) min $(($ELAPSED_TIME%60)) sec" 
else
    echo "ERROR: Makefile not found. This script requires the macx-g++ makespec"
    exit
fi


### copy over frameworks ############################################

echo
echo "______________________________________________________________"
echo "Copy Qt libraries and private frameworks:"

echo "Processing Mac deployment tool..." 
if [ ! -d $BUNDLE ] ; then
    echo "ERROR: cannot find application bundle \"$BUNDLE\" in current directory"
    exit
fi
/usr/bin/macdeployqt-4.8 $BUNDLE

### add file missing #######################################

echo
echo "______________________________________________________________"
echo "Copy file missing:"

# Dossier à ajouter
QET_ELMT_DIR="${current_dir}/elements/"
QET_TBT_DIR="${current_dir}/titleblocks/"
QET_LANG_DIR="${current_dir}/lang/"

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
    #mkdir $BUNDLE/Contents/Resources/lang
    cp -R ${QET_LANG_DIR} $BUNDLE/Contents/Resources/lang
fi


### create disk image ###############################################

echo
echo "______________________________________________________________"
echo "Create disk image:"

# Direction
EX_Dir="examples"
imagedir="/tmp/$APPNAME.$$"

# Remove any previous disk folder or DMG
echo 'Preparing (removing hold files)... '
if [ -e "/Volumes/${APPNAME}" ]; then
    hdiutil detach -quiet "/Volumes/${APPNAME}"
fi
if [ -e "${APPNAME}_$revAp.dmg" ] ; then
    rm -f "${APPNAME}_$revAp.dmg"
fi
if [ -e "packaging/mac-osx/${APPNAME}_$revAp.dmg" ] ; then
    rm -f "packaging/mac-osx/${APPNAME}_$revAp.dmg"
fi
if [ -e $imagedir ] ; then
    rm -rf $imagedir
fi

# Create disk folder
echo 'Copying application bundle and examples... '
mkdir $imagedir
cp -Rf $BUNDLE $imagedir
cp -Rf $EX_Dir $imagedir
# Elimine les accents, symbole ...
strip "$imagedir/$APPBIN"
    
# Creating a disk image from a folder
echo 'Creating disk image... '
hdiutil create -quiet -ov -srcfolder $imagedir -format UDBZ -volname "${APPNAME}" "${APPNAME}_${revAp}.dmg"
hdiutil internet-enable -yes -quiet "${APPNAME}_${revAp}.dmg"

# Clean up disk folder
echo 'Cleaning up... '
cp -Rf "${APPNAME}_$revAp.dmg" "packaging/mac-osx/${APPNAME}_$revAp.dmg"
rm -f "${APPNAME}_$revAp.dmg"
rm -rf $imagedir
rm -rf $BUNDLE


### The end, process is done ##########################################

echo
echo "______________________________________________________________"
echo "The process of creating deployable application bundle is done."
echo The disque image is in the folder \'packaging/mac-osx\'.
# Affiche les mise à jour depuis l'ancienne revision 
if [ ! $(($revAp - $revAv)) -eq 0 ] ; then
    echo
    echo "There are new updates. This numero of revision is $revAp."
    svn log -l $(($revAp - $revAv))
else
    echo
    echo "There are not new updates. This numero of revision is $revAp."
fi
echo

# La version en local n'est pas conforme à la dernière version svn
svnversion | grep -q '[MS:]' ; if [ $? -eq 0 ] ; then 
    echo Please note that the latest \local version is $(svnversion).
    echo This is not the same version as the deposit. 
    echo You can use \'svn diff\' to see the differences. 
    echo And use \'svn revert \<fichier\>\' to delete the difference. 
    echo
fi 


