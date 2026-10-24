#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd); ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
SRC=${1:-"$ROOT/build/game-ports/sources/chocolate-doom"}; OUT=${2:-"$ROOT/build/os3/chocolate-doom"}
: "${STOVE:?set STOVE to an isolated 16.2.0-open3 stove root}"; P="$STOVE/prefix"; SYS="$P/m68k-amigaos"
python3 "$ROOT/scripts/verify_gameport_stove.py" --stove "$STOVE" >/dev/null
M=$($P/bin/m68k-amigaos-gcc -m68040 -m68881 -noixemul -print-file-name=libm.a)
rm -rf "$OUT"
cmake -S "$SRC" -B "$OUT" -DCMAKE_TOOLCHAIN_FILE="$ROOT/toolchain/amigaos3-opengpu.cmake" -DAMIGA_STOVE="$STOVE" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$SYS" -DSDL2_DIR="$SYS/lib/cmake/SDL2" -DSDL2_mixer_DIR="$SYS/lib/cmake/SDL2_mixer" -DENABLE_SDL2_NET=OFF -DENABLE_SDL2_MIXER=ON -DM_LIBRARY="$M"
cmake --build "$OUT" --target chocolate-doom -j2
file "$OUT/src/chocolate-doom"; sha256sum "$OUT/src/chocolate-doom"
