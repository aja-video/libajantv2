#!/bin/zsh

CHECKOUT_DIR="$(git rev-parse --show-toplevel)"

# environment vars to control the build
if [ -z "${GENERATOR}" ];     then GENERATOR="Ninja"; fi
if [ -z "${BUILD_TYPE}" ];    then BUILD_TYPE="Release"; fi
if [ -z "${BUILD_DIR}" ];     then BUILD_DIR="$CHECKOUT_DIR/build"; fi
if [ -z "${INSTALL_DIR}" ];   then INSTALL_DIR="$CHECKOUT_DIR/install"; fi
if [ -z "${OSX_ARCHITECTURE}" ]; then OSX_ARCHITECTURE="arm64"; fi

# AJA-specific CMake Options
if [ -z "${AJA_BUILD_OPENSOURCE}" ]; then AJA_BUILD_OPENSOURCE="ON"; fi
if [ -z "${AJA_BUILD_DOCS}" ]; then AJA_BUILD_DOCS="OFF"; fi
if [ -z "${AJA_INSTALL_SOURCES}" ]; then AJA_INSTALL_SOURCES="OFF"; fi
if [ -z "${AJA_QT_ENABLED}" ]; then AJA_QT_ENABLED="OFF"; fi
if [ -z "${AJA_QT_DEPLOY}" ]; then AJA_QT_DEPLOY="OFF"; fi

echo Configured Options:
echo -------------------
echo CHECKOUT_DIR: $CHECKOUT_DIR
echo GENERATOR:   $GENERATOR
echo BUILD_TYPE:  $BUILD_TYPE
echo PREFIX_PATH: $PREFIX_PATH
echo BUILD_DIR:   $BUILD_DIR
echo INSTALL_DIR: $INSTALL_DIR
echo OSX_ARCHITECTURE: $OSX_ARCHITECTURE
echo -------------------
echo AJA_BUILD_OPENSOURCE: $AJA_BUILD_OPENSOURCE
echo AJA_BUILD_DOCS: $AJA_BUILD_DOCS
echo AJA_INSTALL_SOURCES: $AJA_INSTALL_SOURCES
echo AJA_QT_ENABLED: $AJA_QT_ENABLED
echo AJA_QT_DEPLOY: $AJA_QT_DEPLOY
echo -------------------

# Generate makefiles/ninja files
echo "Generating build"
/usr/bin/arch -$OSX_ARCHITECTURE /bin/zsh -c "cmake -S"$CHECKOUT_DIR" -B"$BUILD_DIR" -G"$GENERATOR" \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
        -DCMAKE_PREFIX_PATH=$PREFIX_PATH \
        -DCMAKE_OSX_ARCHITECTURES=$OSX_ARCHITECTURE \
        -DAJANTV2_BUILD_OPENSOURCE=$AJA_BUILD_OPENSOURCE \
        -DAJANTV2_BUILD_DOCS=$AJA_BUILD_DOCS \
        -DAJA_INSTALL_SOURCES=$AJA_INSTALL_SOURCES \
        -DAJA_QT_ENABLED=$AJA_QT_ENABLED \
        -DAJA_QT_DEPLOY=$AJA_QT_DEPLOY"

if [ "$?" != 0 ]; then
    echo "problem running 'cmake ../"
    exit 1
fi

# Build project
echo "Building Retail"
/usr/bin/arch -$OSX_ARCHITECTURE /bin/zsh -c "cmake --build "$BUILD_DIR""
if [ "$?" != 0 ]; then
    echo "problem running 'cmake --build ${BUILD_DIR}"
    exit 1
fi

# Install project
echo "Installing target"
cmake --install "$BUILD_DIR"
if [ "$?" != 0 ]; then
    echo "problem running 'cmake --install ${BUILD_DIR}"
    exit 1
fi

