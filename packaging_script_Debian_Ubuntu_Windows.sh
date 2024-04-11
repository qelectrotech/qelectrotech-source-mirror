#!/bin/bash
#nettoyage des chroots
#set -x
eval $(gpg-agent --daemon)

#variables repertoires de travail
DEFAULT_DIR="$HOME/paquet_qet"
TEMP_DIR="$HOME"
SSH_TARGET='scorpio810@ssh.tuxfamily.org:/home/qet/qelectrotech.org-web/htdocs/dl_link.inc.php'
SSH_OPTIONS=-B

DEBEMAIL="scorpio@qelectrotech.org"
DEBFULLNAME="Laurent Trinques"
export DEBEMAIL DEBFULLNAME

cd $DEFAULT_DIR
cd qet_git
# get updates
git submodule init
git submodule update
git pull --recurse-submodules
git pull

GITCOMMIT=$(git rev-parse --short HEAD)

#Count GIT commits revision number
if [ -z "$HEAD" ] ; then
  A=$(git rev-list HEAD --count)
#sum of GIT count + old Subversion count commits
  HEAD=$(($A+473))

fi

VERSION=$(cat sources/qetversion.cpp | grep "return QVersionNumber{"| head -n 1| awk -F "{" '{ print $2 }' | awk -F "}" '{ print $1 }' | sed -e 's/,/./g' -e 's/ //g')
#VERSION=$(cat sources/qetversion.cpp | grep "return QVersionNumber{ 0, "| head -n 1| cut -c32-40| sed -e 's/,/./g' -e 's/ //g')   #Find major, minor, and micro version numbers in sources/qetversion.cp


#clean /var/cache/pbuilder/ .. / result/debs
echo  -e "\033[1;31mWould you like to clean all debs n/Y?.\033[m"
read a
if [[ $a == "Y" || $a == "y" ]]; then
  echo -e "\033[1;31mClean_all_base_environment.\033[m"
  cd /var/cache/pbuilder/stable-amd64/result/ && sudo rm qe*
  cd /var/cache/pbuilder/unstable-amd64/result/ && sudo rm qe*
   else
  echo  -e "\033[1;33mNo Clean_all_base_environment.\033[m"
fi
#clean all apt cache
echo  -e "\033[1;31mWould you like to clean all pbuilder n/Y?.\033[m"
read a
if [[ $a == "Y" || $a == "y" ]]; then
  echo -e "\033[1;31mClean_all_pbuilder_environment.\033[m"
  DIST=stable ARCH=amd64 sudo pbuilder --clean
  DIST=unstable ARCH=amd64 sudo pbuilder --clean
else
  echo -e "\033[1;33mNo Clean_all_pbuilder_environment.\033[m"
fi

#Update des chroots
echo  -e "\033[1;31mWould you like to update all chroots n/Y?.\033[m"
read a
if [[ $a == "Y" || $a == "y" ]]; then
  echo  -e "\033[1;33mUpdate_all_base_environment..\033[m"
  sudo DIST=stable pbuilder update --override-config --configfile /home/laurent/.pbuilderrc
  sudo DIST=sid pbuilder update --override-config --configfile /home/laurent/.pbuilderrc
else
  echo -e "\033[1;33mNo Update_all_base_environment.\033[m"
fi

echo -e "\033[1;34mdownload source qelectrotech-$VERSION.r$HEAD.\033[m"

cd $TEMP_DIR

cp -r $DEFAULT_DIR/qet_git/ $TEMP_DIR/qelectrotech-$VERSION.r$HEAD/
rm -rf qelectrotech-$VERSION.r$HEAD/.git
#create GIT tarball
tar cfvz qelectrotech-$VERSION.r$HEAD.tar.gz qelectrotech-$VERSION.r$HEAD/

#copy tarball in $DEFAULT_DIR
cp qelectrotech-$VERSION.r$HEAD.tar.gz $DEFAULT_DIR

