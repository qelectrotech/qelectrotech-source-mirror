#!/bin/bash 

#Based on raspberry pi 5 8 Gb Trixie
#sudo apt install git ssh rsync libqt5svg5-dev qt5-qmake qtbase5-dev libkf5widgetsaddons-dev libkf5coreaddons-dev libsqlite3-dev pkgconf libqt5waylandclient5-dev libqt5waylandcompositor5-dev g++ make
#mkdir -p AppImage/0.100.0/aarch64
# Get GIT sources
#git clone --recursive https://github.com/qelectrotech/qelectrotech-source-mirror.git
#git submodule update --init --recursive

set -x
#delete old qet.h
#rm sources/qet.h

cd sources
git reset --hard origin/master

cd ..

# Fait une mise à jour
git submodule update --init --recursive
git pull
#git checkout test_pugi

GITCOMMIT=$(git rev-parse --short HEAD)
A=$(git rev-list HEAD --count)
HEAD=$(($A+473))

#Find major, minor, and micro version numbers in sources/qetversion.cp

tagName=$(cat sources/qetversion.cpp | grep "return QVersionNumber{"| head -n 1| awk -F "{" '{ print $2 }' | awk -F "}" '{ print $1 }' | sed -e 's/,/./g' -e 's/ //g')
#tagName=$(cat sources/qetversion.cpp | grep "return QVersionNumber{ 0, "| head -n 1| cut -c32-40| sed -e 's/,/./g' -e 's/ //g')   #Find major, minor, and micro version numbers in sources/qetversion.cp

rm -rf build/
mkdir build && cd build
export EXTRA_PLATFORM_PLUGINS=libqwayland-generic.so
export EXTRA_QT_MODULES="waylandcompositor"

qmake ../qelectrotech.pro
make -j$(nproc)

cd ..
rm -rf AppDir/qelectrotech/

mkdir -p AppDir/qelectrotech/usr/bin
cp build/qelectrotech AppDir/qelectrotech/usr/bin/
mkdir -p AppDir/qelectrotech/usr/share/applications
cp misc/org.qelectrotech.qelectrotech.desktop AppDir/qelectrotech/usr/share/applications/qelectrotech.desktop
#cp misc/qelectrotech.appdata.xml AppDir/qelectrotech/usr/share/metainfo/
#sed -i 's/'"QElectroTech-*.*/QElectroTech-$tagName-r$HEAD"'/' AppDir/qelectrotech/usr/share/applications/qelectrotech.desktop
mkdir -p AppDir/qelectrotech/usr/share/icons/hicolor/256x256/apps
mkdir -p AppDir/qelectrotech/usr/translations
cd AppDir/qelectrotech
cp -r ../../{elements,examples,titleblocks,lang,man} usr/share/
cp -r ../../ico/breeze-icons/256x256/apps/*.png usr/share/icons/hicolor/256x256/apps/

cd ../../



# Deploy the dependencies into the AppDir/qelectrotech
# Get 64-bit ARM version of linuxdeploy
#https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20250213-2/linuxdeploy-aarch64.AppImage
# Get 64-bit ARM version of linuxdeploy-plugin-qt
#https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/1-alpha-20250213-1/linuxdeploy-plugin-qt-aarch64.AppImage

./linuxdeploy-aarch64.AppImage --appdir AppDir/qelectrotech --plugin qt
rm AppDir/qelectrotech/AppRun
echo -e '#! /bin/sh \n\ncd $(dirname $(readlink -f $0))/usr/bin/ \nexec ./qelectrotech --common-elements-dir=../share/elements/ --common-tbt-dir=../share/titleblocks/ --lang-dir=../share/lang/ $@' > AppDir/qelectrotech/AppRun
chmod +x AppDir/qelectrotech/AppRun

# Get 64-bit ARM version of go-appimage appimagetool
#wget -c https://github.com/$(wget -q https://github.com/probonopd/go-appimage/releases/expanded_assets/continuous -O - | grep "appimagetool-.*-aarch64.AppImage" | head -n 1 | cut -d '"' -f 2)
#chmod +x appimagetool-*.AppImage
VERSION=$tagName-r$HEAD ./appimagetool-9*.AppImage ./AppDir/qelectrotech/


chmod -x QElectroTech-*.AppImage
shasum -a 256 QElectroTech-$tagName-r$HEAD-aarch64.AppImage > QElectroTech-$tagName-r$HEAD-aarch64.AppImage-SHA256.txt
mv QElectroTech-$tagName-r$HEAD-aarch64.AppImage* ./AppImage/0.100.0/aarch64/trixie
cd ..
  #rsync to server
  echo -e "\033[1;31mWould you like to RSYNC Appimage to server n/Y?.\033[m"
  read a
  if [[ $a == "Y" || $a == "y" ]]; then
    echo -e "\033[1;33mRsync to server qelectrotech-$VERSION.r$HEAD .\033[m"
    echo -e	"\033[1;31mrsync to server password ssh and TF\033[m"
    #cd $DEFAULT_DIR/script
    rsync -e ssh -av --delete-after --no-owner --no-g --chmod=g+w --progress ~/qelectrotech-source-mirror/AppImage/0.100.0/aarch64/ server:download.qelectrotech.org/qet/builds/AppImage/0.100.0/aarch64/
    if [ $? != 0 ]; then
    {
      echo "RSYNC ERROR: problem syncing qelectrotech-$VERSION.r$HEAD "
      rsync -e ssh -av --delete-after --no-owner --no-g --chmod=g+w --progress  ~/qelectrotech-source-mirror/AppImage/0.100.0/aarch64/ server:download.qelectrotech.org/qet/builds/AppImage/0.100.0/aarch64/
    } fi

  else
    echo -e "\033[1;33mExit.\033[m"
  fi
else
  echo  -e "\033[1;33mExit.\033[m"
fi
exit
