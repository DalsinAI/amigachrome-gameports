#!/usr/bin/env bash
set -euo pipefail

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"

RUN_ID=${GITHUB_RUN_ID:-manual-$(date -u +%Y%m%dT%H%M%SZ)}
STOVE=${STOVE:-"$HOME/AmigaChrome/stoves/os32-gcc16"}
PREFIX="$STOVE/prefix"
APP="$HOME/AmigaChrome"
INSTANCES="$HOME/AmigaChromeInstances"
INSTANCE="$INSTANCES/Instance-32"
DH1="$INSTANCE/devices/harddisks/DH1"
LAB_TOOL="$APP/scripts/lab_instance.py"
LAB_ROOT="$HOME/AmigaChrome-dev/astromenace-clean-labs"
WORK="$ROOT/build/os3/astromenace-clean-rebuild"
EVIDENCE="$WORK/evidence"
PROD_OUT="$WORK/production"
DIAG_OUT="$WORK/diagnostic"
PROD_PKG="$PROD_OUT/package/AstroMenace"
DIAG_PKG="$DIAG_OUT/package/AstroMenace"
RELEASE_ID=0011
ACTIVE_LAB=""

section()
{
    printf '\n============================================================\n'
    printf '%s\n' "$*"
    printf '============================================================\n'
}

cleanup_lab()
{
    if [[ -n "$ACTIVE_LAB" ]]; then
        python3 "$LAB_TOOL" stop "$ACTIVE_LAB" || true
        ACTIVE_LAB=""
    fi
}
trap cleanup_lab EXIT

section "Verify daletop, toolchain and Instance-32"
[[ "$(hostname)" == "daletop" ]]
[[ -x "$PREFIX/bin/m68k-amigaos-gcc" ]]
[[ -x "$PREFIX/bin/m68k-amigaos-g++" ]]
[[ -x "$PREFIX/bin/m68k-amigaos-nm" ]]
[[ -x "$PREFIX/bin/sdl2-config" ]]
[[ -x "$LAB_TOOL" ]]
[[ -d "$DH1" ]]
"$PREFIX/bin/m68k-amigaos-gcc" --version | head -1
"$PREFIX/bin/sdl2-config" --version

rm -rf "$WORK"
mkdir -p "$EVIDENCE" "$LAB_ROOT"

section "Inventory existing DH1 AstroMenace material"
python3 - "$DH1" "$EVIDENCE/dh1-before.txt" <<'PY'
from pathlib import Path
import hashlib
import sys

root = Path(sys.argv[1])
out = Path(sys.argv[2])
lines = []
for child in sorted(root.iterdir(), key=lambda p: p.name.lower()):
    key = ''.join(c for c in child.name.lower() if c.isalnum())
    if not key.startswith('astromenace'):
        continue
    kind = 'dir' if child.is_dir() else 'file'
    lines.append(f'{child.name}\t{kind}')
    if child.is_file():
        digest = hashlib.sha256(child.read_bytes()).hexdigest()
        lines.append(f'  sha256 {digest} {child.stat().st_size} bytes')
    elif child.is_dir():
        for rel in (
            'AstroMenace', 'AstroMenace.bin', 'gamedata.vfs',
            'AstroMenace-run.log', 'AstroMenace-production.log', 'SHA256SUMS'
        ):
            path = child / rel
            if path.is_file():
                digest = hashlib.sha256(path.read_bytes()).hexdigest()
                lines.append(f'  {rel}\t{path.stat().st_size}\t{digest}')
if not lines:
    lines.append('(no existing AstroMenace material)')
out.write_text('\n'.join(lines) + '\n', encoding='utf-8')
print(out.read_text(), end='')
PY

section "Prepare two pristine AstroMenace source trees"
rm -rf \
    build/game-ports/sources/astromenace \
    build/game-ports/sources/astromenace-production \
    build/game-ports/sources/astromenace-diagnostic
python3 scripts/game_port_prepare.py prepare \
    --root "$ROOT" --game astromenace --allow-network
python3 scripts/game_port_prepare.py verify \
    --root "$ROOT" --game astromenace
cp -a build/game-ports/sources/astromenace \
      build/game-ports/sources/astromenace-production
cp -a build/game-ports/sources/astromenace \
      build/game-ports/sources/astromenace-diagnostic

