#!/bin/sh
set -eu

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)

SRC=${1:-"$ROOT/build/game-ports/sources/cdogs-sdl"}
OUT=${2:-"$ROOT/build/os3/cdogs"}
STOVE=${STOVE:-"$HOME/AmigaChrome/stoves/os32-gcc16"}
SYS="$STOVE/prefix/m68k-amigaos"
PATCH="$HERE/patches/0001-amigaos3-opengpu.patch"

if git -C "$SRC" apply --check "$PATCH" >/dev/null 2>&1; then
    git -C "$SRC" apply "$PATCH"
fi

rm -rf "$OUT"

cmake -S "$SRC" -B "$OUT" \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT/toolchain/amigaos3-opengpu.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DAMIGA_OS3=ON \
    -DCDOGS_DATA_DIR=PROGDIR: \
    -DCMAKE_PREFIX_PATH="$SYS" \
    -DSDL2_DIR="$SYS/lib/cmake/SDL2" \
    -DSDL2_mixer_DIR="$SYS/lib/cmake/SDL2_mixer"

cmake --build "$OUT" --target cdogs-sdl -j2

file "$OUT/src/cdogs-sdl"
sha256sum "$OUT/src/cdogs-sdl"
