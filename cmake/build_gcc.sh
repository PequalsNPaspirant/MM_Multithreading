#!/usr/bin/env bash
set -eu

arg=${1:-default}
BUILD_DIR="`pwd`/build_gcc"
echo "BUILD_DIR: $BUILD_DIR"

if [ -d "$BUILD_DIR" ]; then
	if [[ "$arg" == "clean" ]]; then
		echo "Cleaning build directory $BUILD_DIR. Press any key to continue..."
		read keyPress
		rm -rf "$BUILD_DIR"
	fi
else
	mkdir "$BUILD_DIR"
fi

cmake -H. -B"$BUILD_DIR" 

cmake --build "$BUILD_DIR"