section "Build production AstroMenace AC090 GCC16 $RELEASE_ID"
STOVE="$STOVE" \
JOBS=4 \
ASTROMENACE_RELEASE_ID="$RELEASE_ID" \
ASTROMENACE_DIAGNOSTICS=0 \
sh ports/astromenace/build-amigaos3-runtime-fixed.sh \
    build/game-ports/sources/astromenace-production \
    "$PROD_OUT"

section "Build diagnostic AstroMenace AC090 GCC16 $RELEASE_ID"
STOVE="$STOVE" \
JOBS=4 \
ASTROMENACE_RELEASE_ID="$RELEASE_ID" \
ASTROMENACE_DIAGNOSTICS=1 \
sh ports/astromenace/build-amigaos3-runtime-fixed.sh \
    build/game-ports/sources/astromenace-diagnostic \
    "$DIAG_OUT"

section "Verify launchers, game binaries and canonical VFS"
for package in "$PROD_PKG" "$DIAG_PKG"; do
    [[ -x "$package/AstroMenace" ]]
    [[ -x "$package/AstroMenace.bin" ]]
    [[ -s "$package/gamedata.vfs" ]]
    (
        cd "$package"
        sha256sum -c SHA256SUMS
    )
done
cmp "$PROD_PKG/gamedata.vfs" "$DIAG_PKG/gamedata.vfs"
strings "$PROD_PKG/AstroMenace" | grep -F \
    'AMLAUNCH: AstroMenace.bin with 1048576-byte child stack'
strings "$DIAG_PKG/AstroMenace" | grep -F \
    'AMLAUNCH: AstroMenace.bin with 1048576-byte child stack'
"$PREFIX/bin/m68k-amigaos-nm" -C "$PROD_PKG/AstroMenace.bin" \
    | grep -F 'AMRunMainWithStack'
"$PREFIX/bin/m68k-amigaos-nm" -C "$PROD_PKG/AstroMenace.bin" \
    | grep -F 'AstroMenaceBody'
strings "$PROD_PKG/AstroMenace.bin" | grep -F 'AMDBG: '
if strings "$PROD_PKG/AstroMenace.bin" \
    | grep -Fq 'stage-first-game-frame-presented'; then
    echo 'production game binary unexpectedly contains qualification probes' >&2
    exit 1
fi
strings "$DIAG_PKG/AstroMenace.bin" \
    | grep -F 'stage-first-game-frame-presented'
strings "$DIAG_PKG/AstroMenace.bin" \
    | grep -F 'stage-shutdown-complete'
grep -F 'Startup stack: launcher creates a 1 MiB child' \
    "$PROD_PKG/BUILD-INFO.txt"
sha256sum \
    "$PROD_PKG/AstroMenace" \
    "$PROD_PKG/AstroMenace.bin" \
    "$PROD_PKG/gamedata.vfs" \
    "$DIAG_PKG/AstroMenace" \
    "$DIAG_PKG/AstroMenace.bin" \
    "$DIAG_PKG/gamedata.vfs" \
    | tee "$EVIDENCE/fresh-build-sha256.txt"

section "Qualify full diagnostic journey from a default small-stack Shell"
DIAG_LAB="$LAB_ROOT/Instance-Astro-Diag-$RUN_ID"
rm -rf "$DIAG_LAB"
python3 "$LAB_TOOL" copy Instance-32 "$DIAG_LAB" --who ChatGPT
ACTIVE_LAB="$DIAG_LAB"
DIAG_TARGET="$DIAG_LAB/devices/harddisks/DH1/AstroMenace-DIAG"
rm -rf "$DIAG_TARGET"
mkdir -p "$DIAG_TARGET"
cp -a "$DIAG_PKG/." "$DIAG_TARGET/"
chmod 755 "$DIAG_TARGET/AstroMenace" "$DIAG_TARGET/AstroMenace.bin"
: > "$DIAG_TARGET/AUTOTEST_FIRST_MISSION"

DIAG_SCRIPT="$DIAG_LAB/devices/harddisks/DH0/S/AstroMenace-AutoTest"
DIAG_USER_STARTUP="$DIAG_LAB/devices/harddisks/DH0/S/User-Startup"
mkdir -p "$(dirname "$DIAG_SCRIPT")"
cat > "$DIAG_SCRIPT" <<'AMIGA'
FailAt 21
Wait 5
CD DH1:AstroMenace-DIAG
Echo "ASTROTEST: starting launcher without a Stack command" >AstroMenace-run.log
AstroMenace >>AstroMenace-run.log
Echo "ASTROTEST: returned RC $RC" >>AstroMenace-run.log
AMIGA
touch "$DIAG_USER_STARTUP"
printf '\nRun >NIL: <NIL: Execute S:AstroMenace-AutoTest\n' \
    >> "$DIAG_USER_STARTUP"

