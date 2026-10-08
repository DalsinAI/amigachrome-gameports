#!/bin/sh
# Neverball / Neverputt release build for the OpenUp stack.
# Uses OpenGPU's SDL2 + GL front ends. No standalone SDL or Mesa runtime.
set -eu

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
SRC=${1:?usage: build.sh /path/to/neverball-source [out-dir]}
OUT=${2:-"$ROOT/build/neverball-release"}

: "${OPENUP_SDK:?set OPENUP_SDK to the OpenGPU/Open SDK root}"
CC=${CC:-m68k-amigaos-gcc}
CXX=${CXX:-m68k-amigaos-g++}
CPU=${CPU:-68040}

SDL2_CONFIG=${SDL2_CONFIG:-"$OPENUP_SDK/bin/sdl2-config"}
[ -x "$SDL2_CONFIG" ] || { echo "missing OpenGPU sdl2-config: $SDL2_CONFIG" >&2; exit 2; }

mkdir -p "$OUT/bin" "$OUT/package/Neverball"
PKG="$OUT/package/Neverball"

SDL_CFLAGS=$("$SDL2_CONFIG" --cflags)
SDL_LIBS=$("$SDL2_CONFIG" --libs --gl)

# Build only the runtime programs. Upstream already carries release .sol assets,
# so the host-only map compiler is not part of the cross build.
make -C "$SRC" clean-src
make -C "$SRC" -j2 neverball neverputt \
  BUILD=release \
  CC="$CC" CXX="$CXX" \
  CFLAGS="-O2 -m$CPU -m68881 -fno-delete-null-pointer-checks" \
  CXXFLAGS="-O2 -m$CPU -m68881 -fno-delete-null-pointer-checks" \
  CPPFLAGS="-DNDEBUG" \
  SDL_CPPFLAGS="$SDL_CFLAGS" SDL_LIBS="$SDL_LIBS" \
  OGL_LIBS="" \
  TTF_LIBS="-L$OPENUP_SDK/lib -lSDL2_ttf" \
  PNG_CPPFLAGS="${PNG_CPPFLAGS:--I$OPENUP_SDK/include}" PNG_LIBS="${PNG_LIBS:--lpng}" \
  JPEG_CPPFLAGS="${JPEG_CPPFLAGS:--I$OPENUP_SDK/include}" JPEG_LIBS="${JPEG_LIBS:--ljpeg}" \
  OGG_LIBS="-lvorbisfile -lvorbis -logg" \
  ENABLE_FETCH=0 ENABLE_NLS=0 \
  USERDIR="PROGDIR:User" DATADIR="PROGDIR:data" LOCALEDIR="PROGDIR:locale"

cp "$SRC/neverball" "$OUT/bin/Neverball"
cp "$SRC/neverputt" "$OUT/bin/Neverputt"
cp "$OUT/bin/Neverball" "$PKG/Neverball"
cp "$OUT/bin/Neverputt" "$PKG/Neverputt"
cp -R "$SRC/data" "$PKG/data"
[ ! -d "$SRC/locale" ] || cp -R "$SRC/locale" "$PKG/locale"
mkdir -p "$PKG/doc"
cp "$SRC/LICENSE.md" "$PKG/"
for f in authors.txt manual.txt release-notes.md; do
  [ ! -f "$SRC/doc/$f" ] || cp "$SRC/doc/$f" "$PKG/doc/"
done
[ ! -d "$SRC/doc/legal" ] || cp -R "$SRC/doc/legal" "$PKG/doc/legal"

cat > "$PKG/OPENUP-RELEASE.txt" <<EOF
Neverball / Neverputt for OpenUp
Upstream: Neverball 1.6.0, commit a1ed09911dca262d80049c12a2824d683af494d6
Target: AmigaOS 3.2.x, 68040+FPU
Graphics: OpenGPU SDL2 + OpenGPU GL
Audio: OpenGPU SDL2 audio -> AHI
Controllers: OpenGPU SDL2 -> OpenInput
Keyboard/mouse: OpenGPU SDL2 native Amiga backend
Runtime dependencies: opengpu.library + SDL2.module + GL.module + openinput.library
EOF

(
  cd "$OUT/package"
  find Neverball -type f -print0 | sort -z | xargs -0 sha256sum > SHA256SUMS
)
echo "Neverball release tree: $PKG"
