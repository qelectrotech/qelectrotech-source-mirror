#!/bin/bash
#set -x

#delete old qet.h
rm sources/qet.h

cd sources
git reset --hard origin/master

cd ..

# Fait une mise à jour
git pull
#git checkout test_pugi

sed -i 's/DEFINES += QET_EXPORT_PROJECT_DB/#DEFINES += QET_EXPORT_PROJECT_DB/' qelectrotech.pro

GITCOMMIT=$(git rev-parse --short HEAD)
A=$(git rev-list HEAD --count)
HEAD=$(($A+473))

VERSION=$(cat sources/qet.h | grep "const QString version" | cut -d\" -f2 | cut -d\" -f1)          #Find version tag in GIT sources/qet.h
tagName=$(cat sources/qet.h | grep displayedVersion | cut -d\" -f2 | cut -d\" -f1)                 #Find displayedVersion tag in GIT sources/qet.h

# recupere le numero de la nouvelle revision
#revAp=$(svnversion | cut -d : -f 2 | tr -d '[:alpha:]')

# On recupere le numero de version de l'originale
tagName=$(sed -n "s/const QString displayedVersion =\(.*\)/\1/p" sources/qet.h | cut -d\" -f2 | cut -d\" -f1 )

# On modifie l'originale avec le numero de revision du depot svn
sed -i 's/'"const QString displayedVersion =.*/const QString displayedVersion = \"$tagName+$GITCOMMIT\";"'/' sources/qet.h

cd ~
flatpak-builder --force-clean --ccache --repo=qet_git/0.8-dev qet_git/build-dir qet_git/build-aux/flatpak/org.qelectrotech.QElectroTech.json --gpg-sign=XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX --gpg-homedir=gpg
flatpak build-update-repo qet_git/0.8-dev --title="Development builds of QElectroTech" --prune --prune-depth=4 --generate-static-deltas --gpg-sign=XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX --gpg-homedir=gpg
rsync -e ssh -av --delete-after --no-owner --no-g --chmod=g+w --progress qet_git/0.8-dev/ scorpio810@ssh.tuxfamily.org:/home/qet/qet-repository/builds/Flatpak/
