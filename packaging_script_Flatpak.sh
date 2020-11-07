#!/bin/bash
#set -x

#delete old qet.h
rm sources/qet.h

cd sources
git reset --hard origin/master

cd ..

# get updates
git pull

sed -i 's/DEFINES += QET_EXPORT_PROJECT_DB/#DEFINES += QET_EXPORT_PROJECT_DB/' qelectrotech.pro

# get the number of the new revision
GITCOMMIT=$(git rev-parse --short HEAD)

tagName=$(cat sources/qet.h | grep displayedVersion | cut -d\" -f2 | cut -d\" -f1)                 #Find displayedVersion tag in GIT sources/qet.h

# We recover the version number of the original
tagName=$(sed -n "s/const QString displayedVersion =\(.*\)/\1/p" sources/qet.h | cut -d\" -f2 | cut -d\" -f1 )

# We modify the original with the revision number of the svn repository
sed -i 's/'"const QString displayedVersion =.*/const QString displayedVersion = \"$tagName+$GITCOMMIT\";"'/' sources/qet.h

cd ~
flatpak-builder --force-clean --ccache --repo=qet_git/0.8-dev qet_git/build-dir qet_git/build-aux/flatpak/org.qelectrotech.QElectroTech.json --gpg-sign=XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX --gpg-homedir=gpg
flatpak build-update-repo qet_git/0.8-dev --title="Development builds of QElectroTech" --prune --prune-depth=4 --generate-static-deltas --gpg-sign=XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX --gpg-homedir=gpg
rsync -e ssh -av --delete-after --no-owner --no-g --chmod=g+w --progress qet_git/0.8-dev/ scorpio810@ssh.tuxfamily.org:/home/qet/qet-repository/builds/Flatpak/
