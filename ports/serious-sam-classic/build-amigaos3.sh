#!/bin/sh
# Serious Sam Classic: The First Encounter for AmigaChrome/OpenUp.
# First playable lane: AC090 68040+FPU, OpenGPU SDL2/GL/input/audio,
# monolithic runtime, no editor/server/tool baggage, no OMC offload yet.
set -eu

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
SRC=${SERIOUS_SAM_SRC:-"$ROOT/build/game-ports/sources/serious-sam-classic"}
OUT=${1:-"$ROOT/build/os3/serious-sam-tfe"}
STOVE=${STOVE:-"$HOME/AmigaChrome/stoves/os32-gcc16"}
P="$STOVE/prefix"
TOOLCHAIN="$ROOT/toolchain/amigaos3-opengpu.cmake"
HOST="$OUT/host"
BUILD="$OUT/build"
PKG="$OUT/package/SeriousSam-TFE"

test -d "$SRC/SamTFE/Sources"
test -x "$P/bin/m68k-amigaos-g++"
test -x "$P/bin/sdl2-config"
test -f "$TOOLCHAIN"
command -v cmake >/dev/null
command -v bison >/dev/null
command -v flex >/dev/null
command -v c++ >/dev/null

rm -rf "$OUT"
mkdir -p "$HOST" "$BUILD" "$PKG"

python3 "$HERE/patch_amigaos3.py" "$SRC"

# Ecc is a build-host tool. Generate and compile it for Linux, then hand
# its path to the m68k CMake build so no host utility is cross-compiled.
(
  cd "$SRC/SamTFE/Sources"
  flex -oEcc/Scanner.cpp Ecc/Scanner.l
  bison -oEcc/Parser.cpp Ecc/Parser.y -d
  cp Ecc/Parser.hpp Ecc/Parser.h
  c++ -std=c++14 -O2 -I. Ecc/Main.cpp Ecc/Parser.cpp Ecc/Scanner.cpp -o "$HOST/ecc"
)
test -x "$HOST/ecc"

export OPENUP_SDK="$P"
export SDL2_RUNTIME="-mcrt=nix20"

cmake -S "$SRC/SamTFE/Sources" -B "$BUILD" \
  -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
  -DAMIGA_STOVE="$STOVE" \
  -DAMIGA_THREAD_FLAGS= \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXE_LINKER_FLAGS="-Wl,--stack,1048576" \
  -DUSE_SYSTEM_SDL2=ON \
  -DUSE_SYSTEM_ZLIB=OFF \
  -DUSE_ASM=OFF \
  -DUSE_I386_NASM_ASM=OFF \
  -DUSE_SINGLE_THREAD=ON \
  -DBUILD_DEDICATED_SERVER=OFF \
  -DBUILD_MAKEFONT=OFF \
  -DBUILD_TEXConv=OFF \
  -DBUILD_AMP11LIB=OFF \
  -DXPLUS=OFF \
  -DECC="$HOST/ecc"

cmake --build "$BUILD" --target SeriousSam -j"${JOBS:-6}"

EXE=$(find "$BUILD" -type f -name SeriousSam -perm -111 | head -1)
test -n "$EXE"
cp "$EXE" "$PKG/SeriousSam"
cp "$SRC/LICENSE" "$PKG/LICENSE-SeriousEngine.txt"

cat > "$PKG/OPENUP-RELEASE.txt" <<'EOF'
Serious Sam Classic: The First Encounter — OpenUp first-light build
==================================================================
Target: AmigaOS 3.2.x / AC090 / 68040 + FPU
CPU model: native m68k, 32-bit big-endian
Graphics: OpenGPU SDL2 + OpenGPU GL
Input: OpenGPU SDL2 -> OpenInput
Audio: Serious Engine SDL2 device -> OpenGPU/OpenAudio/AHI path
Networking: not a first-light requirement; local game remains enabled
Threads: Serious Engine single-thread personality for first light
Modules: Game, Entities and Shaders are linked into one executable
Optional dynamic codecs: disabled for first light
OpenMulticore: no offload in this build; compute domains are documented separately

Commercial Serious Sam game data is NOT included.
Place the executable in a legitimate TFE data directory (or stage that data
beside it) and launch SeriousSam.
EOF

"$P/bin/m68k-amigaos-strip" "$PKG/SeriousSam" 2>/dev/null || true
file "$PKG/SeriousSam" || true
sha256sum "$PKG/SeriousSam" > "$OUT/SHA256SUMS"

echo "Serious Sam TFE first-light package: $PKG"
