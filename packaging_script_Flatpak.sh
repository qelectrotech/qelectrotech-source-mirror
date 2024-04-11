#!/bin/bash
#set -x

cd sources
git reset --hard origin/master

cd ..

# get updates
git submodule init
git submodule update
git pull --recurse-submodules

#sed -i 's/DEFINES += QET_EXPORT_PROJECT_DB/#DEFINES += QET_EXPORT_PROJECT_DB/' qelectrotech.pro

# get the number of the new revision
GITCOMMIT=$(git rev-parse --short HEAD)

tagName=$(cat sources/qetversion.cpp | grep "return QVersionNumber{"| head -n 1| awk -F "{" '{ print $2 }' | awk -F "}" '{ print $1 }' | sed -e 's/,/./g' -e 's/ //g')
#tagName=$(cat sources/qetversion.cpp | grep "return QVersionNumber{ 0, "| head -n 1| cut -c32-40| sed -e 's/,/./g' -e 's/ //g')   #Find major, minor, and micro version numbers in sources/qetversion.cp

cd ~
flatpak-builder --force-clean --ccache --repo=qet_git/0.8-dev qet_git/build-dir qet_git/build-aux/flatpak/org.qelectrotech.QElectroTech.json --gpg-sign=XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX --gpg-homedir=gpg
flatpak build-update-repo qet_git/0.8-dev --title="Development builds of QElectroTech" --prune --prune-depth=4 --generate-static-deltas --gpg-sign=XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX --gpg-homedir=gpg
rsync -e ssh -av --delete-after --no-owner --no-g --chmod=g+w --progress qet_git/0.8-dev/ server:download.qelectrotech.org/qet/qet-repository/builds/Flatpak/
