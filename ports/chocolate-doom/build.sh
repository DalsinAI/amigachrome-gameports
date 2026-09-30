#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
SRC=${1:?usage: build.sh /path/to/chocolate-doom-source [build-dir]}
BUILD=${2:-"$ROOT/build/chocolate-doom"}
: "${AROS_SYSROOT:?set AROS_SYSROOT to the AROS Developer sysroot}"
AROS_CC=${AROS_CC:-m68k-aros-gcc}
AROS_CXX=${AROS_CXX:-m68k-aros-g++}
CPU=${CPU:-68040}

ENV_DIR="$BUILD/amigachrome-cmake"
python3 "$ROOT/scripts/create_aros_cmake_env.py" --out "$ENV_DIR" --sysroot "$AROS_SYSROOT" --cc "$AROS_CC" --cxx "$AROS_CXX" --cpu "$CPU"

cmake -S "$SRC" -B "$BUILD"   -DCMAKE_TOOLCHAIN_FILE="$ENV_DIR/aros-m68k.cmake"   -DCMAKE_BUILD_TYPE=Release   -DCMAKE_PREFIX_PATH="$AROS_SYSROOT"   -DSDL2_DIR="$ENV_DIR/sdl2"   -DSDL2_mixer_DIR="$ENV_DIR/sdl2-mixer"   -DENABLE_SDL2_NET=OFF   -DENABLE_SDL2_MIXER=ON

cmake --build "$BUILD" --target chocolate-doom -j2
echo "Built: $BUILD/src/chocolate-doom"
