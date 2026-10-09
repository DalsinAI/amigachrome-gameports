#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

STOVE=${STOVE:-"$HOME/AmigaChrome/stoves/os32-gcc16"}
PREFIX="$STOVE/prefix"
OUT="$ROOT/build/package-two"
PKG="$OUT/packages"
rm -rf "$OUT"
mkdir -p "$PKG"

python3 scripts/game_port_prepare.py prepare --root "$ROOT" --game neverball --allow-network
python3 scripts/game_port_prepare.py verify --root "$ROOT" --game neverball
python3 scripts/game_port_prepare.py prepare --root "$ROOT" --game astromenace --allow-network
python3 scripts/game_port_prepare.py verify --root "$ROOT" --game astromenace

rm -rf build/os3/neverball-release
STOVE="$PREFIX" OPENUP_SDK="$PREFIX" CPU=68040 JOBS="${JOBS:-6}" \
  sh ports/neverball/build.sh \
  build/game-ports/sources/neverball \
  build/os3/neverball-release

test -f build/os3/neverball-release/package/Neverball/Neverball
test -f build/os3/neverball-release/package/Neverball/Neverputt
test -f build/os3/neverball-release/package/Neverball/data/data-1.6.0.pk3
(
  cd build/os3/neverball-release/package
  zip -qr "$PKG/Neverball-Neverputt-AC090-GCC16-0009.zip" Neverball
)

rm -rf build/os3/astromenace
STOVE="$STOVE" JOBS="${JOBS:-4}" sh ports/astromenace/build-amigaos3.sh

test -f build/os3/astromenace/package/AstroMenace/AstroMenace
test -d build/os3/astromenace/package/AstroMenace/gamedata
test -f build/os3/astromenace/package/AstroMenace/LICENSE.md
(
  cd build/os3/astromenace/package
  zip -qr "$PKG/AstroMenace-AC090-GCC16-0009.zip" AstroMenace
)

(
  cd "$PKG"
  sha256sum *.zip > SHA256SUMS
)

printf 'Neverball/Neverputt and AstroMenace packages complete.\n'
cat "$PKG/SHA256SUMS"
