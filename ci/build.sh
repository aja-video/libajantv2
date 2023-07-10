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
if [ -z ${BUILD_DRIVER} ]; then BUILD_DRIVER=ON; fi
if [ -z ${BUILD_DOCS} ]; then BUILD_DOCS=OFF; fi
if [ -z ${BUILD_DEMOS} ]; then BUILD_DEMOS=ON; fi
if [ -z ${BUILD_TESTS} ]; then BUILD_TESTS=ON; fi
if [ -z ${BUILD_TOOLS} ]; then BUILD_TOOLS=ON; fi
if [ -z ${BUILD_PLUGINS} ]; then BUILD_PLUGINS=ON; fi
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
echo BUILD_DRIVER: $BUILD_DRIVER
echo BUILD_DOCS: $BUILD_DOCS
echo BUILD_DEMOS: $BUILD_DEMOS
echo BUILD_TESTS: $BUILD_TESTS
echo BUILD_TOOLS: $BUILD_TOOLS
echo BUILD_PLUGINS: $BUILD_PLUGINS
echo INSTALL_HEADERS: $INSTALL_HEADERS
echo INSTALL_SOURCES: $INSTALL_SOURCES
echo INSTALL_CMAKE: $INSTALL_CMAKE
echo INSTALL_MISC: $INSTALL_MISC
echo QT_ENABLED: $QT_ENABLED
echo QT_DEPLOY: $QT_DEPLOY
echo -------------------

echo "Generating build"
cmake -S"$CHECKOUT_DIR" -B"$BUILD_DIR" -G"$GENERATOR" \
          -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
          -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
          -DCMAKE_PREFIX_PATH="$PREFIX_PATH" \
          -DAJA_BUILD_SHARED="$BUILD_SHARED" \
          -DAJANTV2_BUILD_OPENSOURCE="$BUILD_OPENSOURCE" \
          -DAJANTV2_BUILD_DRIVER="$BUILD_DRIVER" \
          -DAJANTV2_BUILD_DEMOS="$BUILD_DEMOS" \
          -DAJA_BUILD_DOCS="$BUILD_DOCS" \
          -DAJA_BUILD_TESTS="$BUILD_TESTS" \
          -DAJA_BUILD_TOOLS="$BUILD_TOOLS" \
          -DAJA_BUILD_PLUGINS="$BUILD_PLUGINS" \
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
