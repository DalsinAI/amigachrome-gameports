#!/usr/bin/env bash
set -u
set -o pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
STOVE=${STOVE:-"$HOME/AmigaChrome/stoves/os32-gcc16"}
PREFIX="$STOVE/prefix"
CC="$PREFIX/bin/m68k-amigaos-gcc"
OUT="$ROOT/build/campaign/AC090-FinishThree-0009"
LOGS="$OUT/logs"
STATUS="$OUT/status"
PAYLOAD="$OUT/payload"
OPENAMIGAGCC_COMMIT=787ced6420bebefaacfb6dfe05c2fc083487aaeb
GUEST_COMMIT=8c570397a613f1df1adc4702164603a37b5956dd

rm -rf "$OUT"
mkdir -p "$LOGS" "$STATUS" "$PAYLOAD"
die(){ echo "FATAL: $*" >&2; exit 1; }
[ -x "$CC" ] || die "compiler missing: $CC"
[ -x "$PREFIX/bin/m68k-amigaos-g++" ] || die "C++ compiler missing"
[ -x "$PREFIX/bin/sdl2-config" ] || die "OpenGPU SDL2 SDK missing"

echo "=== GCC16/0009 proof ==="
GCCSRC="$OUT/openamigagcc"
git init -q "$GCCSRC"
git -C "$GCCSRC" remote add origin https://github.com/DalsinAI/openamigagcc.git
git -C "$GCCSRC" fetch -q --depth 1 origin "$OPENAMIGAGCC_COMMIT" || die "cannot fetch compiler proof"
git -C "$GCCSRC" checkout -q --detach FETCH_HEAD
bash "$GCCSRC/tests/repro/prove.sh" "$PREFIX/bin" 2>&1 | tee "$LOGS/00-compiler-proof.log" || die "compiler proof failed"
echo PASS > "$STATUS/00-compiler-proof"

echo "=== Prepare focused sources ==="
for game in openomf cdogs-sdl nxengine-evo astromenace; do
  python3 scripts/game_port_prepare.py prepare --root "$ROOT" --game "$game" --allow-network 2>&1 | tee "$LOGS/prepare-$game.log" || die "prepare failed: $game"
  python3 scripts/game_port_prepare.py verify --root "$ROOT" --game "$game" 2>&1 | tee -a "$LOGS/prepare-$game.log" || die "verify failed: $game"
done

GUEST="$ROOT/build/game-ports/port-layer"
rm -rf "$GUEST"
git init -q "$GUEST"
git -C "$GUEST" remote add origin https://github.com/DalsinAI/amigachrome-guest.git
git -C "$GUEST" fetch -q --depth 1 origin "$GUEST_COMMIT" || die "cannot fetch guest port layer"
git -C "$GUEST" checkout -q --detach FETCH_HEAD

run_port(){
  local name="$1"; shift
  echo "=== $name ==="
  set +e
  "$@" >"$LOGS/$name.log" 2>&1
  local rc=$?
  set -e
  cat "$LOGS/$name.log"
  if [ "$rc" -eq 0 ]; then echo PASS > "$STATUS/$name"; else echo "FAIL rc=$rc" > "$STATUS/$name"; fi
}
copy_if(){ [ -e "$1" ] || return 0; mkdir -p "$(dirname "$2")"; cp -a "$1" "$2"; }

set -e
run_port 01-openomf env STOVE="$STOVE" GUEST_PORT_LAYER="$GUEST" sh ports/openomf/build-amigaos3.sh
if grep -q "^PASS$" "$STATUS/01-openomf"; then
  copy_if build/os3/openomf/openomf "$PAYLOAD/OpenOMF/openomf"
  copy_if build/os3/openomf/OPENOMF_RUN.txt "$PAYLOAD/OpenOMF/OPENOMF_RUN.txt"
  if [ -d build/os3/openomf/work/resources ]; then
    mkdir -p "$PAYLOAD/OpenOMF"
    cp -a build/os3/openomf/work/resources "$PAYLOAD/OpenOMF/resources"
  fi
fi

