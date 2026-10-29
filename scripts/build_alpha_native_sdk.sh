#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Build the real SDL/OpenGPU dependencies used by the Alpha frontend lab.
set -euo pipefail
if [[ $# != 1 ]]; then
  echo "Usage: GCC_PREFIX=... GCC_SOURCE=... $0 OUTPUT_DIRECTORY" >&2
  exit 2
fi
: "${GCC_PREFIX:?Set GCC_PREFIX to the extracted OpenAmigaGCC open3 prefix}"
: "${GCC_SOURCE:?Set GCC_SOURCE to the extracted open3 source release root}"
GCC_PREFIX=$(cd "$GCC_PREFIX" && pwd)
GCC_SOURCE=$(cd "$GCC_SOURCE" && pwd)
mkdir -p "$1"
out=$(cd "$1" && pwd)
export PATH="$GCC_PREFIX/bin:$PATH"
export STOVE="$GCC_PREFIX"
export SDK_HEADERS="$out/combined-headers"
export VASM="$out/vasm/vasmm68k_mot"
export JOBS=${JOBS:-8}
mkdir -p "$out/downloads" "$out/build/sdl2-work/tarballs"
export TARBALLS="$out/build/sdl2-work/tarballs"

fetch() {
  local url=$1 file=$2 sum=$3
  if [[ ! -f "$file" ]]; then
    curl --fail --location --retry 2 --connect-timeout 20 --max-time 180 "$url" -o "$file.part"
    mv "$file.part" "$file"
  fi
  printf '%s  %s\n' "$sum" "$file" | sha256sum --check --status
}
checkout() {
  local url=$1 dest=$2 revision=$3
  if [[ ! -d "$dest/.git" ]]; then git clone --no-checkout "$url" "$dest"; fi
  git -C "$dest" cat-file -e "$revision^{commit}" 2>/dev/null || git -C "$dest" fetch origin "$revision"
  # Only this script's dependency checkouts, never the user's game source.
  git -C "$dest" checkout --detach "$revision"
}

checkout https://github.com/fragglet/lhasa.git "$out/lhasa" 75ed83559f23e9538e0045c62f53f77ab03d03d6
checkout https://github.com/mbitsnbites/vasm-mirror.git "$out/vasm" a13e7e728a3dd5a9ba468bf479119e0bc23e70fd
checkout https://github.com/DalsinAI/openamigartg.git "$out/openamigartg" fb9b556f15f73dc9b6fb1f3dc8f461508efb34fe

# Build lhasa directly from the upstream Makefile's source list; autotools is
# unnecessary. This is a host tool for the NDK install, not guest runtime code.
python3 - "$out/lhasa" <<'PY'
from pathlib import Path
import re, subprocess, sys
p = Path(sys.argv[1])
(p / 'config.h').write_text('#define PACKAGE_NAME "lhasa"\n#define PACKAGE_VERSION "pinned-alpha-lab"\n#define HAVE_FNMATCH_H 1\n')
s = (p / 'lib/Makefile.am').read_text().split('SRC =', 1)[1].split('liblhasatest', 1)[0]
library = re.findall(r'\b[\w]+\.c\b', s)
frontend = ['main.c', 'filter.c', 'list.c', 'extract.c', 'safe.c']
subprocess.run(['cc', '-O2', '-I' + str(p), '-I' + str(p / 'lib'), '-I' + str(p / 'lib/public'),
                *[str(p / 'lib' / f) for f in library], *[str(p / 'src' / f) for f in frontend],
                '-o', str(p / 'lha')], check=True)
PY
export PATH="$out/lhasa:$PATH"
make -C "$out/vasm" CPU=m68k SYNTAX=mot -j"$JOBS"

# The NDK is downloaded from its published location for local use. It is not
# part of this repository or any distributable SDK generated below.
fetch https://aminet.net/dev/misc/NDK3.2.lha "$out/downloads/NDK3.2.lha" 96cabd4ad683dced632e147bf86dee0f50dcb1254386216c25c362916a6409bb
bash "$GCC_SOURCE/openamigagcc/scripts/install-ndk.sh" "$GCC_SOURCE/amiga-gcc" "$GCC_PREFIX" "$out/downloads/NDK3.2.lha"
fetch https://aminet.net/dev/misc/CGraphX-DevKit.lha "$out/downloads/CGraphX-DevKit.lha" dd2610649a692863dc27bbce595520b0ee269a03e03f73a8858396127df0fb0c
fetch https://aminet.net/driver/audio/m68k-amigaos-ahidev.lha "$out/downloads/m68k-amigaos-ahidev.lha" de3571879210ad9ca179fbbc7a8c0591dfd0b673d6785657d8796fc1d7a147ec
mkdir -p "$out/headers"
(cd "$out/headers"; lha xf "$out/downloads/CGraphX-DevKit.lha"; lha xf "$out/downloads/m68k-amigaos-ahidev.lha")
mkdir -p "$SDK_HEADERS"
cp -a "$out/headers/CGraphX/C/Include/." "$SDK_HEADERS/"
cp -a "$out/headers/m68k-amigaos-ahi/Developer/Include/C/." "$SDK_HEADERS/"
fd2sfd "$out/headers/CGraphX/FD/cybergraphics_lib.fd" "$out/headers/CGraphX/C/Include/clib/cybergraphics_protos.h" "$SDK_HEADERS/cybergraphics_lib.sfd"
sfdc --target=m68k-gcc-amigaos --mode=macros --output="$SDK_HEADERS/inline/cybergraphics.h" "$SDK_HEADERS/cybergraphics_lib.sfd"
sfdc --target=m68k-amigaos --mode=proto --output="$SDK_HEADERS/proto/cybergraphics.h" "$SDK_HEADERS/cybergraphics_lib.sfd"
python3 "$GCC_SOURCE/openamigagcc/scripts/fix_sfdc_base_order.py" "$SDK_HEADERS/inline/cybergraphics.h"

# The repository's own script validates SDL's pinned tarball, applies its real
# Amiga backends and OpenGPU patches, and checks the module's A4 residency.
sh "$out/openamigartg/library/modules/sdl2/build.sh" "$out/build" stub module

# Only Alpha's font dependency is needed. The satellite Makefile also parses
# libxmp's source Makefile, even when building only SDL2_ttf.
python3 - "$out" <<'PY'
from pathlib import Path
import hashlib, json, subprocess, sys, tarfile
p = Path(sys.argv[1]); source = p / 'openamigartg/library/modules/sdl2'
pins = json.loads((source / 'UPSTREAM.json').read_text())
for key in ['sdl2_ttf', 'freetype', 'libxmp']:
    pin = pins[key]; archive = p / 'build/sdl2-work/tarballs' / pin['file']
    if not archive.exists():
        part = archive.with_name(archive.name + '.part')
        subprocess.run(['curl', '--fail', '--location', '--retry', '2', '--connect-timeout', '20',
                        '--max-time', '180', pin['url'], '-o', str(part)], check=True)
        part.rename(archive)
    if hashlib.sha256(archive.read_bytes()).hexdigest() != pin['sha256']:
        raise SystemExit('Archive hash mismatch: ' + str(archive))
    destination = p / 'build/sdl2-work' / pin['dir']
    # Do not silently reapply patches onto a partially built source tree.
    if not destination.exists():
        with tarfile.open(archive) as tf:
            tf.extractall(p / 'build/sdl2-work', filter='data')
        for patch in sorted((source / 'patches' / key).glob('*.patch')):
            with patch.open('rb') as stream:
                subprocess.run(['patch', '-s', '-p1', '-d', str(destination)], stdin=stream, check=True)
PY
make -f "$out/openamigartg/library/modules/sdl2/satellites/Makefile" -j"$JOBS" OUT="$out/build" WORK="$out/build/sdl2-work" "$out/build/libSDL2_ttf.a"
cp "$out/build/sdl2-work/SDL2_ttf-2.24.0/SDL_ttf.h" "$out/build/include/SDL2/"
cp -a "$SDK_HEADERS/." "$out/build/include/"
mkdir -p "$out/build/lib"
ln -sfn ../libSDL2.a "$out/build/lib/libSDL2.a"
ln -sfn ../libSDL2_ttf.a "$out/build/lib/libSDL2_ttf.a"
python3 "$out/openamigartg/library/modules/sdl2/fpcr_check.py" "$GCC_PREFIX/bin/m68k-amigaos-objdump" "$out/build/SDL2.module" "$out/build/libSDL2.a" "$out/build/libSDL2_ttf.a"
sha256sum "$out/build/SDL2.module" "$out/build/libSDL2.a" "$out/build/libSDL2_ttf.a" > "$out/build/SHA256SUMS"
printf 'OPENUP_SDK=%s\nRuntime module: %s\n' "$out/build" "$out/build/SDL2.module"
