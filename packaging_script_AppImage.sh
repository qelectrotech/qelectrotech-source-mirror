#!/bin/bash 

set -x


#delete old qet.h
rm sources/qet.h

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

./linuxdeployqt-continuous-x86_64.AppImage qelectrotech/usr/share/org.qelectrotech.qelectrotech.desktop  -appimage -bundle-non-qt-libs -verbose=1 -extra-plugins=iconengines
rm qelectrotech/AppRun
sed -i 's/'"QElectroTech_*.*/QElectroTech_$tagName-r$HEAD"'/' qelectrotech/org.qelectrotech.qelectrotech.desktop
cp AppRun qelectrotech/
rm QElectroTech_*.AppImage

ARCH=x86_64 ./appimagetool-x86_64.AppImage qelectrotech
chmod -x QElectroTech_$tagName-r$HEAD-x86_64.AppImage
shasum -a 256 QElectroTech_$tagName-r$HEAD-x86_64.AppImage > QElectroTech_$tagName-r$HEAD-x86_64.AppImage-SHA256.txt
mv QElectroTech_$tagName-r$HEAD-x86_64.AppImage* ../AppImage/0.100.0/
cd ..
  #rsync to server
  echo -e "\033[1;31mWould you like to RSYNC Appimage to server n/Y?.\033[m"
  read a
  if [[ $a == "Y" || $a == "y" ]]; then
    echo -e "\033[1;33mRsync to server qelectrotech-$VERSION.r$HEAD .\033[m"
    echo -e	"\033[1;31mrsync to server password ssh and TF\033[m"
    cd $DEFAULT_DIR/script
    rsync -e ssh -av --delete-after --no-owner --no-g --chmod=g+w --progress ~/qelectrotech-source-mirror/AppImage/0.100.0/ server:download.qelectrotech.org/qet/builds/AppImage/0.100.0/
    if [ $? != 0 ]; then
    {
      echo "RSYNC ERROR: problem syncing qelectrotech-$VERSION.r$HEAD "
      rsync -e ssh -av --delete-after --no-owner --no-g --chmod=g+w --progress  ~/qelectrotech-source-mirror/AppImage/0.100.0/ server:download.qelectrotech.org/qet/builds/AppImage/0.100.0/
    } fi

  else
    echo -e "\033[1;33mExit.\033[m"
  fi
else
  echo  -e "\033[1;33mExit.\033[m"
fi
exit