python3 "$LAB_TOOL" start "$DIAG_LAB" --who ChatGPT \
    --purpose "AstroMenace $RELEASE_ID launcher full journey $RUN_ID"

diag_clean_exit=0
for attempt in $(seq 1 90); do
    if [[ -f "$DIAG_TARGET/stage-shutdown-complete" ]] \
       && [[ -f "$DIAG_TARGET/AstroMenace-run.log" ]] \
       && grep -Fq 'ASTROTEST: returned RC 0' \
            "$DIAG_TARGET/AstroMenace-run.log"; then
        diag_clean_exit=1
        echo "Diagnostic completed after $((attempt * 5)) seconds."
        sleep 2
        break
    fi
    if [[ -f "$DIAG_TARGET/AstroMenace-run.log" ]] \
       && grep -Fq 'ASTROTEST: returned RC' \
            "$DIAG_TARGET/AstroMenace-run.log" \
       && ! grep -Fq 'ASTROTEST: returned RC 0' \
            "$DIAG_TARGET/AstroMenace-run.log"; then
        echo 'Diagnostic returned a non-zero code.'
        break
    fi
    sleep 5
done

DIAG_EVIDENCE="$EVIDENCE/diagnostic"
mkdir -p "$DIAG_EVIDENCE"
cp "$DIAG_TARGET/AstroMenace-run.log" "$DIAG_EVIDENCE/" 2>/dev/null || true
cp "$DIAG_TARGET/AstroMenace-first-frame.bmp" "$DIAG_EVIDENCE/" 2>/dev/null || true
cp "$DIAG_TARGET/AstroMenace-workshop.bmp" "$DIAG_EVIDENCE/" 2>/dev/null || true
cp "$DIAG_TARGET/AstroMenace-first-mission.bmp" "$DIAG_EVIDENCE/" 2>/dev/null || true
cp "$DIAG_LAB/logs/native-runtime.log" "$DIAG_EVIDENCE/" 2>/dev/null || true
cp "$DIAG_LAB/state/current-screen.jpg" "$DIAG_EVIDENCE/" 2>/dev/null || true
cp "$DIAG_TARGET"/stage-* "$DIAG_EVIDENCE/" 2>/dev/null || true
cleanup_lab

[[ "$diag_clean_exit" -eq 1 ]]
[[ -s "$DIAG_EVIDENCE/AstroMenace-run.log" ]]
grep -F 'ASTROTEST: starting launcher without a Stack command' \
    "$DIAG_EVIDENCE/AstroMenace-run.log"
grep -F 'AMLAUNCH: AstroMenace.bin with 1048576-byte child stack' \
    "$DIAG_EVIDENCE/AstroMenace-run.log"
grep -F 'AMDBG: entered main' "$DIAG_EVIDENCE/AstroMenace-run.log"
grep -F 'AMDBG: after VFS open' "$DIAG_EVIDENCE/AstroMenace-run.log"
grep -F 'AMDBG: window and GL context ready' \
    "$DIAG_EVIDENCE/AstroMenace-run.log"
grep -F 'AMDBG: main menu ready' "$DIAG_EVIDENCE/AstroMenace-run.log"
grep -F 'ASTROTEST: returned RC 0' "$DIAG_EVIDENCE/AstroMenace-run.log"
for marker in \
    stage-first-frame-presented \
    stage-workshop-frame-presented \
    stage-first-game-frame-presented \
    stage-audio-channel-active \
    stage-keydown-received \
    stage-keyup-received \
    stage-shutdown-complete; do
    [[ -f "$DIAG_EVIDENCE/$marker" ]]
done
for frame in \
    AstroMenace-first-frame.bmp \
    AstroMenace-workshop.bmp \
    AstroMenace-first-mission.bmp; do
    [[ -s "$DIAG_EVIDENCE/$frame" ]]
    python3 scripts/validate_bmp_frame.py "$DIAG_EVIDENCE/$frame" \
        | tee "$DIAG_EVIDENCE/$frame.validation.json"
