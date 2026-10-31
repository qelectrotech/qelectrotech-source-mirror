#!/bin/bash
#
# Build the QElectroTech AppImage (x86_64) with Qt6 and CMake.
#
# Run it from the root of qelectrotech-source-mirror, as a normal user.
# linuxdeployqt does not support Qt6: linuxdeploy and its Qt plugin are
# used instead, and appimagetool makes the final AppImage.
#
# Qt : official binaries (aqtinstall), e.g.
#   aqt install-qt linux desktop 6.11.2 linux_gcc_64 -m qtpdf qtimageformats -O ~/Qt
# or any Qt6 whose qmake is given in QMAKE below.

set -e
set -x

#################################################################################################
# Configuration

QT_VERSION=6.11.2
QMAKE=${QMAKE:-$HOME/Qt/$QT_VERSION/gcc_64/bin/qmake}

# KDE Frameworks built statically against this Qt, once (see below)
KF_VERSION=v6.10.0
KF_PREFIX=${KF_PREFIX:-$HOME/Qt/kf6-$KF_VERSION-qt$QT_VERSION}

SOURCE_DIR=$(pwd)
TOOLS_DIR=$SOURCE_DIR/appdir                         # linuxdeploy, appimagetool
APPDIR=$SOURCE_DIR/appdir/qelectrotech               # AppDir assembled here
OUTPUT_DIR=$SOURCE_DIR/AppImage                      # AppImages, one folder per version

UPLOAD_HOST=server
UPLOAD_PATH=download.qelectrotech.org/qet/builds/AppImage

if [ ! -f sources/qetversion.cpp ]; then
    echo "Run this script from the root of qelectrotech-source-mirror."
    exit 1
fi

if [ ! -x "$QMAKE" ]; then
    echo "qmake not found: $QMAKE (set QMAKE=/path/to/Qt6/bin/qmake)."
    exit 1
fi

QT_PREFIX=$($QMAKE -query QT_INSTALL_PREFIX)
QT_LIBDIR=$($QMAKE -query QT_INSTALL_LIBS)

#################################################################################################
# Update the sources

git reset --hard
git pull
git submodule update --init --recursive

GITCOMMIT=$(git rev-parse --short HEAD)
A=$(git rev-list HEAD --count)
HEAD=$(($A+473))

# Find major, minor, and micro version numbers in sources/qetversion.cpp
tagName=$(grep "return QVersionNumber{" sources/qetversion.cpp | head -n 1 \
          | awk -F "{" '{ print $2 }' | awk -F "}" '{ print $1 }' | sed -e 's/,/./g' -e 's/ //g')

APPIMAGE=QElectroTech_$tagName-r$HEAD-x86_64.AppImage

#################################################################################################
# Build and install into the AppDir

# QET's own BUILD_KF=ON (FetchContent) does not work: kcoreaddons cannot find
# the fetched ECM. ECM, KCoreAddons and KWidgetsAddons are built apart instead.
if [ ! -f "$KF_PREFIX/.built" ]; then
    KF_BUILD=$(mktemp -d)
    for MODULE in extra-cmake-modules kcoreaddons kwidgetsaddons; do
        git clone --depth 1 --branch "$KF_VERSION" \
            "https://invent.kde.org/frameworks/$MODULE.git" "$KF_BUILD/$MODULE"
        cmake -S "$KF_BUILD/$MODULE" -B "$KF_BUILD/$MODULE/build" -G Ninja \
              -DCMAKE_BUILD_TYPE=Release \
              -DCMAKE_INSTALL_PREFIX="$KF_PREFIX" \
              -DCMAKE_PREFIX_PATH="$QT_PREFIX;$KF_PREFIX" \
              -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
              -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF -DBUILD_QCH=OFF \
              -DBUILD_DESIGNERPLUGIN=OFF -DBUILD_PYTHON_BINDINGS=OFF \
              -DKCOREADDONS_USE_QML=OFF -DKDE_SKIP_TEST_SETTINGS=ON -Wno-dev
        cmake --build "$KF_BUILD/$MODULE/build" -j"$(nproc)"
        cmake --install "$KF_BUILD/$MODULE/build"
    done
    rm -Rf "$KF_BUILD"
    touch "$KF_PREFIX/.built"
fi

rm -Rf build/
cmake -S . -B build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr \
      -DCMAKE_PREFIX_PATH="$QT_PREFIX;$KF_PREFIX" \
      -DPACKAGE_TESTS=OFF \
      -DBUILD_TESTING=OFF \
      -DQET_EXPORT_PROJECT_DB=ON \
      -DBUILD_WITH_KF=ON \
      -DBUILD_KF=OFF
