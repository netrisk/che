#!/bin/sh

SDL2_VER=2.0.4
BUILD_DIR=$PWD/windows-build
SDL2_DIR=$BUILD_DIR/SDL2-$SDL2_VER

# Set up some common variables
PREFIX=$BUILD_DIR/stage
TOOLSET=i586-mingw32msvc
# We make CC variable available to child processes
export CC="$TOOLSET-gcc -static-libgcc"

mkdir -p $BUILD_DIR
if ! test -e $BUILD_DIR/.sdl2_installed; then
	cd $BUILD_DIR
	wget https://www.libsdl.org/release/SDL2-${SDL2_VER}.tar.gz
	tar xzf SDL2-${SDL2_VER}.tar.gz
	cd $SDL2_DIR
	./configure --target=$TOOLSET --host=$TOOLSET --prefix=$PREFIX/$TOOLSET && make && make install && touch $BUILD_DIR/.sdl2_installed
fi

export PATH="$PREFIX/$TOOLSET/bin:$PATH"
# Configure build
./configure --target=$TOOLSET --host=$TOOLSET --prefix=$PREFIX/$TOOLSET
