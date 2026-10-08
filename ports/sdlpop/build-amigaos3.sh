#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd); ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
SRC=${1:-"$ROOT/build/game-ports/sources/sdlpop"}; OUT=${2:-"$ROOT/build/os3/sdlpop"}
STOVE=${STOVE:-"$HOME/AmigaChrome/stoves/os32-gcc16"}; SYS="$STOVE/prefix/m68k-amigaos"; PATCH="$HERE/patches/0001-amigaos3-opengpu.patch"
if git -C "$SRC" apply --check "$PATCH" >/dev/null 2>&1; then git -C "$SRC" apply "$PATCH"; fi
rm -rf "$OUT"
cmake -S "$SRC/src" -B "$OUT" -DCMAKE_TOOLCHAIN_FILE="$ROOT/toolchain/amigaos3-opengpu.cmake" -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DSDL2="$SYS"
cmake --build "$OUT" --target prince -j2
file "$SRC/prince"; sha256sum "$SRC/prince"
