#!/bin/sh

# build the lib with default options
#
# CMAKE_PREFIX_PATH is the recommended way to specify the Qt version see:
# https://doc.qt.io/qt-5/cmake-get-started.html
# https://doc.qt.io/qt-6/cmake-get-started.html

SELF_DIR="$( cd "$(dirname "$0")" || exit 1 ; pwd -P )"
CHECKOUT_DIR="$( cd "$SELF_DIR/.." || exit ; pwd -P )"

# environment vars to control the build
if [ -z ${GENERATOR} ]; then GENERATOR="Unix Makefiles"; fi
if [ -z ${BUILD_TYPE} ]; then BUILD_TYPE="Release"; fi
if [ -z ${BUILD_DIR} ]; then BUILD_DIR="$CHECKOUT_DIR/build"; fi
if [ -z ${INSTALL_DIR} ]; then INSTALL_DIR="$CHECKOUT_DIR/install"; fi
if [ -z ${BUILD_SHARED} ]; then BUILD_SHARED=OFF; fi
if [ -z ${BUILD_OPENSOURCE} ]; then BUILD_OPENSOURCE=ON; fi
if [ -z ${DISABLE_DRIVER} ]; then DISABLE_DRIVER=OFF; fi
if [ -z ${DISABLE_DEMOS} ]; then DISABLE_DEMOS=OFF; fi
if [ -z ${DISABLE_TESTS} ]; then DISABLE_TESTS=OFF; fi
if [ -z ${DISABLE_TOOLS} ]; then DISABLE_TOOLS=OFF; fi
if [ -z ${DISABLE_PLUGINS} ]; then DISABLE_PLUGINS=OFF; fi
if [ -z ${INSTALL_HEADERS} ]; then INSTALL_HEADERS=ON; fi
if [ -z ${INSTALL_SOURCES} ]; then INSTALL_SOURCES=ON; fi
if [ -z ${INSTALL_CMAKE} ]; then INSTALL_CMAKE=ON; fi
if [ -z ${INSTALL_MISC} ]; then INSTALL_MISC=ON; fi
if [ -z ${QT_ENABLED} ]; then QT_ENABLED=ON; fi
if [ -z ${QT_DEPLOY} ]; then QT_DEPLOY=ON; fi

echo Configured Options:
echo -------------------
echo CHECKOUT_DIR: $CHECKOUT_DIR
echo GENERATOR: $GENERATOR
echo BUILD_TYPE: $BUILD_TYPE
echo PREFIX_PATH: $PREFIX_PATH
echo BUILD_DIR: $BUILD_DIR
echo INSTALL_DIR: $INSTALL_DIR
echo BUILD_SHARED: $BUILD_SHARED
echo BUILD_OPENSOURCE: $BUILD_OPENSOURCE
echo DISABLE_DRIVER: $DISABLE_DRIVER
echo DISABLE_DEMOS: $DISABLE_DEMOS
echo DISABLE_TESTS: $DISABLE_TESTS
echo DISABLE_TOOLS: $DISABLE_TOOLS
echo DISABLE_PLUGINS: $DISABLE_PLUGINS
echo INSTALL_HEADERS: $INSTALL_HEADERS
echo INSTALL_SOURCES: $INSTALL_SOURCES
echo INSTALL_CMAKE: $INSTALL_CMAKE
echo INSTALL_MISC: $INSTALL_MISC
echo QT_ENABLED: $QT_ENABLED
echo QT_DEPLOY: $QT_DEPLOY
echo -------------------

echo "Removing old build/install directories"
if [ -d $BUILD_DIR ]; then
    rm -rf $BUILD_DIR
fi
if [ -d $INSTALL_DIR]; then 
    rm -rf $INSTALL_DIR
fi

echo "Generating build"
cmake -S"$CHECKOUT_DIR" -B"$BUILD_DIR" -G"$GENERATOR" \
          -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
          -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
          -DCMAKE_PREFIX_PATH="$PREFIX_PATH" \
          -DAJA_BUILD_SHARED="$BUILD_SHARED" \
          -DAJANTV2_BUILD_OPENSOURCE="$BUILD_OPENSOURCE" \
          -DAJA_DISABLE_DRIVER="$DISABLE_DRIVER" \
          -DAJA_DISABLE_DEMOS="$DISABLE_DEMOS" \
          -DAJA_DISABLE_TESTS="$DISABLE_TESTS" \
          -DAJA_DISABLE_TOOLS="$DISABLE_TOOLS" \
          -DAJA_DISABLE_PLUGINS="$DISABLE_PLUGINS" \
          -DAJA_INSTALL_HEADERS="$INSTALL_HEADERS" \
          -DAJA_INSTALL_SOURCES="$INSTALL_SOURCES" \
          -DAJA_INSTALL_CMAKE="$INSTALL_CMAKE" \
          -DAJA_INSTALL_MISC="$INSTALL_MISC" \
          -DAJA_QT_ENABLED="$QT_ENABLED" \
          -DAJA_QT_DEPLOY="$QT_DEPLOY"

if [ "$?" != 0 ]; then
    echo "Error generating makefiles/ninja files"
    exit 1
fi

echo "Building all targets"
if [ "$GENERATOR" = "Unix Makefiles" ]; then
    cmake --build "$BUILD_DIR" -- -j$(nproc)
else
    cmake --build "$BUILD_DIR"
fi
if [ "$?" != 0 ]; then
    echo "Error building all targets"
    exit 1
fi

echo "Installing all targets"
cmake --install "$BUILD_DIR"
if [ "$?" != 0 ]; then
    echo "Error installing all targets"
    exit 1
fi
