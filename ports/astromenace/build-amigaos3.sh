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
SDL_LIB=$(find_file libSDL2.a "$SYS" "$P")
GL_LIB=$(find_file 'libGL.a' "$SYS" "$P")
OPENAL_LIB=$(find_file 'libopenal*.a' "$SYS" "$P" "$FALLBACK" || true)
[ -n "$OPENAL_LIB" ] || OPENAL_LIB=$(find_file 'libOpenAL*.a' "$SYS" "$P" "$FALLBACK" || true)
ALUT_LIB=$(find_file 'libalut*.a' "$SYS" "$P" "$FALLBACK" || true)
OGG_LIB=$(find_file 'libogg*.a' "$SYS" "$P" "$FALLBACK" || true)
VORBIS_LIB=$(find_file 'libvorbis.a' "$SYS" "$P" "$FALLBACK" || true)
VORBISFILE_LIB=$(find_file 'libvorbisfile.a' "$SYS" "$P" "$FALLBACK" || true)
FREETYPE_LIB=$(find_file 'libfreetype*.a' "$SYS" "$P" "$FALLBACK" || true)

OPENAL_HEADER=$(find_file al.h "$SYS/include" "$P/include" "$FALLBACK/include" || true)
ALUT_HEADER=$(find_file alut.h "$SYS/include" "$P/include" "$FALLBACK/include" || true)
OGG_HEADER=$(find_file ogg.h "$SYS/include" "$P/include" "$FALLBACK/include" || true)
VORBIS_HEADER=$(find_file vorbisfile.h "$SYS/include" "$P/include" "$FALLBACK/include" || true)
FT_HEADER=$(find_file ft2build.h "$SYS/include" "$P/include" "$FALLBACK/include" || true)

missing=0
for pair in \
  "SDL2:$SDL_LIB" "OpenGL:$GL_LIB" "OpenAL:$OPENAL_LIB" "ALUT:$ALUT_LIB" \
  "Ogg:$OGG_LIB" "Vorbis:$VORBIS_LIB" "Vorbisfile:$VORBISFILE_LIB" "FreeType:$FREETYPE_LIB" \
  "OpenAL headers:$OPENAL_HEADER" "ALUT headers:$ALUT_HEADER" "Ogg headers:$OGG_HEADER" \
  "Vorbis headers:$VORBIS_HEADER" "FreeType headers:$FT_HEADER"; do
  name=${pair%%:*}
  value=${pair#*:}
  if [ -z "$value" ] || [ ! -f "$value" ]; then
    echo "ASTROMENACE_MISSING: $name"
    missing=1
  else
    echo "ASTROMENACE_FOUND: $name -> $value"
  fi
done
[ "$missing" -eq 0 ] || exit 4

OPENAL_INC=$(dirname "$OPENAL_HEADER")
ALUT_INC=$(dirname "$ALUT_HEADER")
OGG_INC=$(dirname "$(dirname "$OGG_HEADER")")
VORBIS_INC=$(dirname "$(dirname "$VORBIS_HEADER")")
FT_INC=$(dirname "$FT_HEADER")

rm -rf "$OUT"
mkdir -p "$OUT"

SDL_LIBS=$(SDL2_RUNTIME=-mcrt=nix20 "$SDL2_CONFIG" --libs)

cmake -S "$SRC" -B "$OUT" \
  -DCMAKE_TOOLCHAIN_FILE="$ROOT/toolchain/amigaos3-opengpu.cmake" \
  -DCMAKE_BUILD_TYPE=Release \
  -DAMIGA_STOVE="$STOVE" \
  -DDONTCREATEVFS=1 \
  -DCMAKE_INCLUDE_PATH="$SYS/include;$SYS/include/SDL2;$FALLBACK/include" \
  -DCMAKE_LIBRARY_PATH="$SYS/lib;$P/lib;$FALLBACK/lib" \
  -DINSTALL_DESKTOP_FILES=OFF \
  -DSDL2_INCLUDE_DIR="$SDL_INC" \
  -DSDL2_LIBRARY_TEMP="$SDL_LIB" \
  -DOPENGL_INCLUDE_DIR="$GL_INC" \
  -DOPENGL_gl_LIBRARY="$GL_LIB" \
  -DOPENAL_INCLUDE_DIR="$OPENAL_INC" \
  -DOPENAL_LIBRARY="$OPENAL_LIB" \
  -DALUT_INCLUDE_DIR="$ALUT_INC" \
  -DALUT_LIBRARY="$ALUT_LIB" \
  -DOGG_INCLUDE_DIR="$OGG_INC" \
  -DOGG_LIBRARY="$OGG_LIB" \
  -DVORBISFILE_INCLUDE_DIR="$VORBIS_INC" \
  -DVORBISFILE_LIBRARY="$VORBISFILE_LIB" \
  -DVORBIS_LIBRARY="$VORBIS_LIB" \
  -DFREETYPE_INCLUDE_DIRS="$FT_INC" \
  -DFREETYPE_LIBRARY_RELEASE="$FREETYPE_LIB" \
  -DCMAKE_EXE_LINKER_FLAGS="$SDL_LIBS"

cmake --build "$OUT" --target astromenace -j${JOBS:-4}

BIN="$OUT/astromenace"
[ -f "$BIN" ] || BIN=$(find "$OUT" -type f -name astromenace -print -quit)
[ -n "$BIN" ] && [ -f "$BIN" ] || { echo "AstroMenace binary not found" >&2; exit 5; }

file "$BIN"
sha256sum "$BIN"
cat > "$OUT/ASTROMENACE_RUN.txt" <<EOF
AstroMenace first-light engine build
Upstream pin: bbdb3ac5af2774c92b85c4d9b2a238f606911e66
Target: AmigaOS 3.x / AC090 / 68040 + FPU
Compiler: $("$CC" --version | head -1)
Graphics: OpenGPU SDL2 + GL
Audio: OpenAudio/OpenAL compatibility path
VFS generation intentionally disabled during cross-build.
Runtime gate: title -> ship/menu -> first mission -> input -> audio -> rendered 3D scene -> clean exit.
EOF
