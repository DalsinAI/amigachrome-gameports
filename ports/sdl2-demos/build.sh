#!/usr/bin/env bash
set -euo pipefail

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
PREFIX=${PREFIX:-"$HOME/AmigaChrome/stoves/os32-gcc16/prefix"}
CC=${CC:-"$PREFIX/bin/m68k-amigaos-gcc"}
STRIP=${STRIP:-"$PREFIX/bin/m68k-amigaos-strip"}
SDL2_CONFIG=${SDL2_CONFIG:-"$PREFIX/bin/sdl2-config"}
OUT=${OUT:-"$ROOT/build/sdl2-demos"}
DEST_ROOT=${DEST_ROOT:-"~/AmigaChromeInstances/Instance-32/devices/harddisks/DH1/SDL2 Demos"}

[ -x "$CC" ] || { echo "missing compiler: $CC" >&2; exit 2; }
[ -x "$SDL2_CONFIG" ] || { echo "missing OpenGPU SDL2 SDK: $SDL2_CONFIG" >&2; exit 2; }
[ -d "$(dirname "$DEST_ROOT")" ] || { echo "Instance-32 DH1 is missing" >&2; exit 2; }

rm -rf "$OUT"
mkdir -p "$OUT/bin" "$OUT/logs" "$DEST_ROOT"

SDL_CFLAGS=$($SDL2_CONFIG --cflags)
SDL_LIBS=$($SDL2_CONFIG --libs)
COMMON_FLAGS="-std=gnu11 -O2 -fomit-frame-pointer -fno-delete-null-pointer-checks -Wall -Wextra -Wno-unused-function"

printf '%s\n' "Compiler: $($CC --version | head -1)" | tee "$OUT/BUILD-INFO.txt"
printf '%s\n' "SDL2: $($SDL2_CONFIG --version)" | tee -a "$OUT/BUILD-INFO.txt"
printf '%s\n' "CFLAGS: $SDL_CFLAGS $COMMON_FLAGS" | tee -a "$OUT/BUILD-INFO.txt"
printf '%s\n' "LIBS: $SDL_LIBS -lm" | tee -a "$OUT/BUILD-INFO.txt"
printf '%s\n' "Target: Instance-32 DH1:SDL2 Demos" | tee -a "$OUT/BUILD-INFO.txt"
printf '%s\n' "Runtime renderer proof: every window title shows SDL renderer name and flags" | tee -a "$OUT/BUILD-INFO.txt"

write_readme() {
    local name=$1
    local text=$2
    cat > "$OUT/bin/$name.readme" <<EOF
$name for OpenUp / AmigaChrome

$text

Common controls:
  F or F11  Toggle fullscreen
  Esc       Quit cleanly

Renderer proof:
  The window title always shows the renderer selected by SDL.
  opengpu [ACCELERATED...] means the OpenGPU renderer is active.
  software [SOFTWARE/CPU...] means SDL fell back to CPU rendering.

Target: AmigaOS 3.2.x, 68040 + FPU
Graphics: OpenGPU-capable SDL2 renderer selected at runtime
Built: $(date -u +%Y-%m-%dT%H:%M:%SZ)
EOF
}

deploy_one() {
    local name=$1
    local src=$2
    local help=$3
    local binary="$OUT/bin/$name"
    local incoming="$DEST_ROOT/.$name.incoming-${GITHUB_RUN_ID:-$$}"
    local readme_incoming="$DEST_ROOT/.$name.readme.incoming-${GITHUB_RUN_ID:-$$}"
    local sums_incoming="$DEST_ROOT/.$name.SHA256SUMS.incoming-${GITHUB_RUN_ID:-$$}"

    echo "=== Building $name ==="
    # shellcheck disable=SC2086
    "$CC" $SDL_CFLAGS $COMMON_FLAGS -I"$HERE/common" "$src" -o "$binary" $SDL_LIBS -lm \
        2>&1 | tee "$OUT/logs/$name.log"
    [ -s "$binary" ] || { echo "$name build produced no binary" >&2; exit 3; }
    if [ -x "$STRIP" ]; then
        "$STRIP" "$binary"
    fi
    write_readme "$name" "$help"
    (
        cd "$OUT/bin"
        sha256sum "$name" "$name.readme" > "$name.SHA256SUMS"
    )

    cp -p "$binary" "$incoming"
    cp -p "$OUT/bin/$name.readme" "$readme_incoming"
    cp -p "$OUT/bin/$name.SHA256SUMS" "$sums_incoming"
    mv -f "$incoming" "$DEST_ROOT/$name"
    mv -f "$readme_incoming" "$DEST_ROOT/$name.readme"
    mv -f "$sums_incoming" "$DEST_ROOT/$name.SHA256SUMS"

    test -s "$DEST_ROOT/$name"
    echo "$name READY on DH1:SDL2 Demos/$name"
    stat -c '%n %s bytes mode=%a' "$DEST_ROOT/$name"
}

echo "######## PHASE ONE ########"
deploy_one OpenFire "$HERE/openfire/openfire.c" \
    "Doom-style procedural fire. C changes palette, Up/Down change heat, Space pauses."

