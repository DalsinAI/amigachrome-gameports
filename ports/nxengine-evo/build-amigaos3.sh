#!/bin/sh
set -eu

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
SRC=${1:-"$ROOT/build/game-ports/sources/nxengine-evo"}
OUT=${2:-"$ROOT/build/os3/nxengine-evo"}
STOVE=${STOVE:-"$HOME/AmigaChrome/stoves/os32-gcc16"}
P="$STOVE/prefix"
SYS="$P/m68k-amigaos"
CC="$P/bin/m68k-amigaos-gcc"
CXX="$P/bin/m68k-amigaos-g++"
SDL2_CONFIG="$P/bin/sdl2-config"
PATCH="$HERE/patches/0001-aros-resource-manager.patch"

[ -x "$CC" ] || { echo "missing GCC16 compiler: $CC" >&2; exit 2; }
[ -x "$CXX" ] || { echo "missing GCC16 C++ compiler: $CXX" >&2; exit 2; }
[ -x "$SDL2_CONFIG" ] || { echo "missing OpenGPU SDL2 SDK: $SDL2_CONFIG" >&2; exit 2; }
[ -d "$SRC" ] || { echo "missing pinned NXEngine-evo source: $SRC" >&2; exit 2; }

if git -C "$SRC" apply --check "$PATCH" >/dev/null 2>&1; then
  git -C "$SRC" apply "$PATCH"
elif git -C "$SRC" apply --reverse --check "$PATCH" >/dev/null 2>&1; then
  :
else
  echo "NXEngine-evo Amiga portability patch does not apply cleanly" >&2
  exit 3
fi

python3 "$HERE/patch_amigaos3_deps.py" "$SRC"

find_file() {
  name="$1"
  shift
  for root in "$@"; do
    [ -d "$root" ] || continue
    hit=$(find "$root" -type f -name "$name" -print -quit 2>/dev/null || true)
    [ -z "$hit" ] || { printf '%s\n' "$hit"; return 0; }
  done
  return 1
}

FALLBACK="$HOME/AmigaChrome-dev/webkit-os32-040/m68k-amigaos"
SDL_INC="$SYS/include/SDL2"
SDL_LIB=$(find_file libSDL2.a "$SYS" "$P" || true)
MIX_LIB=$(find_file libSDL2_mixer.a "$SYS" "$P" || true)
IMG_LIB=$(find_file libSDL2_image.a "$SYS" "$P" || true)
PNG_LIB=$(find_file 'libpng*.a' "$SYS" "$P" "$FALLBACK" || true)
JPEG_LIB=$(find_file 'libjpeg*.a' "$SYS" "$P" "$FALLBACK" || true)
PNG_HEADER=$(find_file png.h "$SYS/include" "$P/include" "$FALLBACK/include" || true)
JPEG_HEADER=$(find_file jpeglib.h "$SYS/include" "$P/include" "$FALLBACK/include" || true)

missing=0
for pair in "SDL2:$SDL_LIB" "SDL2_mixer:$MIX_LIB" "SDL2_image:$IMG_LIB" "PNG:$PNG_LIB" "JPEG:$JPEG_LIB" "PNG headers:$PNG_HEADER" "JPEG headers:$JPEG_HEADER"; do
  name=${pair%%:*}
  value=${pair#*:}
  if [ -z "$value" ] || [ ! -f "$value" ]; then
    echo "NXENGINE_MISSING: $name"
    missing=1
  else
    echo "NXENGINE_FOUND: $name -> $value"
  fi
done
[ -f "$SDL_INC/SDL.h" ] || { echo "NXENGINE_MISSING: SDL2 headers at $SDL_INC"; missing=1; }
[ "$missing" -eq 0 ] || exit 4

PNG_INC=$(dirname "$PNG_HEADER")
JPEG_INC=$(dirname "$JPEG_HEADER")

rm -rf "$OUT"
mkdir -p "$OUT"

SDL_LIBS=$(SDL2_RUNTIME=-mcrt=nix20 "$SDL2_CONFIG" --libs)

cmake -S "$SRC" -B "$OUT" \
  -DCMAKE_TOOLCHAIN_FILE="$ROOT/toolchain/amigaos3-opengpu.cmake" \
  -DCMAKE_BUILD_TYPE=Release \
  -DAMIGA_STOVE="$STOVE" \
  -DPLATFORM=pc \
  -DCMAKE_INCLUDE_PATH="$SYS/include;$SYS/include/SDL2;$FALLBACK/include" \
  -DCMAKE_LIBRARY_PATH="$SYS/lib;$P/lib;$FALLBACK/lib" \
  -DSDL2_INCLUDE_DIR="$SDL_INC" \
  -DSDL2_LIBRARY_TEMP="$SDL_LIB" \
  -DSDL2_MIXER_INCLUDE_DIR="$SDL_INC" \
  -DSDL2_MIXER_LIBRARY="$MIX_LIB" \
  -DSDL2_IMAGE_INCLUDE_DIR="$SDL_INC" \
  -DSDL2_IMAGE_LIBRARY="$IMG_LIB" \
  -DPNG_PNG_INCLUDE_DIR="$PNG_INC" \
  -DPNG_LIBRARY="$PNG_LIB" \
  -DJPEG_INCLUDE_DIR="$JPEG_INC" \
  -DJPEG_LIBRARY="$JPEG_LIB" \
  -DCMAKE_EXE_LINKER_FLAGS="$SDL_LIBS"

cmake --build "$OUT" --target nx -j${JOBS:-4}

BIN="$OUT/nxengine-evo"
[ -f "$BIN" ] || BIN=$(find "$OUT" -type f -name nxengine-evo -print -quit)
[ -n "$BIN" ] && [ -f "$BIN" ] || { echo "NXEngine-evo binary not found" >&2; exit 5; }

file "$BIN"
sha256sum "$BIN"
cat > "$OUT/NXENGINE_RUN.txt" <<EOF
NXEngine-evo first-light build
Target: AmigaOS 3.x / AC090 / 68040 + FPU
Compiler: $("$CC" --version | head -1)
Graphics/input/audio: OpenGPU SDL2 satellite stack
Cave Story data is not bundled.
Runtime gate: opening room -> map/sprites -> movement -> input -> audio -> clean exit.
EOF