clean $TEMP_DIR
rm -Rf qelectrotech-$VERSION.r$HEAD/ && rm qelectrotech-$VERSION.r$HEAD.tar.gz

#return to working directory
cd $DEFAULT_DIR

#extract tarball and go to archive
tar zxvf qelectrotech-$VERSION.r$HEAD.tar.gz && cd qelectrotech-$VERSION.r$HEAD/

#create origin.tar.gz
dh_make -e  scorpio@qelectrotech.org -c gpl2  -f ../qelectrotech-$VERSION.r$HEAD.tar.gz

#clean debian folder
cd /
rm -Rf debian/

#return to working directory
cd $DEFAULT_DIR

#copy debian folder
cp -r debian/ qelectrotech-$VERSION.r$HEAD/

#update changelog to new release
cd qelectrotech-$VERSION.r$HEAD/
dch -d -D unstable "New upstream subversion version "
#edit changelog
echo -e "\033[1;31mWould you like to edit changelog n/Y?.\033[m"
read a
if [[ $a == "Y" || $a == "y" ]]; then
  echo -e "\033[1;31mEdit debian changelog..\033[m"
  dch --edit
else
  echo -e "\033[1;33mNo edit changelog.\033[m"
fi

cd debian/ && rm -rf *ex *EX README*

#deplacement dans le paquet_qet
cd $DEFAULT_DIR/qelectrotech-$VERSION.r$HEAD/
quilt push -a -f
quilt refresh

