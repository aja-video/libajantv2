#!/bin/sh

# build the lib with default options
#
# CMAKE_PREFIX_PATH is the recommended way to specify the Qt version see:
# https://doc.qt.io/qt-5/cmake-get-started.html
# https://doc.qt.io/qt-6/cmake-get-started.html

SELF_DIR="$( cd "$(dirname "$0")" || exit 1 ; pwd -P )"
ROOT_DIR="$( cd "$SELF_DIR/.." || exit ; pwd -P )"

# environment vars to control the build
if [ -z ${GENERATOR} ]; then GENERATOR="Unix Makefiles"; fi
if [ -z ${BUILD_DIR} ]; then BUILD_DIR="$ROOT_DIR/build"; fi

cmake -S"$ROOT_DIR" -B"$BUILD_DIR" -G"$GENERATOR"
if [ "$?" != 0 ]; then
    echo "problem running 'cmake ../"
    exit 3
fi
if [ "$GENERATOR" = "Unix Makefiles" ]; then
    cmake --build "$BUILD_DIR" -- -j$(nproc)
else
    cmake --build "$BUILD_DIR"
fi
if [ "$?" != 0 ]; then
    echo "problem running 'cmake --build ${BUILD_DIR}"
    exit 4
fi

printf "\nBUILD SUCCESS!\n"
