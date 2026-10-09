#!/usr/bin/env bash
set -euo pipefail

AMIGACHROME_ROOT="${AMIGACHROME_ROOT:-$HOME/AmigaChrome}"

MEDIA_REPO=$(cd "$AMIGACHROME_ROOT" && python3 - <<'PY'
from pathlib import Path
import sys
sys.path.insert(0, str(Path.cwd() / "scripts"))
import cradle_paths
print(cradle_paths.get(Path.cwd(), "mediaRepository"))
PY
)

NEVERBALL="$MEDIA_REPO/Neverball-Neverputt-AC090-GCC16-0009.zip"
ASTRO="$MEDIA_REPO/AstroMenace-AC090-GCC16-0009.zip"
test -f "$NEVERBALL" || { echo "missing $NEVERBALL" >&2; exit 10; }
test -f "$ASTRO" || { echo "missing $ASTRO" >&2; exit 11; }

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
    if p.is_dir():
        m=re.fullmatch(r"Instance-(\d+)", p.name)
        if m:
            used.append(int(m.group(1)))
print(root / f"Instance-{max(used, default=0)+1}")
PY
)

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
unzip -q "$NEVERBALL" -d "$DH1/Games"
unzip -q "$ASTRO" -d "$DH1/Games"

cat > "$INSTANCE/state/games-workshop.json" <<EOF
{
  "friendlyName": "Games Workshop",
  "machine": "a1200-aaplus",
  "cpu": "68040+FPU",
  "ac090FastMemoryMiB": 512,
  "gamesVolume": "DH1:",
  "gamesDirectory": "DH1:Games",
  "neverballPackage": "$(basename "$NEVERBALL")",
  "astroMenacePackage": "$(basename "$ASTRO")"
}
EOF

echo "GAMES_WORKSHOP_READY"
echo "instance=$INSTANCE"
echo "mediaRepository=$MEDIA_REPO"
echo "machine=a1200-aaplus"
echo "ac090FastMemoryMiB=512"
echo "games=DH1:Games"
find "$DH1/Games" -maxdepth 2 -type f | sort | head -100
