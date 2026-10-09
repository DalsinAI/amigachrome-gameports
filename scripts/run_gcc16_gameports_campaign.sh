#!/usr/bin/env bash
set -u
set -o pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

STOVE=${STOVE:-"$HOME/AmigaChrome/stoves/os32-gcc16"}
PREFIX="$STOVE/prefix"
CC="$PREFIX/bin/m68k-amigaos-gcc"
CAMPAIGN="$ROOT/build/campaign/AC090-GamePorts-open1"
LOGS="$CAMPAIGN/logs"
STATUS="$CAMPAIGN/status"
PAYLOAD="$CAMPAIGN/payload"
OPENAMIGAGCC_COMMIT=787ced6420bebefaacfb6dfe05c2fc083487aaeb
GUEST_COMMIT=8c570397a613f1df1adc4702164603a37b5956dd

rm -rf "$CAMPAIGN"
mkdir -p "$LOGS" "$STATUS" "$PAYLOAD"

die() { echo "FATAL: $*" >&2; exit 1; }

[ -x "$CC" ] || die "qualified stove compiler not found: $CC"
[ -x "$PREFIX/bin/m68k-amigaos-g++" ] || die "qualified stove C++ compiler not found"
[ -x "$PREFIX/bin/sdl2-config" ] || die "OpenGPU SDK is not installed in the GCC16 stove"

{
  echo "campaign=AC090-GamePorts-open1"
  echo "gameports_commit=$(git rev-parse HEAD)"
  echo "guest_commit=$GUEST_COMMIT"
  echo "openamigagcc_commit=$OPENAMIGAGCC_COMMIT"
  echo "stove=$STOVE"
  "$CC" --version | head -1
  "$PREFIX/bin/sdl2-config" --version || true
} | tee "$CAMPAIGN/BUILD_IDENTITY.txt"

echo "=== 1/8 GCC16 open1 hard qualification gate ==="
GCCSRC="$ROOT/build/campaign/openamigagcc"
rm -rf "$GCCSRC"
git init -q "$GCCSRC"
git -C "$GCCSRC" remote add origin https://github.com/DalsinAI/openamigagcc.git
git -C "$GCCSRC" fetch -q --depth 1 origin "$OPENAMIGAGCC_COMMIT" || die "cannot fetch OpenAmigaGCC proof source"
git -C "$GCCSRC" checkout -q --detach FETCH_HEAD
[ -f "$GCCSRC/patches/gcc/0009-amigaos-loop-distribution-stays-on-by-default.patch" ] ||
  die "OpenAmigaGCC 0009 patch is absent from proof source"
command -v qemu-m68k >/dev/null 2>&1 || die "qemu-m68k is required for the GCC16 proof gate"
if ! bash "$GCCSRC/tests/repro/prove.sh" "$PREFIX/bin" 2>&1 | tee "$LOGS/00-compiler-proof.log"; then
  die "GCC16 reproducer suite failed; no game ports will be built"
fi
echo PASS > "$STATUS/00-compiler-proof"

echo "=== Preparing exact pinned source inputs ==="
for game in openomf cdogs-sdl neverball uqm chocolate-doom openjazz sdlpop assaultcube; do
  python3 scripts/game_port_prepare.py prepare --root "$ROOT" --game "$game" --allow-network     2>&1 | tee "$LOGS/prepare-$game.log" || die "source preparation failed for $game"
  python3 scripts/game_port_prepare.py verify --root "$ROOT" --game "$game"     2>&1 | tee -a "$LOGS/prepare-$game.log" || die "source verification failed for $game"
done

GUEST="$ROOT/build/game-ports/port-layer"
rm -rf "$GUEST"
git init -q "$GUEST"
git -C "$GUEST" remote add origin https://github.com/DalsinAI/amigachrome-guest.git
git -C "$GUEST" fetch -q --depth 1 origin "$GUEST_COMMIT" || die "cannot fetch canonical ACGame/OpenOMF profile"
git -C "$GUEST" checkout -q --detach FETCH_HEAD
[ "$(git -C "$GUEST" rev-parse HEAD)" = "$GUEST_COMMIT" ] || die "guest port-layer pin mismatch"

run_port() {
  local seq="$1" name="$2"
  shift 2
  echo "=== $seq $name ==="
  set +e
  "$@" >"$LOGS/$name.log" 2>&1
  local rc=$?
  set -e
  cat "$LOGS/$name.log"
  if [ "$rc" -eq 0 ]; then
    echo PASS > "$STATUS/$name"
  else
    echo "FAIL rc=$rc" > "$STATUS/$name"
  fi
  return 0
}

copy_if() {
  local src="$1" dst="$2"
  [ -e "$src" ] || return 0
  mkdir -p "$(dirname "$dst")"
  cp -a "$src" "$dst"
}

set -e

run_port "2/8" "01-openomf" env STOVE="$STOVE" GUEST_PORT_LAYER="$GUEST"   sh ports/openomf/build-amigaos3.sh
copy_if build/os3/openomf/openomf "$PAYLOAD/OpenOMF/openomf"
copy_if build/os3/openomf/OPENOMF_RUN.txt "$PAYLOAD/OpenOMF/OPENOMF_RUN.txt"

run_port "3/8" "02-neverball" env STOVE="$PREFIX" OPENUP_SDK="$PREFIX" CPU=68040   sh ports/neverball/build.sh build/game-ports/sources/neverball build/os3/neverball-release
copy_if build/os3/neverball-release/package/Neverball "$PAYLOAD/Neverball"
copy_if build/os3/neverball-release/package/SHA256SUMS "$PAYLOAD/Neverball-SHA256SUMS"

