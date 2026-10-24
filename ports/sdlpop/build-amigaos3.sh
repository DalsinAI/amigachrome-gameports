#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd); ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
SRC=${1:-"$ROOT/build/game-ports/sources/sdlpop"}; OUT=${2:-"$ROOT/build/os3/sdlpop"}
: "${STOVE:?set STOVE to an isolated 16.2.0-open3 stove root}"; SYS="$STOVE/prefix/m68k-amigaos"; PATCH="$HERE/patches/0001-amigaos3-opengpu.patch"
python3 "$ROOT/scripts/verify_gameport_stove.py" --stove "$STOVE" >/dev/null
if git -C "$SRC" apply --check "$PATCH" >/dev/null 2>&1; then git -C "$SRC" apply "$PATCH"; fi
rm -rf "$OUT"
cmake -S "$SRC/src" -B "$OUT" -DCMAKE_TOOLCHAIN_FILE="$ROOT/toolchain/amigaos3-opengpu.cmake" -DAMIGA_STOVE="$STOVE" -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DSDL2="$SYS"
cmake --build "$OUT" --target prince -j2
file "$SRC/prince"; sha256sum "$SRC/prince"