echo  -e "\033[1;31mWould you like to build Debian packages n/Y?.\033[m"
read a
if [[ $a == "Y" || $a == "y" ]]; then

  #build
  echo  -e "\033[1;34mBuild_all_environements.\033[m"
  sudo DIST=sid ARCH=amd64  pdebuild --configfile /home/laurent/.pbuilderrc
  dch -i -D stable  "Backport stable " &&
  sudo DIST=stable ARCH=amd64 pdebuild --configfile /home/laurent/.pbuilderrc
  STRING="Build_all_environements is ready!!!"
  echo $STRING

  #prepare download copys
  cd $DEFAULT_DIR
  mkdir -p $HEAD/stable/amd64
  mkdir -p $HEAD/sid/amd64

  #copie des debs des results vers $HEAD
  find /var/cache/pbuilder/stable-amd64/result -name "qe*" -exec cp {} $DEFAULT_DIR/$HEAD/stable/amd64 \;
  find /var/cache/pbuilder/unstable-amd64/result -name "qe*" -exec cp {} $DEFAULT_DIR/$HEAD/sid/amd64 \;

  #debsign
  echo -e "\033[1;31mWould you like to sign all packages n/Y?.\033[m"
  read a
  if [[ $a == "Y" || $a == "y" ]]; then

    #debsign stable amd64
    cd $DEFAULT_DIR/$HEAD/stable/amd64
    debsign -k D3439F4E2273F7DCD3228C9EBC9F825E1D4FB6C1 *.changes
    echo -e "\033[1;33mAll stable amd64 are sign.\033[m"

    #debsign sid amd64
    cd $DEFAULT_DIR/$HEAD/sid/amd64
    debsign -k D3439F4E2273F7DCD3228C9EBC9F825E1D4FB6C1 *.changes
    echo -e "\033[1;33mAll sid amd64 are sign.\033[m"

     else
    echo -e "\033[1;33mExit.\033[m"
  fi

  #upload vers incoming
  echo -e "\033[1;31mWould you like to upload to incoming n/Y?.\033[m"
  read a
  if [[ $a == "Y" || $a == "y" ]]; then
    echo -e "\033[1;33mUpload qelectrotech-$VERSION.r$HEAD.\033[m"

    cd ~/apt
    reprepro -vb . include unstable $DEFAULT_DIR/$HEAD/sid/amd64/*.changes
    reprepro -vb . include stable $DEFAULT_DIR/$HEAD/stable/amd64/*.changes
    reprepro -b . export

  else
    echo -e "\033[1;33mExit.\033[m"
  fi

  #rsync to TF
  echo -e "\033[1;31mWould you like to RSYNC Debian packages to TF n/Y?.\033[m"
  read a
  if [[ $a == "Y" || $a == "y" ]]; then
    echo -e "\033[1;33mRsync to TF qelectrotech-$VERSION.r$HEAD .\033[m"
    echo -e	"\033[1;31mrsync to TF password ssh and TF\033[m"
    cd $DEFAULT_DIR/script
    rsync -e ssh -av --delete-after --no-owner --no-g --chmod=g+w --progress --exclude-from=ExclusionRSync ~/apt/ server:download.qelectrotech.org/qet/qet-repository/debian/
    if [ $? != 0 ]; then
    {
      echo "RSYNC ERROR: problem syncing qelectrotech-$VERSION.r$HEAD "
      rsync -e ssh -av --delete-after --no-owner --no-g --chmod=g+w --progress --exclude-from=ExclusionRSync ~/apt/ server:download.qelectrotech.org/qet/qet-repository/debian/
    } fi

  else
    echo -e "\033[1;33mExit.\033[m"
  fi
else
  echo -e "\033[1;33mExit.\033[m"
fi

############# Ubuntu packages ################################################################################################

echo  -e "\033[1;31mWould you like to build Ubuntu PPA packages n/Y?.\033[m"
read a
if [[ $a == "Y" || $a == "y" ]]; then

#deplacement dans le paquet_qet
cd $DEFAULT_DIR/qelectrotech-$VERSION.r$HEAD/

dch -i -D bionic "Bionic Upload to ppa"
debuild --no-lintian -S -sa -kD3439F4E2273F7DCD3228C9EBC9F825E1D4FB6C1

dch -i -D focal "Focal Upload to ppa"
debuild --no-lintian -S -kD3439F4E2273F7DCD3228C9EBC9F825E1D4FB6C1

dch -i -D jammy "Jammyh Upload to ppa"
debuild --no-lintian -S -kD3439F4E2273F7DCD3228C9EBC9F825E1D4FB6C1

dch -i -D kinetic "Kinetic Upload to ppa"
debuild --no-lintian -S -kD3439F4E2273F7DCD3228C9EBC9F825E1D4FB6C1

cd $DEFAULT_DIR/

dput my-ppa-dev qelectrotech_$VERSION.r$HEAD-*ubuntu1_source.changes
dput my-ppa-dev qelectrotech_$VERSION.r$HEAD-*ubuntu2_source.changes
dput my-ppa-dev qelectrotech_$VERSION.r$HEAD-*ubuntu3_source.changes
dput my-ppa-dev qelectrotech_$VERSION.r$HEAD-*ubuntu4_source.changes


else
  echo  -e "\033[1;33mExit.\033[m"
fi


############# Windows packages ################################################################################################

#copy and update folders of nsis_base skeleton
echo  -e "\033[1;31mWould you like to build Window packages n/Y?.\033[m"
read a
if [[ $a == "Y" || $a == "y" ]]; then


# 64 bits
cd $DEFAULT_DIR
cp -r nsis_base nsis_base64$HEAD
cd qelectrotech-$VERSION.r$HEAD/
cp -r {elements,examples,titleblocks} /$DEFAULT_DIR/nsis_base64$HEAD/files
cp -r lang/*.qm  /$DEFAULT_DIR/nsis_base64$HEAD/files/lang

cd $DEFAULT_DIR
cd qelectrotech-$VERSION.r$HEAD/build-aux/windows/
cp {lang_extra.nsh,QET64.nsi,lang_extra_fr.nsh} /$DEFAULT_DIR/nsis_base64$HEAD/

#copy and update folders of  readytouse_base skeleton
cd $DEFAULT_DIR
cp -r readytouse_base qelectrotech-$VERSION+git$HEAD-x86-win64-readytouse
cd qelectrotech-$VERSION.r$HEAD/
cp -r {elements,examples,titleblocks} /$DEFAULT_DIR/qelectrotech-$VERSION+git$HEAD-x86-win64-readytouse
cp -r lang/*.qm  /$DEFAULT_DIR/qelectrotech-$VERSION+git$HEAD-x86-win64-readytouse/lang

export PATH=/home/laurent/digikam3333/project/bundles/mxe/build.win64/usr/bin:$PATH


#cross-compil 64 bits
cd $DEFAULT_DIR/qelectrotech-$VERSION.r$HEAD
#rm -r build/
mkdir build/ && cd build

/home/laurent/digikam3333/project/bundles/mxe/build.win64/usr/bin/x86_64-w64-mingw32.static-qmake-qt5 $DEFAULT_DIR/qelectrotech-$VERSION.r$HEAD/qelectrotech.pro


make -j$(nproc)

cd $DEFAULT_DIR/qelectrotech-$VERSION.r$HEAD/build/release/
cp qelectrotech.exe /$DEFAULT_DIR/nsis_base64$HEAD/files/bin
cp qelectrotech.exe /$DEFAULT_DIR/qelectrotech-$VERSION+git$HEAD-x86-win64-readytouse/bin

cd $DEFAULT_DIR/nsis_base64$HEAD
sed -i 's/'"0.5-dev_x86_64-win64+[0-9]*"'/'"$VERSION"_x86_64-win64+git"$HEAD"'/' QET64.nsi
makensis QET64.nsi

#################################################################################################################

#crompress readytouse
cd $DEFAULT_DIR
7za a -m0=lzma qelectrotech-$VERSION+git$HEAD-x86-win64-readytouse.7z qelectrotech-$VERSION+git$HEAD-x86-win64-readytouse/

echo  -e "\033[1;31mWould you like to prepare to RSYNC Windows packages /Y?.\033[m"
read a
if [[ $a == "Y" || $a == "y" ]]; then

cd /home/laurent/builds/nightly/

name=$(date +%Y-%m-%d-) #defined a name based on the date

#start increment to 1
i=1
#increment by i if the builds have already been created
while [ "$(ls /home/$USER/builds/nightly/ | grep $name$i)" != "" ]; do
	((i++))
done
#create a new folder incremented by 1
mkdir /home/$USER/builds/nightly/$name$i


cd $DEFAULT_DIR/
cp qelectrotech-$VERSION+git$HEAD-x86-win64-readytouse.7z /home/laurent/builds/nightly/$name$i

cd $DEFAULT_DIR/nsis_base64$HEAD
cp Installer_QElectroTech-$VERSION"_x86_64-win64+git"$HEAD-1.exe /home/laurent/builds/nightly/$name$i




else
  echo  -e "\033[1;33mExit.\033[m"
fi


#rsync to TF Windows builds
echo  -e "\033[1;31mWould you like to upload Windows packages n/Y?.\033[m"
read a
if [[ $a == "Y" || $a == "y" ]]; then
rsync -e ssh -av -W --delete-after --no-owner --no-g --chmod=g+w --progress  /home/laurent/builds/nightly/ server:download.qelectrotech.org/qet/qet-repository/builds/nightly/
if [ $? != 0 ]; then
{
echo "RSYNC ERROR: problem syncing qelectrotech-$VERSION.r$HEAD "
rsync -e ssh -av -W --delete-after --no-owner --no-g --chmod=g+w --progress  /home/laurent/builds/nightly/ server:download.qelectrotech.org/qet/qet-repository/builds/nightly/
} fi

else
  echo  -e "\033[1;33mExit.\033[m"
fi
else
  echo  -e "\033[1;33mExit.\033[m"
fi
exit

