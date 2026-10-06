#!/bin/sh
# Build C-Dogs SDL for AmigaOS 3.x (68040 + FPU) against openamigasdl's SDL 2.
# Work in progress: it builds and stages, but does not run yet (see README.md).
#   ports/cdogs-sdl/build.sh SOURCE OPENSDL_DIR [OUT]
# SOURCE       cxong/cdogs-sdl checked out at the pin below, unmodified
# OPENSDL_DIR  a built libSDL2-amigaos3 (openamigasdl's sdl2/build.sh), with
#              lib/SDL2_mixer built (make -C lib/SDL2_mixer native-build SDL2_DIR=../..)
# OUT          where the game folder is staged (default build/cdogs-sdl-os3)
# Needs bebbo's m68k-amigaos-gcc on PATH (the Kitchen's OS 3.2 stove), cmake,
# and ffmpeg (the shipped OGG sounds become WAV; our SDL2_mixer reads WAV only).
set -eu
PIN=8263a6f0200a71498f8c47bd0fce0f2d2d678d84
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
SRC=$(CDPATH= cd -- "$1" && pwd)
OPENSDL=$(CDPATH= cd -- "$2" && pwd)
OUT=${3:-build/cdogs-sdl-os3}

[ "$(git -C "$SRC" rev-parse HEAD)" = "$PIN" ] || { echo "C-Dogs source is not at $PIN" >&2; exit 1; }
if ! git -C "$SRC" apply --check -R "$HERE/patches/0001-amigaos3-bootstrap.patch" 2>/dev/null; then
    git -C "$SRC" apply "$HERE/patches/0001-amigaos3-bootstrap.patch"
fi

BUILD=$SRC/build-amigaos3
mkdir -p "$BUILD"
cmake -S "$SRC" -B "$BUILD" \
    -DCMAKE_TOOLCHAIN_FILE="$HERE/amigaos3.cmake" \
    -DAMIGA_OS3=ON -DOPENSDL_DIR="$OPENSDL" \
    -DCDOGS_DATA_DIR=PROGDIR: -DBUILD_TESTING=OFF
cmake --build "$BUILD" --target cdogs-sdl -j"$(nproc 2>/dev/null || echo 4)"

mkdir -p "$OUT/CDogs"
cp "$BUILD/src/cdogs-sdl" "$SRC/COPYING" "$OUT/CDogs/"
for d in data graphics missions dogfights; do
    cp -R "$SRC/$d" "$OUT/CDogs/"
done
( cd "$SRC/sounds" && find . -type f \( -name '*.ogg' -o -name '*.wav' -o -name '*.mp3' \) ) |
while read -r f; do
    mkdir -p "$OUT/CDogs/sounds/$(dirname "$f")"
    ffmpeg -loglevel error -y -i "$SRC/sounds/$f" -ac 1 -ar 22050 -sample_fmt s16 \
        "$OUT/CDogs/sounds/${f%.*}.wav"
done
echo "staged in $OUT/CDogs"
