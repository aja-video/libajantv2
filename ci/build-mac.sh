#!/bin/zsh

CHECKOUT_DIR="$(git rev-parse --show-toplevel)"

# environment vars to control the build
if [ -z "${GENERATOR}" ];     then GENERATOR="Ninja"; fi
if [ -z "${BUILD_TYPE}" ];    then BUILD_TYPE="Release"; fi
if [ -z "${BUILD_DIR}" ];     then BUILD_DIR="$CHECKOUT_DIR/build"; fi
if [ -z "${INSTALL_DIR}" ];   then INSTALL_DIR="$CHECKOUT_DIR/install"; fi
if [ -z "${OSX_ARCHITECTURE}" ]; then OSX_ARCHITECTURE="arm64"; fi

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

# Generate makefiles/ninja files
echo "Generating build"
/usr/bin/arch -$OSX_ARCHITECTURE /bin/zsh -c "cmake -S"$CHECKOUT_DIR" -B"$BUILD_DIR" -G"$GENERATOR" \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
        -DCMAKE_PREFIX_PATH=$PREFIX_PATH \
        -DCMAKE_OSX_ARCHITECTURES=$OSX_ARCHITECTURE \
        -DAJANTV2_BUILD_OPENSOURCE=OFF \
        -DAJA_QT_ENABLED=ON \
        -DAJA_QT_DEPLOY=ON"

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

