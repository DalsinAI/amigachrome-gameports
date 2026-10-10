#!/bin/sh
set -eu

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
SRC=${1:-"$ROOT/build/game-ports/sources/astromenace"}
OUT=${2:-"$ROOT/build/os3/astromenace"}
STOVE=${STOVE:-"$HOME/AmigaChrome/stoves/os32-gcc16"}
P="$STOVE/prefix"
SYS="$P/m68k-amigaos"
CC="$P/bin/m68k-amigaos-gcc"
CXX="$P/bin/m68k-amigaos-g++"
SDL2_CONFIG="$P/bin/sdl2-config"

[ -x "$CC" ] || { echo "missing GCC16 compiler: $CC" >&2; exit 2; }
[ -x "$CXX" ] || { echo "missing GCC16 C++ compiler: $CXX" >&2; exit 2; }
[ -x "$SDL2_CONFIG" ] || { echo "missing OpenGPU SDL2 SDK: $SDL2_CONFIG" >&2; exit 2; }
[ -d "$SRC" ] || { echo "missing pinned AstroMenace source: $SRC" >&2; exit 2; }

find_file() {
  pattern="$1"
  shift
  for root in "$@"; do
    [ -d "$root" ] || continue
    hit=$(find "$root" -type f -name "$pattern" -print -quit 2>/dev/null || true)
    [ -z "$hit" ] || { printf '%s\n' "$hit"; return 0; }
  done
  return 1
}

FALLBACK="$HOME/AmigaChrome-dev/webkit-os32-040/m68k-amigaos"
SDL_INC="$SYS/include/SDL2"
GL_INC="$SYS/include"
SDL_LIB=$(find_file libSDL2.a "$SYS" "$P" || true)
MIX_LIB=$(find_file libSDL2_mixer.a "$SYS" "$P" || true)
[ -n "$MIX_LIB" ] || MIX_LIB=$(find_file libSDL2_mixer_static.a "$SYS" "$P" || true)
GL_LIB=$(find_file 'libGL.a' "$SYS" "$P" || true)
FREETYPE_LIB=$(find_file 'libfreetype*.a' "$SYS" "$P" "$FALLBACK" || true)
PNG_LIB=$(find_file 'libpng*.a' "$SYS" "$P" "$FALLBACK" || true)
ZLIB_LIB=$(find_file 'libz.a' "$SYS" "$P" "$FALLBACK" || true)
FT_HEADER=$(find_file ft2build.h "$SYS/include" "$P/include" "$FALLBACK/include" || true)

missing=0
for pair in \
  "SDL2:$SDL_LIB" "SDL2_mixer/OpenAudio:$MIX_LIB" "OpenGL:$GL_LIB" \
  "FreeType:$FREETYPE_LIB" "PNG:$PNG_LIB" "zlib:$ZLIB_LIB" "FreeType headers:$FT_HEADER"; do
  name=${pair%%:*}
  value=${pair#*:}
  if [ -z "$value" ] || [ ! -f "$value" ]; then
    echo "ASTROMENACE_MISSING: $name"
    missing=1
  else
    echo "ASTROMENACE_FOUND: $name -> $value"
  fi
done
[ -f "$SDL_INC/SDL_mixer.h" ] || { echo "ASTROMENACE_MISSING: SDL_mixer headers"; missing=1; }
[ "$missing" -eq 0 ] || exit 4

FT_INC=$(dirname "$FT_HEADER")

python3 "$HERE/patch_amigaos3.py" "$SRC" "$HERE/audio_amigachrome.cpp"
python3 "$HERE/patch_amiga_random.py" "$SRC"

rm -rf "$OUT"
mkdir -p "$OUT"

SDL_LIBS=$(SDL2_RUNTIME=-noixemul "$SDL2_CONFIG" --libs)

cmake -S "$SRC" -B "$OUT" \
  -DCMAKE_TOOLCHAIN_FILE="$ROOT/toolchain/amigaos3-opengpu.cmake" \
  -DCMAKE_BUILD_TYPE=Release \
  -DAMIGA_STOVE="$STOVE" \
  -DAMIGACHROME=ON \
  -DDONTCREATEVFS=1 \
  -DCMAKE_INCLUDE_PATH="$SYS/include;$SYS/include/SDL2;$FALLBACK/include" \
  -DCMAKE_LIBRARY_PATH="$SYS/lib;$P/lib;$FALLBACK/lib" \
  -DINSTALL_DESKTOP_FILES=OFF \
  -DSDL2_INCLUDE_DIR="$SDL_INC" \
  -DSDL2_LIBRARY_TEMP="$SDL_LIB" \
  -DSDL2_MIXER_INCLUDE_DIR="$SDL_INC" \
  -DSDL2_MIXER_LIBRARY="$MIX_LIB" \
  -DOPENGL_INCLUDE_DIR="$GL_INC" \
  -DOPENGL_gl_LIBRARY="$GL_LIB" \
  -DFREETYPE_INCLUDE_DIRS="$FT_INC" \
  -DFREETYPE_LIBRARY_RELEASE="$FREETYPE_LIB" \
  -DAMIGACHROME_PNG_LIBRARY="$PNG_LIB" \
  -DAMIGACHROME_ZLIB_LIBRARY="$ZLIB_LIB" \
  -DCMAKE_EXE_LINKER_FLAGS="$SDL_LIBS"

cmake --build "$OUT" --target astromenace -j${JOBS:-4}

BIN="$OUT/astromenace"
[ -f "$BIN" ] || BIN=$(find "$OUT" -type f -name astromenace -print -quit)
[ -n "$BIN" ] && [ -f "$BIN" ] || { echo "AstroMenace binary not found" >&2; exit 5; }

file "$BIN"
sha256sum "$BIN"

PKG="$OUT/package/AstroMenace"
rm -rf "$OUT/package"
mkdir -p "$PKG"
cp "$BIN" "$PKG/AstroMenace"
cp -a "$SRC/gamedata" "$PKG/gamedata"
cp "$SRC/LICENSE.md" "$PKG/LICENSE.md"
cp -a "$SRC/licenses" "$PKG/licenses"
cat > "$PKG/README-AmigaChrome.txt" <<EOF
AstroMenace for AmigaChrome
===========================

Target: AmigaOS 3.x / AC090 / 68040 + FPU
Graphics: OpenGPU SDL2 + GL
Audio: OpenGPU SDL2_mixer -> OpenAudio/AHI

Launch AstroMenace from this directory.

On the first launch, if gamedata.vfs is not present, the AmigaChrome build
creates it automatically from the bundled, redistributable upstream gamedata/
tree. Subsequent launches use gamedata.vfs directly.

Upstream source:
https://github.com/viewizard/astromenace
Pinned commit:
bbdb3ac5af2774c92b85c4d9b2a238f606911e66

Licensing:
See LICENSE.md and licenses/. Upstream explicitly licenses the game assets for
redistribution under GPL-3.0, CC BY-SA 4.0 and OFL 1.1 as documented there.
EOF

cat > "$OUT/ASTROMENACE_RUN.txt" <<EOF
AstroMenace first-light engine build
Upstream pin: bbdb3ac5af2774c92b85c4d9b2a238f606911e66
Target: AmigaOS 3.x / AC090 / 68040 + FPU
Compiler: $("$CC" --version | head -1)
Graphics: OpenGPU SDL2 + GL
Audio: OpenGPU SDL2_mixer -> OpenAudio/AHI path
VFS generation intentionally disabled during cross-build.
Runtime gate: title -> ship/menu -> first mission -> input -> audio -> rendered 3D scene -> clean exit.
EOF
