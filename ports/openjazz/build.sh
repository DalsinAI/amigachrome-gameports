#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
SRC=${1:?usage: build.sh /path/to/openjazz-source [build-dir]}
BUILD=${2:-"$ROOT/build/openjazz"}
: "${AROS_SYSROOT:?set AROS_SYSROOT to the AROS Developer sysroot}"
AROS_CC=${AROS_CC:-m68k-aros-gcc}
AROS_CXX=${AROS_CXX:-m68k-aros-g++}
CPU=${CPU:-68040}

ENV_DIR="$BUILD/amigachrome-cmake"
python3 "$ROOT/scripts/create_aros_cmake_env.py" --out "$ENV_DIR" --sysroot "$AROS_SYSROOT" --cc "$AROS_CC" --cxx "$AROS_CXX" --cpu "$CPU"
LIBGCC=$("$AROS_CXX" --sysroot="$AROS_SYSROOT" -m"$CPU" -print-libgcc-file-name)

cmake -S "$SRC" -B "$BUILD"   -DCMAKE_TOOLCHAIN_FILE="$ENV_DIR/aros-m68k.cmake"   -DCMAKE_BUILD_TYPE=Release   -DCMAKE_PREFIX_PATH="$AROS_SYSROOT"   -DSDL2_DIR="$ENV_DIR/sdl2"   -DNETWORK=OFF   -DSCALE=OFF   -DSDL_VERSION=2   -DCMAKE_EXE_LINKER_FLAGS="$LIBGCC"

cmake --build "$BUILD" --target OpenJazz -j2
echo "Built: $BUILD/OpenJazz"
