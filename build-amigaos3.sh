#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 AmigaChrome Team
#
# fheroes2 for AmigaOS 3.x, 68040, soft float (one binary runs with or without
# an FPU), on OpenRTG's SDL2/SDL2_mixer modules and the system's z.library.
#
#   bash build-amigaos3.sh SDK_DIR SOURCE_CHECKOUT OUT_DIR
#
# SDK_DIR is build_sdk.sh's output. SOURCE_CHECKOUT is a git clone of
# ihhub/fheroes2 that has the pinned commit (source.json); the build uses a
# clean export of that commit with patches/ applied, offline.
set -euo pipefail
[[ $# == 3 ]] || { echo "Usage: $0 SDK_DIR SOURCE_CHECKOUT OUT_DIR" >&2; exit 2; }
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$HERE
SDK=$(cd "$1" && pwd)
SRC=$(cd "$2" && pwd)
[[ ! -e "$3" ]] || { echo "Use a new OUT_DIR" >&2; exit 2; }
mkdir -p "$3"
OUT=$(cd "$3" && pwd)
JOBS=${JOBS:-$(nproc)}
# shellcheck source=/dev/null
source "$SDK/sdk.env"
PIN=$(python3 -c "import json,sys; print(json.load(open(sys.argv[1]))['commit'])" "$HERE/source.json")
CPU="-m68040 -specs=$OPENSDK/Lib/m68k-soft-m68040.specs"
CC=$PREFIX/bin/m68k-amigaos-gcc
CXX=$PREFIX/bin/m68k-amigaos-g++

# 1. The pinned source, exported clean, with the Amiga patches.
git -C "$SRC" cat-file -e "$PIN^{commit}"
EPOCH=$(git -C "$SRC" show -s --format=%ct "$PIN")
TREE=$OUT/source
mkdir -p "$TREE"
git -C "$SRC" archive "$PIN" | tar -x -C "$TREE"
for p in "$HERE"/patches/*.patch; do
    patch -s --batch --forward --fuzz=0 -p1 -d "$TREE" < "$p"
done

# 2. The port's own pieces: zlib.h over z.library, and libnix's missing statvfs.
mkdir -p "$OUT/port/include" "$OUT/port/lib"
cp "$HERE/zlib/zlib.h" "$HERE/zlib/zconf.h" "$OUT/port/include/"
$CC $CPU -noixemul -O2 -Wall -Wextra -c "$HERE/zlib/zlib_zlibrary.c" -I"$HERE/zlib" -o "$OUT/port/zlib_zlibrary.o"
$CXX $CPU -noixemul -O2 -Wall -Wextra -c "$HERE/compat/statvfs.cpp" -o "$OUT/port/statvfs.o"
"$PREFIX/bin/m68k-amigaos-ar" rcs "$OUT/port/lib/libz.a" "$OUT/port/zlib_zlibrary.o"
"$PREFIX/bin/m68k-amigaos-ar" rcs "$OUT/port/lib/libamigacompat.a" "$OUT/port/statvfs.o"

# 3. CMake package files for SDL2 and SDL2_mixer: OpenRTG's link stubs, which
#    load SDL2.module and SDL2_mixer.module at run time.
mkdir -p "$OUT/cmake/SDL2" "$OUT/cmake/SDL2_mixer"
cat > "$OUT/cmake/SDL2/SDL2Config.cmake" <<EOF
if(NOT TARGET SDL2::SDL2)
  add_library(SDL2::SDL2 STATIC IMPORTED)
  set_target_properties(SDL2::SDL2 PROPERTIES
    IMPORTED_LOCATION "$SYSROOT/lib/libSDL2.a"
    INTERFACE_INCLUDE_DIRECTORIES "$SYSROOT/include/SDL2")
  # AmigaOS needs no SDL_main: the program's own main() is the entry point.
  add_library(SDL2::SDL2main INTERFACE IMPORTED)
endif()
set(SDL2_FOUND TRUE)
EOF
cat > "$OUT/cmake/SDL2_mixer/SDL2_mixerConfig.cmake" <<EOF
if(NOT TARGET SDL2_mixer::SDL2_mixer)
  add_library(SDL2_mixer::SDL2_mixer STATIC IMPORTED)
  set_target_properties(SDL2_mixer::SDL2_mixer PROPERTIES
    IMPORTED_LOCATION "$SYSROOT/lib/libSDL2_mixer.a"
    INTERFACE_INCLUDE_DIRECTORIES "$SYSROOT/include/SDL2"
    INTERFACE_LINK_LIBRARIES SDL2::SDL2)
endif()
set(SDL2_mixer_FOUND TRUE)
EOF

# 4. Configure and build the game only (the translations need host gettext
#    and are built separately).
SOURCE_DATE_EPOCH=$EPOCH cmake -S "$TREE" -B "$OUT/build" -G "Unix Makefiles" \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT/toolchain/amigaos3-opengpu.cmake" \
    -DAMIGA_STOVE="$SDK" -DAMIGA_CPU_FLAGS="$CPU" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS_RELEASE="-O2 -DNDEBUG" \
    -DCMAKE_C_FLAGS="-fno-delete-null-pointer-checks -ffile-prefix-map=$TREE=." \
    -DCMAKE_CXX_FLAGS="-fno-delete-null-pointer-checks -ffile-prefix-map=$TREE=." \
    -DCMAKE_CXX_STANDARD_LIBRARIES="$OUT/port/statvfs.o -latomic" \
    -DSDL2_DIR="$OUT/cmake/SDL2" -DSDL2_mixer_DIR="$OUT/cmake/SDL2_mixer" \
    -DZLIB_INCLUDE_DIR="$OUT/port/include" -DZLIB_LIBRARY="$OUT/port/lib/libz.a" \
    -DTHREADS_PREFER_PTHREAD_FLAG=ON -DCMAKE_INSTALL_PREFIX=/PROGDIR \
    > "$OUT/configure.log" 2>&1 || { tail -40 "$OUT/configure.log"; exit 4; }
SOURCE_DATE_EPOCH=$EPOCH cmake --build "$OUT/build" --target fheroes2 -j"$JOBS" > "$OUT/build.log" 2>&1 \
    || { grep -E "error|Error" "$OUT/build.log" | head -40; exit 5; }

# 5. The package: executable, the engine's own data and maps, docs. No game
#    data: fetch_runtime_data.py stages the demo, or the player's own files go
#    in data/ and maps/.
PKG=$OUT/fheroes2
mkdir -p "$PKG/files/data" "$PKG/maps" "$PKG/data" "$PKG/docs"
cp "$OUT/build/fheroes2" "$PKG/fheroes2"
"$PREFIX/bin/m68k-amigaos-strip" "$PKG/fheroes2"
cp "$TREE"/files/data/*.h2d "$PKG/files/data/"
cp "$TREE"/maps/*.fh2m "$PKG/maps/" 2>/dev/null || true
cp "$TREE/LICENSE" "$TREE/changelog.txt" "$PKG/docs/"
cp "$HERE/README.md" "$PKG/docs/README-Amiga.md"
python3 - "$OUT" "$PIN" "$EPOCH" "$HERE" <<'PY'
import hashlib, json, sys
from pathlib import Path
out, pin, epoch, here = Path(sys.argv[1]), sys.argv[2], sys.argv[3], Path(sys.argv[4])
d = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()
sdk = Path((out / "build/CMakeCache.txt").read_text().split("AMIGA_STOVE:UNINITIALIZED=")[1].split("\n")[0]) \
    if "AMIGA_STOVE:UNINITIALIZED=" in (out / "build/CMakeCache.txt").read_text() else None
receipt = {
    "source_commit": pin, "source_date_epoch": epoch,
    "patches": {p.name: d(p) for p in sorted((here / "patches").glob("*.patch"))},
    "executable_unstripped_sha256": d(out / "build/fheroes2"),
    "executable_sha256": d(out / "fheroes2/fheroes2"),
    "cpu": "68040, soft float (m68k-soft-m68040.specs)",
    "scope": "Offline source build only; no runtime or release pass implied",
}
(out / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
print(json.dumps(receipt, indent=2))
PY
