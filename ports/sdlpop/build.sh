#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
SRC=${1:?usage: build.sh /path/to/SDLPoP-source [build-dir]}
BUILD=${2:-"$ROOT/build/sdlpop"}
: "${AROS_SYSROOT:?set AROS_SYSROOT to the AROS Developer sysroot}"
AROS_CC=${AROS_CC:-m68k-aros-gcc}
AROS_CXX=${AROS_CXX:-m68k-aros-g++}
CPU=${CPU:-68040}

ENV_DIR="$BUILD/amigachrome-cmake"
python3 "$ROOT/scripts/create_aros_cmake_env.py" --out "$ENV_DIR" --sysroot "$AROS_SYSROOT" --cc "$AROS_CC" --cxx "$AROS_CXX" --cpu "$CPU"

cmake -S "$SRC/src" -B "$BUILD"   -DCMAKE_TOOLCHAIN_FILE="$ENV_DIR/aros-m68k.cmake"   -DCMAKE_BUILD_TYPE=Release   -DCMAKE_POLICY_VERSION_MINIMUM=3.5   -DSDL2="$AROS_SYSROOT"   -DCMAKE_C_FLAGS="--sysroot=$AROS_SYSROOT -I$AROS_SYSROOT/include -Dalloca=__builtin_alloca -m$CPU"

cmake --build "$BUILD" --target prince -j2
echo "Built SDLPoP prince executable."
