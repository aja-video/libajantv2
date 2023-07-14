#!/bin/sh

# designed to be run in a container so not using 'sudo'

QT_MAJ_MIN="5.15"
QT_VERSION="5.15.8"
SRC_URL="https://download.qt.io/official_releases/qt/$QT_MAJ_MIN/$QT_VERSION/single"
SRC_FILE="qt-everywhere-opensource-src-$QT_VERSION.tar.xz"
SRC_DIR="qt-everywhere-src-$QT_VERSION"
BUILD_DIR="build"
INSTALL_DIR="/opt/aja/Qt/$QT_VERSION/gcc_64"
NPROC=$(nproc)

if [ ! -f "$SRC_FILE" ]; then
    echo "$SRC_FILE does not exist locally, downloading"
    wget "$SRC_URL/$SRC_FILE"
fi

if [ ! -d "$SRC_DIR" ]; then
    printf "\n## Extracting '%s' to '%s'...\n" "$SRC_FILE" "$SRC_DIR"
    xzcat -T 0 $SRC_FILE | tar x
fi

if [ ! -d "$SRC_DIR" ]; then
    echo "error: can't find $SRC_DIR, either the download or extract failed"
    exit 1
fi

if [ -d "$BUILD_DIR" ]; then
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR" || exit 2

printf "\n## Configuring Qt %s...\n" "$QT_VERSION"
../$SRC_DIR/configure -v -prefix $INSTALL_DIR \
    -ccache -opensource -confirm-license -nomake examples -nomake tests \
	-qt-pcre -qt-zlib -qt-harfbuzz -qt-libjpeg -qt-libpng -qt-tiff -qt-webp \
	-bundled-xcb-xinput -xcb -no-icu -plugin-sql-sqlite \
	-skip qt3d -skip qtandroidextras -skip qtcanvas3d -skip qtcharts -skip qtconnectivity -skip qtdatavis3d \
	-skip qtlocation -skip qtlottie -skip qtnetworkauth -skip qtpurchasing -skip qtquick3d -skip qtremoteobjects \
	-skip qtsensors -skip qtserialbus -skip qtspeech -skip qtvirtualkeyboard -skip qtwebchannel -skip qtwebengine \
	-skip qtwebglplugin -skip qtwebview

if [ "$?" != 0 ]; then
    echo "error: problem with the 'configure' step"
    exit 3
    cd - || exit 99
fi

printf "\n## Building Qt %s...\n" "$QT_VERSION"
make -j "$NPROC"

if [ "$?" != 0 ]; then
    echo "error: problem with the 'make' step"
    exit 4
    cd - || exit 99
fi

printf "\n## Installing Qt %s...\n" "$QT_VERSION"
make install

if [ "$?" != 0 ]; then
    echo "error: problem with the 'make install' step"
    exit 5
    cd - || exit 99
fi

# NOTE: Uncomment below if you you want docs.
# printf "\n## Building Docs for Qt %s...\n" "$QT_VERSION"
# make -j "$NPROC" docs

# if [ "$?" != 0 ]; then
#     echo "error: problem with the 'make docs' step"
#     exit 6
#     cd - || exit 99
# fi

# printf "\n## Installing Docs for Qt %s...\n" "$QT_VERSION"
# make install_docs

# if [ "$?" != 0 ]; then
#     echo "error: problem with the 'make install_docs' step"
#     exit 7
#     cd - || exit 99
# fi

printf "\n## Copying config Summary and Options for Qt %s...\n" "$QT_VERSION"
cp config.summary config.opt "$INSTALL_DIR"

if [ "$?" != 0 ]; then
    echo "warning: could not copy config summary and opt to '$INSTALL_DIR'"
fi

printf "\n## Building of Qt %s was a success!\n" "$QT_VERSION"

cd - || exit 20

printf "\nMoving Qt to /opt...\n"
mv /opt/aja/Qt /opt/Qt$QT_VERSION

printf "\nCleaning up build dir and sources...\n"
rm -rf $SRC_FILE
rm -rf $SRC_DIR
rm -rf $BUILD_DIR

echo "\ngoodbye! -_~\n"
