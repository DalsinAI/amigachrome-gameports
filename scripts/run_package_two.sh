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

echo "=== Create Games Workshop ==="
INSTANCES_ROOT=$(cd "$AMIGACHROME_ROOT" && python3 - <<'PY'
from pathlib import Path
import sys
sys.path.insert(0, str(Path.cwd() / "scripts"))
import cradle_paths
print(cradle_paths.get(Path.cwd(), "instances"))
PY
)
mkdir -p "$INSTANCES_ROOT"

INSTANCE=$(python3 - "$INSTANCES_ROOT" <<'PY'
import re, sys
from pathlib import Path
root=Path(sys.argv[1])
used=[]
for p in root.iterdir():
    if not p.is_dir():
        continue
    m=re.fullmatch(r"Instance-(\d+)", p.name)
    if m:
        used.append(int(m.group(1)))
n=max(used, default=0)+1
print(root / f"Instance-{n}")
PY
)

echo "Provisioning $INSTANCE"
cd "$AMIGACHROME_ROOT"
python3 scripts/provision.py \
  --install-dir "$INSTANCE" \
  --recipe amigaos-3.2 \
  --aros amigaos32 \
  --cpu-profile 68040 \
  --dh0-mode replace \
  --media-repository "$MEDIA_REPO"

python3 - "$INSTANCE" <<'PY'
import json, sys
from pathlib import Path

inst=Path(sys.argv[1])
local_path=inst/"config"/"local.json"
identity_path=inst/"config"/"instance.json"
baseline_path=inst/"config"/"recipe-baseline.json"

local=json.loads(local_path.read_text(encoding="utf-8"))
local.update({
    "machine": "a1200-aaplus",
    "runtimeEngine": "ac090-native",
    "cpu": "m68k-ac090",
    "cpuProfile": "68040",
    "ac090Cpu": "040",
    "chipMemory": 8 * 1024 * 1024,
    "fastMemory": 0,
    "z3Memory": 0,
    "ac090FastMemoryMiB": 512,
    "ac090MemoryMode": "standard",
})
local_path.write_text(json.dumps(local, indent=2, sort_keys=True)+"\n", encoding="utf-8")

identity={}
try:
    identity=json.loads(identity_path.read_text(encoding="utf-8"))
except Exception:
    pass
identity["friendlyName"]="Games Workshop"
identity_path.write_text(json.dumps(identity, indent=2, sort_keys=True)+"\n", encoding="utf-8")

if baseline_path.is_file():
    baseline=json.loads(baseline_path.read_text(encoding="utf-8"))
    cfg=baseline.setdefault("config", {})
    for k in ("machine","runtimeEngine","cpu","cpuProfile","ac090Cpu","chipMemory","fastMemory",
              "z3Memory","ac090FastMemoryMiB","ac090MemoryMode"):
        cfg[k]=local[k]
    baseline_path.write_text(json.dumps(baseline, indent=2, sort_keys=True)+"\n", encoding="utf-8")
PY

DH1="$INSTANCE/devices/harddisks/DH1"
test -d "$DH1"
mkdir -p "$DH1/Games"
rm -rf "$DH1/Games/Neverball" "$DH1/Games/AstroMenace"
unzip -q "$MEDIA_REPO/Neverball-Neverputt-AC090-GCC16-0009.zip" -d "$DH1/Games"
unzip -q "$MEDIA_REPO/AstroMenace-AC090-GCC16-0009.zip" -d "$DH1/Games"

echo "Games Workshop ready:"
echo "  instance=$INSTANCE"
echo "  motherboard=a1200-aaplus"
echo "  chip=8 MiB"
echo "  AC090 RAM=512 MiB"
echo "  games=$DH1/Games"
find "$DH1/Games" -maxdepth 2 -type f | sort | head -80
