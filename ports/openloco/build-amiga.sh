#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 AmigaChrome Team
#
# Cradle's entry point (stove_build, target ports/openloco).
# OPENLOCO_STEP=inventory (this commit) only lists the inputs the build
# needs on the build host; it builds nothing.
[ -n "${BASH_VERSION:-}" ] || exec bash "$0" "$@"
set -uo pipefail
: "${OUT:?OUT must be set}"
mkdir -p "$OUT"
{
echo "host=$(hostname) user=$(id -un) STOVE=${STOVE:-unset} OUT=$OUT pwd=$PWD"
echo "== stove"; ls -la "${STOVE:-/nonexistent}" 2>&1 | head -20
ls "${STOVE:-/x}"/prefix/m68k-amigaos/lib 2>/dev/null | head -60
"${STOVE:-/x}"/bin/m68k-amigaos-gcc --version 2>/dev/null | head -1
"${STOVE:-/x}"/prefix/bin/m68k-amigaos-gcc --version 2>/dev/null | head -1
echo "== stoves dir"; ls -la ~/AmigaChrome/stoves 2>&1
for s in ~/AmigaChrome/stoves/*; do echo "-- $s"; ls "$s" | head; [ -f "$s/provenance.json" ] && head -c 400 "$s/provenance.json"; "$s"/prefix/bin/m68k-amigaos-gcc --version 2>/dev/null | head -1; done
echo "== provenance.json (open3 stoves)"
timeout 120 find ~ /srv /run/media/da1ek/WorkingSpace/AmigaChrome -maxdepth 6 -name provenance.json -not -path '*/node_modules/*' 2>/dev/null | head -40
echo "== SDL3 SDK outputs"
timeout 120 find ~ /srv -maxdepth 7 \( -name libSDL3.a -o -name SDL3.module \) 2>/dev/null | head -40
echo "== openloco trees"
timeout 120 find ~ /srv -maxdepth 6 -iname '*openloco*' -not -path '*/cradle-builds/*' 2>/dev/null | head -60
echo "== staged codec manifests"
timeout 60 find ~ -maxdepth 7 -path '*port-dependencies*' -name manifest.json 2>/dev/null | head
echo "== upstream tarballs"; ls ~/AmigaChrome-dev/upstream-tarballs 2>&1 | head -40
echo "== Locomotion data"
timeout 180 find /run/media/da1ek/WorkingSpace/AmigaChrome/mediastore -maxdepth 5 \( -iname '*loco*' -o -iname 'g1.dat' -o -iname 'title.dat' \) 2>/dev/null | head -40
echo "== dev reachable?"; timeout 10 ssh -o BatchMode=yes -o ConnectTimeout=5 192.168.0.168 'hostname; ls ~/team3 | head -40; find ~ -maxdepth 5 \( -iname "*openloco*" -o -name libSDL3.a \) 2>/dev/null | head -40' 2>&1
echo "== disk"; df -h ~ /run/media/da1ek/WorkingSpace 2>&1
} 2>&1 | tee "$OUT/inventory.txt"
exit 0