deploy_one OpenTunnel "$HERE/opentunnel/opentunnel.c" \
    "Polar tunnel with OCS-style dithering and a moving Boing Ball. Arrows change direction/speed; Space pauses."

deploy_one OpenTriangles "$HERE/opentriangles/opentriangles.c" \
    "Accelerated SDL_RenderGeometry showcase. T toggles texture, B cycles blend mode, Space pauses; drag with the left mouse button to spin."

deploy_one OpenEffects "$HERE/openeffects/openeffects.c" \
    "Plasma, metaballs, rotozoom and deformation. Space/Right advances, Left goes back, P pauses."

deploy_one OpenRay "$HERE/openray/openray.c" \
    "Interactive raycaster. WASD or arrows move, Shift runs, left-mouse drag turns, M toggles the map."

(
    cd "$OUT/bin"
    sha256sum OpenFire OpenTunnel OpenTriangles OpenEffects OpenRay > PHASE1-SHA256SUMS
)
cp -p "$OUT/bin/PHASE1-SHA256SUMS" "$DEST_ROOT/PHASE1-SHA256SUMS"
echo "=== Phase one deployed; continuing directly into phase two ==="

echo "######## PHASE TWO ########"
deploy_one OpenHypno "$HERE/openhypno/openhypno.c" \
    "Line tunnel, parallax star trails, moving perspective floor and a rotating wireframe cube. Arrows reverse direction; Space pauses."

deploy_one OpenIntro "$HERE/openintro/openintro.c" \
    "Classic intro scene with star trails, Doom fire, a copper-colour sine scroller and CRT scanlines. Space pauses."

deploy_one OpenFloor "$HERE/openfloor/openfloor.c" \
    "Infinite perspective grid, moving copper bars and a shaded bouncing Boing Ball. Arrows change speed, C changes palette, Space pauses."

(
    cd "$OUT/bin"
    sha256sum OpenHypno OpenIntro OpenFloor > PHASE2-SHA256SUMS
)
cp -p "$OUT/bin/PHASE2-SHA256SUMS" "$DEST_ROOT/PHASE2-SHA256SUMS"
echo "=== Phase two deployed; continuing directly into phase three ==="

echo "######## PHASE THREE ########"
deploy_one OpenSprites "$HERE/opensprites/opensprites.c" \
    "Hundreds of alpha/colour-modulated rotating texture copies. Arrows change count, B cycles blend mode, C toggles colour cycling, Space pauses."

deploy_one OpenTarget "$HERE/opentarget/opentarget.c" \
    "Render-to-texture and target composition with rotation, flips and blend modes. B changes blend, T toggles trails, R resets, Space pauses."

deploy_one OpenParticles "$HERE/openparticles/openparticles.c" \
    "Large additive or alpha particle fountain. Arrows change count, B changes blend, G toggles gravity, Space pauses."

deploy_one OpenBoids "$HERE/openboids/openboids.c" \
    "Flocking simulation rendered in one SDL_RenderGeometry batch. Mouse attracts/repels, arrows change count, W changes edges, C colour, R reset."

deploy_one OpenScope "$HERE/openscope/openscope.c" \
    "Generated SDL audio through the Amiga audio backend with an oscilloscope and level display. Arrows change pitch, W waveform, M mute, Space pauses."

(
    cd "$OUT/bin"
    sha256sum OpenSprites OpenTarget OpenParticles OpenBoids OpenScope > PHASE3-SHA256SUMS
    sha256sum OpenFire OpenTunnel OpenTriangles OpenEffects OpenRay \
              OpenHypno OpenIntro OpenFloor \
              OpenSprites OpenTarget OpenParticles OpenBoids OpenScope > ALL-SHA256SUMS
)
cp -p "$OUT/bin/PHASE3-SHA256SUMS" "$DEST_ROOT/PHASE3-SHA256SUMS"
cp -p "$OUT/bin/ALL-SHA256SUMS" "$DEST_ROOT/ALL-SHA256SUMS"

cat > "$DEST_ROOT/README.txt" <<'EOF'
OpenUp SDL2 Demos
=================

Run from Shell:

  CD "DH1:SDL2 Demos"

Phase one:
  OpenFire
  OpenTunnel
  OpenTriangles
  OpenEffects
  OpenRay

Phase two:
  OpenHypno
  OpenIntro
  OpenFloor

Phase three:
  OpenSprites
  OpenTarget
  OpenParticles
  OpenBoids
  OpenScope

Every demo opens in a resizable OpenBench window. F/F11 toggles fullscreen and
Esc exits. Each executable has a matching .readme and SHA256SUMS file.

The window title proves which SDL renderer is actually active:
  opengpu [ACCELERATED...]  OpenGPU renderer selected
  software [SOFTWARE/CPU...] SDL CPU fallback selected
EOF

cp -p "$OUT/BUILD-INFO.txt" "$DEST_ROOT/BUILD-INFO.txt"

echo "=== All SDL2 demos deployed ==="
find "$DEST_ROOT" -maxdepth 1 -type f -printf '%f %s bytes\n' | sort
