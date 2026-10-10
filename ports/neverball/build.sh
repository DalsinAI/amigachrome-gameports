#!/bin/sh
# Neverball / Neverputt release build for the OpenUp stack.
# Uses OpenGPU SDL2 + GL and OpenGPU satellite libraries. No standalone SDL,
# Mesa, libpng, libjpeg or libvorbis runtime is carried by this port.
set -eu

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
SRC=${1:?usage: build.sh /path/to/neverball-source [out-dir]}
OUT=${2:-"$ROOT/build/neverball-release"}

: "${OPENUP_SDK:?set OPENUP_SDK to the OpenGPU/Open SDK root}"
CPU=${CPU:-68040}
STOVE=${STOVE:-$HOME/AmigaChrome/stoves/os32-gcc16/prefix}
CC=${CC:-$STOVE/bin/m68k-amigaos-gcc}
CXX=${CXX:-$STOVE/bin/m68k-amigaos-g++}
DATA_PK3=${NEVERBALL_DATA_PK3:-"$ROOT/build/game-ports/runtime-assets/neverball/data-1.6.0.pk3"}
DATA_SHA=b2eddfbe05443e36719541639cb6246c5dd81260bce67a053d0d2c0ad34bc58c

[ -x "$CC" ] || { echo "missing 68k compiler: $CC" >&2; exit 2; }
[ -x "$CXX" ] || { echo "missing 68k C++ compiler: $CXX" >&2; exit 2; }
[ -f "$DATA_PK3" ] || { echo "missing prepared Neverball release data: run game_port_prepare.py prepare --game neverball" >&2; exit 2; }
[ "$(sha256sum "$DATA_PK3" | awk '{print $1}')" = "$DATA_SHA" ] || {
    echo "Neverball release-data SHA-256 mismatch" >&2
    exit 2
}

SDL2_CONFIG=${SDL2_CONFIG:-"$OPENUP_SDK/bin/sdl2-config"}
[ -x "$SDL2_CONFIG" ] || { echo "missing OpenGPU sdl2-config: $SDL2_CONFIG" >&2; exit 2; }

# The patches apply in order. A tree they were applied to before (a second run) is left as it is:
# the git directory keeps the checksum of the set that was applied. One patched by an older
# build.sh (the first patch alone, no checksum) skips the patches it already has.
STAMP=$(git -C "$SRC" rev-parse --absolute-git-dir)/openup-patches-applied
SUM=$(cat "$HERE"/patches/*.patch | sha256sum | cut -d' ' -f1)
if [ "$(cat "$STAMP" 2>/dev/null)" != "$SUM" ]; then
    for PATCH in "$HERE"/patches/*.patch; do
        [ -f "$PATCH" ] || { echo "missing Neverball OpenUp patch: $PATCH" >&2; exit 2; }
        if git -C "$SRC" apply --check "$PATCH" >/dev/null 2>&1; then
            git -C "$SRC" apply "$PATCH"
        elif [ ! -f "$STAMP" ] && git -C "$SRC" apply --reverse --check "$PATCH" >/dev/null 2>&1; then
            : # already applied by an older build.sh
        else
            echo "Neverball patch $(basename "$PATCH") does not apply cleanly to pinned source" >&2
            exit 3
        fi
    done
    echo "$SUM" > "$STAMP"
fi

mkdir -p "$OUT/bin" "$OUT/package/Neverball"
PKG="$OUT/package/Neverball"

SDL_CFLAGS=$("$SDL2_CONFIG" --cflags)
SDL_LIBS=$("$SDL2_CONFIG" --libs --gl)

make -C "$SRC" clean-src
make -C "$SRC" -j${JOBS:-6} neverball neverputt \
  BUILD=release \
  CC="$CC" CXX="$CXX" \
  CFLAGS="-O2 -m$CPU -m68881 -fno-delete-null-pointer-checks" \
  CXXFLAGS="-O2 -m$CPU -m68881 -fno-delete-null-pointer-checks" \
  CPPFLAGS="-DNDEBUG" \
  SDL_CPPFLAGS="$SDL_CFLAGS" SDL_LIBS="$SDL_LIBS" \
  OGL_LIBS="" \
  TTF_LIBS="-L$OPENUP_SDK/lib -lSDL2_image -lSDL2_mixer -lSDL2_ttf" \
  PNG_CPPFLAGS="" PNG_LIBS="" \
  JPEG_CPPFLAGS="" JPEG_LIBS="" \
  OGG_LIBS="" \
  ENABLE_FETCH=0 ENABLE_NLS=0 \
  USERDIR="PROGDIR:User" DATADIR="PROGDIR:data" LOCALEDIR="PROGDIR:locale"

rm -rf "$PKG"
mkdir -p "$OUT/bin" "$PKG/doc" "$PKG/data"
cp "$SRC/neverball" "$OUT/bin/Neverball"
cp "$SRC/neverputt" "$OUT/bin/Neverputt"
cp "$OUT/bin/Neverball" "$PKG/Neverball"
cp "$OUT/bin/Neverputt" "$PKG/Neverputt"
cp "$DATA_PK3" "$PKG/data/data-1.6.0.pk3"
cp "$SRC/LICENSE.md" "$PKG/"
for f in authors.txt manual.txt release-notes.md; do
    [ ! -f "$SRC/doc/$f" ] || cp "$SRC/doc/$f" "$PKG/doc/"
done
[ ! -d "$SRC/doc/legal" ] || cp -R "$SRC/doc/legal" "$PKG/doc/legal"

cat > "$PKG/OPENUP-RELEASE.txt" <<EOF
Neverball / Neverputt for OpenUp
Upstream: Neverball 1.6.0, commit a1ed09911dca262d80049c12a2824d683af494d6
Official release data SHA-256: $DATA_SHA
Target: AmigaOS 3.2.x, 68040+FPU
Graphics: OpenGPU SDL2 + OpenGPU GL
Images: OpenGPU SDL2_image
Audio: OpenGPU SDL2_mixer -> SDL2 audio -> AHI
Controllers: OpenGPU SDL2 -> OpenInput
Keyboard/mouse: OpenGPU SDL2 native Amiga backend
Runtime dependencies: opengpu.library + SDL2.module + GL.module + openinput.library
EOF

(
  cd "$OUT/package"
  find Neverball -type f -print0 | sort -z | xargs -0 sha256sum > SHA256SUMS
)
echo "Neverball release tree: $PKG"