run_port "4/8" "03-cdogs" env STOVE="$STOVE"   sh ports/cdogs-sdl/build-amigaos3.sh
copy_if build/os3/cdogs/src/cdogs-sdl "$PAYLOAD/C-Dogs/cdogs-sdl"

run_port "5/8" "04-uqm" env STOVE="$STOVE"   sh ports/uqm/build-amigaos3.sh
UQM_BIN=$(find build/os3/uqm -type f -name 'uqm*' -perm -111 2>/dev/null | head -1 || true)
[ -z "$UQM_BIN" ] || copy_if "$UQM_BIN" "$PAYLOAD/UQM/uqm"

run_port "6/8a" "05-chocolate-doom" env STOVE="$STOVE"   sh ports/chocolate-doom/build-amigaos3.sh
copy_if build/os3/chocolate-doom/src/chocolate-doom "$PAYLOAD/ChocolateDoom/chocolate-doom"

run_port "6/8b" "06-openjazz" env STOVE="$STOVE"   sh ports/openjazz/build-amigaos3.sh
copy_if build/os3/openjazz/OpenJazz "$PAYLOAD/OpenJazz/OpenJazz"

run_port "6/8c" "07-sdlpop" env STOVE="$STOVE"   sh ports/sdlpop/build-amigaos3.sh
copy_if build/game-ports/sources/sdlpop/prince "$PAYLOAD/SDLPoP/prince"
copy_if build/game-ports/sources/sdlpop/data "$PAYLOAD/SDLPoP/data"
copy_if build/game-ports/sources/sdlpop/SDLPoP.ini "$PAYLOAD/SDLPoP/SDLPoP.ini"

echo "=== Preparing AssaultCube patches ==="
ACROOT="$ROOT/build/game-ports/sources/assaultcube"
if git -C "$ACROOT" apply --check "$ROOT/ports/assaultcube/patches/0001-amigaos-server-bootstrap.patch" >/dev/null 2>&1; then
  git -C "$ACROOT" apply "$ROOT/ports/assaultcube/patches/0001-amigaos-server-bootstrap.patch"
fi
if git -C "$ACROOT" apply --check "$ROOT/ports/assaultcube/patches/0002-opengpu-sdk-client-first-light.patch" >/dev/null 2>&1; then
  git -C "$ACROOT" apply "$ROOT/ports/assaultcube/patches/0002-opengpu-sdk-client-first-light.patch"
fi
run_port "7/8a" "08-assaultcube-server" env STOVE="$STOVE"   sh ports/assaultcube/build-amiga-server.sh
copy_if "$ACROOT/source/src/ac_server" "$PAYLOAD/AssaultCube/ac_server"

run_port "7/8b" "09-assaultcube-client" env STOVE="$STOVE"   sh ports/assaultcube/build-amiga-client.sh
copy_if "$ACROOT/source/src/ac_client" "$PAYLOAD/AssaultCube/ac_client"

cat > "$CAMPAIGN/FIRST_LIGHT_TEST_PLAN.txt" <<'EOF'
AC090 GCC16/open1 Game Ports - First-Light Order
================================================

Use one clean AC090 / AmigaOS 3.x / 68040 + FPU instance and this exact payload.

1. OpenOMF / OMFAGA
   Supply original OMF2097 data under PROGDIR:resources.
   Gate: launch -> native indexed AGA frame -> arena -> keyboard/pad -> clean exit.
   Audio is intentionally NULL for this first-light build.

2. Neverball / Neverputt
   Gate A: softpipe title/menu/gameplay.
   Gate B: accelerated OpenGPU/virgl title/menu/gameplay.
   Record whether the old black-frame symptom is gone. Exercise keyboard/pad and clean exit.

3. C-Dogs
   Gate: launch -> usable first frame -> gameplay/input -> audio -> clean exit.
   Specifically record whether old-stove alert 0x80000005 (divide by zero) is absent.

4. UQM
   Supply official UQM 0.8 content.
   Gate: launch -> menu -> solar system/combat -> input -> audio status -> clean exit.

5. Chocolate Doom
   Supply a compatible IWAD (Freedoom is suitable for qualification).
   Gate: menu -> E1M1 -> input -> audio -> config/save -> clean exit.

6. OpenJazz
   Supply Jazz Jackrabbit data.
   Gate: title/menu -> gameplay -> input -> audio -> clean exit.

7. SDLPoP
   Use the staged project runtime data.
   Gate: level one -> animation -> input -> audio -> clean exit.

8. AssaultCube
   Use authorised game data.
   Server: launch/listen/clean stop.
   Client: menu -> offline bot map -> movement -> textured 3D scene -> input -> clean exit.
   Audio remains a separate follow-up if the current client is using the first-light stub.
EOF

{
  echo "AC090 GCC16/open1 game-port campaign status"
  echo "=========================================="
  for f in "$STATUS"/*; do
    [ -f "$f" ] || continue
    printf "%-28s %s\n" "$(basename "$f")" "$(cat "$f")"
  done
} | tee "$CAMPAIGN/CAMPAIGN_STATUS.txt"

find "$PAYLOAD" -type f -print0 2>/dev/null | sort -z | xargs -0 -r sha256sum > "$CAMPAIGN/SHA256SUMS"
find "$CAMPAIGN" -maxdepth 3 -type f -printf '%P\n' | sort > "$CAMPAIGN/CONTENTS.txt"

echo "=== 8/8 campaign complete ==="
cat "$CAMPAIGN/CAMPAIGN_STATUS.txt"