cmake --build build -j"$(nproc)"

rm -Rf "$APPDIR"
DESTDIR="$APPDIR" cmake --install build

# Developer files, in case something installs them
rm -Rf "$APPDIR"/usr/include "$APPDIR"/usr/lib/cmake "$APPDIR"/usr/lib/pkgconfig \
       "$APPDIR"/usr/lib/*.a "$APPDIR"/usr/lib/x86_64-linux-gnu "$APPDIR"/usr/share/ECM \
       "$APPDIR"/usr/mkspecs "$APPDIR"/usr/share/qlogging-categories6

#################################################################################################
# AppRun : QET is built with absolute /usr/share/qelectrotech paths, give it
# the collections, title blocks and translations inside the AppImage.

cat > "$TOOLS_DIR/AppRun" << 'EOF'
#!/bin/bash
DIR="$(dirname "$(readlink -f "$0")")"
QET_DATA="$DIR/usr/share/qelectrotech"
exec "$DIR/usr/bin/qelectrotech" \
     "--common-elements-dir=$QET_DATA/elements/" \
     "--common-tbt-dir=$QET_DATA/titleblocks/" \
     "--lang-dir=$QET_DATA/lang/" \
     "$@"
EOF
chmod +x "$TOOLS_DIR/AppRun"

#################################################################################################
# Deploy Qt6 and the libraries with linuxdeploy

cd "$TOOLS_DIR"

wget -q -N https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
wget -q -N https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
wget -q -N https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage
chmod +x linuxdeploy-x86_64.AppImage linuxdeploy-plugin-qt-x86_64.AppImage appimagetool-x86_64.AppImage

export QMAKE
export LD_LIBRARY_PATH="$QT_LIBDIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export APPIMAGE_EXTRACT_AND_RUN=1                    # tools run without FUSE
export EXTRA_PLATFORM_PLUGINS="libqwayland-generic.so;libqwayland-egl.so"

./linuxdeploy-x86_64.AppImage \
    --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/qelectrotech" \
    --desktop-file "$APPDIR/usr/share/applications/org.qelectrotech.qelectrotech.desktop" \
    --icon-file "$APPDIR/usr/share/icons/hicolor/256x256/apps/qelectrotech.png" \
    --custom-apprun "$TOOLS_DIR/AppRun" \
    --plugin qt

# SQLite driver only: the other drivers need client libraries we do not ship
find "$APPDIR/usr/plugins/sqldrivers" -type f ! -name "libqsqlite.so" -delete

# AppStream metadata, named after the desktop file id
if [ -f "$APPDIR/usr/share/appdata/qelectrotech.appdata.xml" ]; then
    mkdir -p "$APPDIR/usr/share/metainfo"
    mv "$APPDIR/usr/share/appdata/qelectrotech.appdata.xml" \
       "$APPDIR/usr/share/metainfo/org.qelectrotech.qelectrotech.appdata.xml"
    rmdir "$APPDIR/usr/share/appdata" 2>/dev/null || true
fi

#################################################################################################
# Make the AppImage

sed -i 's/'"QElectroTech_*.*/QElectroTech_$tagName-r$HEAD"'/' "$APPDIR/org.qelectrotech.qelectrotech.desktop"

rm -f QElectroTech_*.AppImage
ARCH=x86_64 ./appimagetool-x86_64.AppImage --no-appstream "$APPDIR" "$APPIMAGE"
chmod +x "$APPIMAGE"
sha256sum "$APPIMAGE" > "$APPIMAGE-SHA256.txt"

mkdir -p "$OUTPUT_DIR/$tagName"
mv "$APPIMAGE"* "$OUTPUT_DIR/$tagName/"
cd "$SOURCE_DIR"

set +x

#################################################################################################
# rsync to server

echo -e "\033[1;31mWould you like to RSYNC AppImage to server n/Y?.\033[m"
read a
if [[ $a == "Y" || $a == "y" ]]; then
    echo -e "\033[1;33mRsync to server QElectroTech_$tagName-r$HEAD .\033[m"
    if ! rsync -e ssh -av --delete-after --no-owner --no-g --chmod=g+w --progress \
               "$OUTPUT_DIR/$tagName/" "$UPLOAD_HOST:$UPLOAD_PATH/$tagName/"; then
        echo "RSYNC ERROR: problem syncing QElectroTech_$tagName-r$HEAD, retrying"
        rsync -e ssh -av --delete-after --no-owner --no-g --chmod=g+w --progress \
              "$OUTPUT_DIR/$tagName/" "$UPLOAD_HOST:$UPLOAD_PATH/$tagName/"
    fi
else
    echo -e "\033[1;33mExit.\033[m"
fi
exit