run_port 02-nxengine-evo env STOVE="$STOVE" JOBS="${JOBS:-4}" sh ports/nxengine-evo/build-amigaos3.sh
copy_if build/os3/nxengine-evo/nxengine-evo "$PAYLOAD/NXEngine-evo/nxengine-evo"
copy_if build/os3/nxengine-evo/NXENGINE_RUN.txt "$PAYLOAD/NXEngine-evo/NXENGINE_RUN.txt"

run_port 03-astromenace env STOVE="$STOVE" JOBS="${JOBS:-4}" sh ports/astromenace/build-amigaos3.sh
if [ -d build/os3/astromenace/package/AstroMenace ]; then
  mkdir -p "$PAYLOAD"
  cp -a build/os3/astromenace/package/AstroMenace "$PAYLOAD/AstroMenace"
fi
copy_if build/os3/astromenace/ASTROMENACE_RUN.txt "$PAYLOAD/AstroMenace/ASTROMENACE_RUN.txt"

{
 echo "AC090 GCC16/0009 focused finish-three status"
 echo "============================================"
 for f in "$STATUS"/*; do printf "%-24s %s\n" "$(basename "$f")" "$(cat "$f")"; done
} | tee "$OUT/CAMPAIGN_STATUS.txt"

echo "=== Package finished ports ==="
PACKAGES="$OUT/packages"
mkdir -p "$PACKAGES"

package_dir() {
  local src="$1" zipname="$2"
  [ -d "$src" ] || return 0
  (
    cd "$(dirname "$src")"
    zip -qr "$PACKAGES/$zipname" "$(basename "$src")"
  )
}

if grep -q "^PASS$" "$STATUS/01-openomf"; then
  cat > "$PAYLOAD/OpenOMF/README-AmigaChrome.txt" <<'EOF'
OpenOMF / OMFAGA for AmigaChrome
================================
Target: AC090 / AmigaOS 3.x / 68040 + FPU
Renderer: native ACGame indexed AGA
Audio: NULL first-light backend

The engine resources are included. One Must Fall 2097 freeware game data is
not republished in this archive. Use the recorded AmigaChrome runtime-data
fetch/staging path, or place compatible OMF2097 data beneath PROGDIR:resources.

Runtime release gate: launch -> native AGA frame -> arena -> input -> clean exit.
EOF
  package_dir "$PAYLOAD/OpenOMF" "OpenOMF-OMFAGA-AC090-GCC16-0009.zip"
fi

if grep -q "^PASS$" "$STATUS/02-nxengine-evo"; then
  cat > "$PAYLOAD/NXEngine-evo/README-AmigaChrome.txt" <<'EOF'
NXEngine-evo for AmigaChrome
============================
Target: AC090 / AmigaOS 3.x / 68040 + FPU
Graphics/input/audio: OpenGPU SDL2 satellite stack

Cave Story freeware data is not republished in this archive. Use the recorded
AmigaChrome runtime-data fetch/staging path to place the public NXEngine-
compatible dataset beside the engine.

Runtime release gate: opening room -> map/sprites -> movement -> input -> audio -> clean exit.
EOF
  package_dir "$PAYLOAD/NXEngine-evo" "NXEngine-evo-AC090-GCC16-0009.zip"
fi

if grep -q "^PASS$" "$STATUS/03-astromenace"; then
  package_dir "$PAYLOAD/AstroMenace" "AstroMenace-AC090-GCC16-0009.zip"
fi

(
  cd "$PACKAGES"
  find . -type f -name '*.zip' -print0 | sort -z | xargs -0 -r sha256sum
) > "$OUT/PACKAGE_SHA256SUMS"
(
  cd "$PAYLOAD"
  find . -type f -print0 2>/dev/null | sort -z | xargs -0 -r sha256sum
) > "$OUT/SHA256SUMS"

grep -q "^PASS$" "$STATUS/01-openomf" || exit 20
grep -q "^PASS$" "$STATUS/02-nxengine-evo" || exit 21
grep -q "^PASS$" "$STATUS/03-astromenace" || exit 22
echo "FINISH_THREE_PASS"