done
sha256sum "$DIAG_EVIDENCE"/*.bmp \
    | tee "$DIAG_EVIDENCE/frame-sha256.txt"

section "Remove every old DH1 AstroMenace copy and install fresh production"
python3 - "$DH1" "$EVIDENCE/dh1-removed.txt" <<'PY'
from pathlib import Path
import shutil
import sys

root = Path(sys.argv[1])
out = Path(sys.argv[2])
removed = []
for child in sorted(root.iterdir(), key=lambda p: p.name.lower()):
    key = ''.join(c for c in child.name.lower() if c.isalnum())
    if not key.startswith('astromenace'):
        continue
    removed.append(child.name)
    if child.is_dir() and not child.is_symlink():
        shutil.rmtree(child)
    else:
        child.unlink()
out.write_text('\n'.join(removed or ['(nothing removed)']) + '\n', encoding='utf-8')
print(out.read_text(), end='')
PY

INCOMING="$DH1/.AstroMenace-$RELEASE_ID-incoming-$RUN_ID"
rm -rf "$INCOMING"
mkdir -p "$INCOMING"
cp -a "$PROD_PKG/." "$INCOMING/"
chmod 755 "$INCOMING/AstroMenace" "$INCOMING/AstroMenace.bin"
[[ -s "$INCOMING/AstroMenace" ]]
[[ -s "$INCOMING/AstroMenace.bin" ]]
[[ -s "$INCOMING/gamedata.vfs" ]]
mv "$INCOMING" "$DH1/AstroMenace"
INSTALL="$DH1/AstroMenace"
(
    cd "$INSTALL"
    sha256sum -c SHA256SUMS
)
cmp "$PROD_PKG/AstroMenace" "$INSTALL/AstroMenace"
cmp "$PROD_PKG/AstroMenace.bin" "$INSTALL/AstroMenace.bin"
cmp "$PROD_PKG/gamedata.vfs" "$INSTALL/gamedata.vfs"

section "Smoke-test exact installed production copy without Stack command"
PROD_LAB="$LAB_ROOT/Instance-Astro-Prod-$RUN_ID"
rm -rf "$PROD_LAB"
python3 "$LAB_TOOL" copy Instance-32 "$PROD_LAB" --who ChatGPT
ACTIVE_LAB="$PROD_LAB"
PROD_TARGET="$PROD_LAB/devices/harddisks/DH1/AstroMenace"
PROD_SCRIPT="$PROD_LAB/devices/harddisks/DH0/S/AstroMenace-ProductionTest"
PROD_USER_STARTUP="$PROD_LAB/devices/harddisks/DH0/S/User-Startup"
[[ -x "$PROD_TARGET/AstroMenace" ]]
[[ -x "$PROD_TARGET/AstroMenace.bin" ]]
rm -f "$PROD_TARGET/AstroMenace-production.log"
rm -rf "$PROD_TARGET/User"
mkdir -p "$(dirname "$PROD_SCRIPT")"
cat > "$PROD_SCRIPT" <<'AMIGA'
FailAt 21
Wait 5
CD DH1:AstroMenace
Echo "ASTROPROD: starting exact installed launcher without a Stack command" >AstroMenace-production.log
AstroMenace >>AstroMenace-production.log
Echo "ASTROPROD: returned RC $RC" >>AstroMenace-production.log
AMIGA
touch "$PROD_USER_STARTUP"
printf '\nRun >NIL: <NIL: Execute S:AstroMenace-ProductionTest\n' \
    >> "$PROD_USER_STARTUP"

python3 "$LAB_TOOL" start "$PROD_LAB" --who ChatGPT \
    --purpose "AstroMenace $RELEASE_ID exact installed production smoke $RUN_ID"

menu_ready=0
for attempt in $(seq 1 75); do
    if [[ -f "$PROD_TARGET/AstroMenace-production.log" ]] \
       && grep -Fq 'AMDBG: main menu ready' \
            "$PROD_TARGET/AstroMenace-production.log"; then
        menu_ready=1
        echo "Production copy reached the main menu after $((attempt * 5)) seconds."
        sleep 5
        break
    fi
    if [[ -f "$PROD_TARGET/AstroMenace-production.log" ]] \
       && grep -Fq 'ASTROPROD: returned RC' \
            "$PROD_TARGET/AstroMenace-production.log"; then
        echo 'Production copy returned before reaching the menu.'
        break
    fi
    sleep 5
done

PROD_EVIDENCE="$EVIDENCE/production"
mkdir -p "$PROD_EVIDENCE"
cp "$PROD_TARGET/AstroMenace-production.log" "$PROD_EVIDENCE/" 2>/dev/null || true
cp "$PROD_LAB/logs/native-runtime.log" "$PROD_EVIDENCE/" 2>/dev/null || true
cp "$PROD_LAB/state/current-screen.jpg" "$PROD_EVIDENCE/" 2>/dev/null || true
sha256sum \
    "$PROD_TARGET/AstroMenace" \
    "$PROD_TARGET/AstroMenace.bin" \
    "$PROD_TARGET/gamedata.vfs" \
    > "$PROD_EVIDENCE/installed-sha256.txt"
cleanup_lab

[[ "$menu_ready" -eq 1 ]]
[[ -s "$PROD_EVIDENCE/AstroMenace-production.log" ]]
grep -F 'ASTROPROD: starting exact installed launcher without a Stack command' \
    "$PROD_EVIDENCE/AstroMenace-production.log"
grep -F 'AMLAUNCH: AstroMenace.bin with 1048576-byte child stack' \
    "$PROD_EVIDENCE/AstroMenace-production.log"
grep -F 'AMDBG: entered main' "$PROD_EVIDENCE/AstroMenace-production.log"
grep -F 'AMDBG: after VFS open' "$PROD_EVIDENCE/AstroMenace-production.log"
grep -F 'AMDBG: window and GL context ready' \
    "$PROD_EVIDENCE/AstroMenace-production.log"
grep -F 'AMDBG: after asset load' "$PROD_EVIDENCE/AstroMenace-production.log"
grep -F 'AMDBG: main menu ready' "$PROD_EVIDENCE/AstroMenace-production.log"
if grep -Fq 'ASTROPROD: returned RC' \
    "$PROD_EVIDENCE/AstroMenace-production.log"; then
    echo 'Production process exited unexpectedly during the smoke window.' >&2
    exit 1
fi

section "Audit clean DH1 install and package release archive"
python3 - "$DH1" "$EVIDENCE/dh1-after.txt" <<'PY'
from pathlib import Path
import sys

root = Path(sys.argv[1])
out = Path(sys.argv[2])
lines = []
for child in sorted(root.iterdir(), key=lambda p: p.name.lower()):
    key = ''.join(c for c in child.name.lower() if c.isalnum())
    if key.startswith('astromenace'):
        lines.append(f'{child.name}\t{"dir" if child.is_dir() else "file"}')
out.write_text('\n'.join(lines) + '\n', encoding='utf-8')
print(out.read_text(), end='')
PY
[[ "$(wc -l < "$EVIDENCE/dh1-after.txt")" -eq 1 ]]
grep -Fx $'AstroMenace\tdir' "$EVIDENCE/dh1-after.txt"
[[ ! -e "$DH1/AstroMenace-DIAG" ]]
[[ ! -e "$DH1/AstroMenace-AC090-GCC16-0010.zip" ]]
[[ -x "$INSTALL/AstroMenace" ]]
[[ -x "$INSTALL/AstroMenace.bin" ]]
[[ -s "$INSTALL/gamedata.vfs" ]]
grep -F "Release: AC090 GCC16 $RELEASE_ID" \
    "$INSTALL/README-AmigaChrome.txt"
grep -F 'No preceding Stack command is' \
    "$INSTALL/README-AmigaChrome.txt"

python3 - "$PROD_OUT/package" "$WORK/AstroMenace-AC090-GCC16-$RELEASE_ID.zip" <<'PY'
from pathlib import Path
import sys
import zipfile

source = Path(sys.argv[1])
destination = Path(sys.argv[2])
with zipfile.ZipFile(destination, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
    for path in sorted(source.rglob('*')):
        if path.is_file():
            archive.write(path, path.relative_to(source))
print(destination)
PY
sha256sum "$WORK/AstroMenace-AC090-GCC16-$RELEASE_ID.zip" \
    | tee "$WORK/AstroMenace-AC090-GCC16-$RELEASE_ID.zip.sha256"

cat > "$EVIDENCE/RESULT.txt" <<EOF
AstroMenace AC090 GCC16 $RELEASE_ID: PASS

- pristine production and diagnostic rebuilds: PASS
- default small-stack Shell launcher: PASS
- pre-CRT 1 MiB child process stack: PASS
- VFS, SDL2, OpenAudio and VirGL startup: PASS
- menu, workshop and first mission journey: PASS
- audio and keyboard probes: PASS
- clean diagnostic return code 0: PASS
- exact installed production copy reached main menu: PASS
- old DH1 AstroMenace copies removed: PASS
- final DH1 location: DH1:AstroMenace
EOF
cat "$EVIDENCE/RESULT.txt"
