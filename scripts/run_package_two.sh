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

AMIGACHROME_ROOT="${AMIGACHROME_ROOT:-$HOME/AmigaChrome}"
MEDIA_REPO=$(python3 - "$AMIGACHROME_ROOT" <<'PY'
import json
import os
import sys
from pathlib import Path

root = Path(sys.argv[1]).expanduser()
cfg = root / "config" / "paths.json"
chosen = None
try:
    data = json.loads(cfg.read_text(encoding="utf-8"))
    raw = data.get("mediaRepository")
    if isinstance(raw, str) and raw.strip():
        chosen = Path(raw).expanduser()
except (OSError, ValueError):
    pass

if chosen is None:
    env = os.environ.get("AMIGACHROME_MEDIA_REPOSITORY")
    chosen = Path(env).expanduser() if env else Path.home() / ".local" / "share" / "amigachrome" / "install-media"

print(chosen.resolve())
PY
)

test -d "$MEDIA_REPO" || {
  echo "Media Repository does not exist: $MEDIA_REPO" >&2
  exit 6
}

cp -f "$PKG/Neverball-Neverputt-AC090-GCC16-0009.zip" "$MEDIA_REPO/"
cp -f "$PKG/AstroMenace-AC090-GCC16-0009.zip" "$MEDIA_REPO/"
cp -f "$PKG/SHA256SUMS" "$MEDIA_REPO/Neverball-AstroMenace-AC090-GCC16-0009-SHA256SUMS"

printf 'Neverball/Neverputt and AstroMenace packages complete.\n'
printf 'Media Repository: %s\n' "$MEDIA_REPO"
ls -lh "$MEDIA_REPO/Neverball-Neverputt-AC090-GCC16-0009.zip" \
       "$MEDIA_REPO/AstroMenace-AC090-GCC16-0009.zip"
cat "$PKG/SHA256SUMS"
