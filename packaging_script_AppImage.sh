#!/bin/bash
#set -x

#delete old qet.h
rm sources/qet.h

cd sources
git reset --hard origin/master

cd ..
# get updates
git pull --recurse-submodules

sed -i 's/DEFINES += QET_EXPORT_PROJECT_DB/#DEFINES += QET_EXPORT_PROJECT_DB/' qelectrotech.pro

GITCOMMIT=$(git rev-parse --short HEAD)
A=$(git rev-list HEAD --count)
HEAD=$(($A+473))

# We recover the version number of the original
tagName=$(sed -n "s/const QString displayedVersion =\(.*\)/\1/p" sources/qet.h | cut -d\" -f2 | cut -d\" -f1 )

# On modifie l'originale avec le numéro de révision du dépôt GIT
sed -i 's/'"const QString displayedVersion =.*/const QString displayedVersion = \"$tagName+$GITCOMMIT\";"'/' sources/qet.h

rm -Rf build/
mkdir build && cd build

qmake ../qelectrotech.pro
make -j48

cd ../appdir/
rm qelectrotech/usr/bin/qelectrotech
rm -Rf qelectrotech/usr/{lib,plugins}
rm -Rf qelectrotech/usr/share/{elements,examples,titleblocks,lang,man,doc}

cp ../build/qelectrotech qelectrotech/usr/bin/
cp -r ../{elements,examples,titleblocks,lang,man} qelectrotech/usr/share/

./linuxdeployqt-continuous-x86_64.AppImage qelectrotech/usr/share/qelectrotech.desktop  -appimage -bundle-non-qt-libs -verbose=1 -extra-plugins=iconengines
rm qelectrotech/AppRun
sed -i 's/'"QElectroTech_0.8-DEV.*/QElectroTech_0.8-DEV-r"$HEAD""'/' qelectrotech/qelectrotech.desktop
cp AppRun qelectrotech/
rm QElectroTech_*.AppImage

ARCH=x86_64 ./appimagetool-x86_64.AppImage qelectrotech
chmod -x QElectroTech_0.8-DEV-r$HEAD-x86_64.AppImage
