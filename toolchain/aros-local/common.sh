#!/usr/bin/env bash
set -euo pipefail
TOOLS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$TOOLS_DIR/../.." && pwd)"
# shellcheck source=/dev/null
source "$TOOLS_DIR/lock.env"
WORK_ROOT="${AMIGACHROME_AROS_WORK_ROOT:-$ROOT/build/aros-local}"
AROS_SRC="$WORK_ROOT/aros-src"
AROS_BUILD="$WORK_ROOT/aros-build"
AROS_TOOLCHAIN_BUILD="$WORK_ROOT/aros-toolchain-build"
AROS_TOOLCHAIN_DIR="${AMIGACHROME_AROS_TOOLCHAIN_PREFIX:-$WORK_ROOT/toolchain}"
AROS_TOOLCHAIN_MARKER="$AROS_TOOLCHAIN_DIR/.amigachrome-toolchain.json"
AROS_PORTS="$WORK_ROOT/aros-ports"
OUT="$WORK_ROOT/out"
CACHE_DIR="${AMIGACHROME_AROS_CACHE_DIR:-$ROOT/RECOVERY/AROS_LOCAL_BUILD}"
SNAPSHOT="$CACHE_DIR/$AROS_SNAPSHOT_NAME"
SNAPSHOT_SHA="$SNAPSHOT.sha256"
TOOLCHAIN_SNAPSHOT="$CACHE_DIR/$AROS_TOOLCHAIN_SNAPSHOT_NAME"
TOOLCHAIN_SNAPSHOT_SHA="$TOOLCHAIN_SNAPSHOT.sha256"
TOOLCHAIN_MANIFEST="$CACHE_DIR/$AROS_TOOLCHAIN_SNAPSHOT_NAME.manifest.json"
LOG_DIR="$WORK_ROOT/logs"
mkdir -p "$WORK_ROOT" "$OUT" "$CACHE_DIR" "$LOG_DIR"

sha256_file() { sha256sum "$1" | awk '{print $1}'; }

require_file() {
  [ -f "$1" ] || { echo "Missing required file: $1" >&2; exit 2; }
}

check_source_ref() {
  local marker="$AROS_SRC/.amigachrome-source-ref"
  if [ -f "$marker" ]; then
    local got
    got="$(tr -d '\r\n' < "$marker")"
    [ "$got" = "$AROS_REF" ] || { echo "AROS snapshot ref mismatch: $got != $AROS_REF" >&2; exit 3; }
  elif [ -d "$AROS_SRC/.git" ]; then
    local got
    got="$(git -C "$AROS_SRC" rev-parse HEAD)"
    [ "$got" = "$AROS_REF" ] || { echo "AROS git ref mismatch: $got != $AROS_REF" >&2; exit 3; }
  else
    echo "AROS source has no verifiable ref marker: $AROS_SRC" >&2
    exit 3
  fi
}

check_toolchain_marker() {
  [ -f "$AROS_TOOLCHAIN_MARKER" ] || return 1
  python3 - "$AROS_TOOLCHAIN_MARKER" "$AROS_REF" "$AROS_TARGET" "$AROS_TOOLCHAIN_DIR" <<'PY'
import json, os, sys
p, ref, target, prefix = sys.argv[1:]
try:
    data = json.load(open(p, encoding='utf-8'))
except Exception as exc:
    raise SystemExit(f"Invalid AROS toolchain marker {p}: {exc}")
expected = os.path.realpath(prefix)
if data.get('arosRef') != ref:
    raise SystemExit(f"AROS toolchain ref mismatch: {data.get('arosRef')} != {ref}")
if data.get('target') != target:
    raise SystemExit(f"AROS toolchain target mismatch: {data.get('target')} != {target}")
if os.path.realpath(data.get('prefix','')) != expected:
    raise SystemExit(f"AROS toolchain prefix mismatch: {data.get('prefix')} != {expected}")
PY
}

write_toolchain_marker() {
  mkdir -p "$AROS_TOOLCHAIN_DIR"
  python3 - "$AROS_TOOLCHAIN_MARKER" "$AROS_REF" "$AROS_TARGET" "$AROS_TOOLCHAIN_DIR" <<'PY'
import json, os, platform, sys, time
p, ref, target, prefix = sys.argv[1:]
data = {
    'schema': 1,
    'arosRef': ref,
    'target': target,
    'prefix': os.path.realpath(prefix),
    'hostSystem': platform.system(),
    'hostMachine': platform.machine(),
    'createdAt': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
}
with open(p, 'w', encoding='utf-8') as f:
    json.dump(data, f, indent=2)
    f.write('\n')
PY
}

build_config_fingerprint() {
  printf '%s\n' \
    "AROS_REF=$AROS_REF" \
    "AROS_TARGET=$AROS_TARGET" \
    "AROS_TOOLCHAIN_DIR=$(realpath -m "$AROS_TOOLCHAIN_DIR")" \
    "AROS_PORTS=$(realpath -m "$AROS_PORTS")" \
    'SERIAL_DEBUG=yes' \
    'CCACHE=yes' \
    'PREBUILT_TOOLCHAIN=yes' | sha256sum | awk '{print $1}'
}

[executed on device: daletop (557d2ffd-2bd4-42f8-8777-9a5024193e1e)]