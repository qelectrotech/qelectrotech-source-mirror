#!/bin/bash
#set -x

#delete old qet.h
rm sources/qet.h

cd sources
git reset --hard origin/master

cd ..
# get updates
git submodule init
git submodule update
git pull --recurse-submodules
git pull

GITCOMMIT=$(git rev-parse --short HEAD)
A=$(git rev-list HEAD --count)
HEAD=$(($A+473))

#Find major, minor, and micro version numbers in sources/qetversion.cp
tagName=$(cat sources/qetversion.cpp | grep "return QVersionNumber{ 0, "| head -n 1| cut -c32-40| sed -e 's/,/./g' -e 's/ //g')   #Find major, minor, and micro version numbers in sources/qetversion.cp

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
sed -i 's/'"QElectroTech_*.*/QElectroTech_$tagName-r$HEAD"'/' qelectrotech/qelectrotech.desktop
cp AppRun qelectrotech/
rm QElectroTech_*.AppImage

ARCH=x86_64 ./appimagetool-x86_64.AppImage qelectrotech
chmod -x QElectroTech_$tagName-r$HEAD-x86_64.AppImage
shasum -a 256 QElectroTech_$tagName-r$HEAD-x86_64.AppImage > QElectroTech_$tagName-r$HEAD-x86_64.AppImage-SHA256.txt
