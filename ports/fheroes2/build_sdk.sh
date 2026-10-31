#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 AmigaChrome Team
#
# The SDK fheroes2 builds against, from GitHub-hosted inputs only:
#   - OpenAmigaGCC 16.2.0-open3 (compiler, libnix, sfdc) from its release;
#   - OpenNDK, the Team's own NDK, from the open3 source release, installed
#     as the compiler's ndk-include (no vendor NDK);
#   - the CyberGraphX and AHI developer headers SDL's back ends need, read
#     from the AROS tree at the commit toolchain/aros-local/lock.env pins;
#   - OpenRTG's SDL2.module, libSDL2.a, SDL2_mixer.module and libSDL2_mixer.a.
# Every download is checked against a pinned commit or SHA-256. The output is
# a local build workspace, not something to publish: it holds third-party
# developer headers.
#
#   bash ports/fheroes2/build_sdk.sh OUT_DIR
set -euo pipefail
[[ $# == 1 ]] || { echo "Usage: $0 OUT_DIR" >&2; exit 2; }
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
mkdir -p "$1"
OUT=$(cd "$1" && pwd)
JOBS=${JOBS:-$(nproc)}
# shellcheck source=/dev/null
source "$ROOT/toolchain/aros-local/lock.env"

GCC_TAG=v16.2.0-open3
GCC_BIN=openamigagcc-16.2.0-open3-x86_64-linux.tar.xz
GCC_BIN_SHA=9fcc136ab59e3bb5b91d2764d0bf290b709d72d7380be4ce0cdf005edf8eff21
GCC_SRC=openamigagcc-16.2.0-open3-src.tar.xz
GCC_SRC_SHA=d8b7cf2101cf70df1cf719d199230260790420f43ec6e899f3d061a85ff3bcda
OPENRTG_COMMIT=8b6d09ff1023edc3fb1e62e3db0490ec385820b2
VASM_COMMIT=a13e7e728a3dd5a9ba468bf479119e0bc23e70fd

fetch() {  # url file sha256
    if [[ ! -f "$2" ]]; then
        curl --fail --location --retry 2 --connect-timeout 20 --max-time 600 "$1" -o "$2.part"
        mv "$2.part" "$2"
    fi
    printf '%s  %s\n' "$3" "$2" | sha256sum --check --status || { echo "hash mismatch: $2" >&2; exit 3; }
}
checkout() {  # url dir commit
    [[ -d "$2/.git" ]] || git clone -q --filter=blob:none --no-checkout "$1" "$2"
    git -C "$2" cat-file -e "$3^{commit}" 2>/dev/null || git -C "$2" fetch -q origin "$3"
    git -C "$2" checkout -q --detach "$3"
}

# 1. Compiler and source release.
mkdir -p "$OUT/downloads"
fetch "https://github.com/DalsinAI/openamigagcc/releases/download/$GCC_TAG/$GCC_BIN" "$OUT/downloads/$GCC_BIN" $GCC_BIN_SHA
fetch "https://github.com/DalsinAI/openamigagcc/releases/download/$GCC_TAG/$GCC_SRC" "$OUT/downloads/$GCC_SRC" $GCC_SRC_SHA
PREFIX=$OUT/prefix
SRC=$OUT/openamigagcc-src
if [[ ! -x "$PREFIX/bin/m68k-amigaos-gcc" ]]; then
    rm -rf "$PREFIX.tmp"; mkdir -p "$PREFIX.tmp"
    tar --no-same-owner -xJf "$OUT/downloads/$GCC_BIN" -C "$PREFIX.tmp" --strip-components=1
    mv "$PREFIX.tmp" "$PREFIX"
fi
if [[ ! -d "$SRC/amiga-gcc" ]]; then
    rm -rf "$SRC.tmp"; mkdir -p "$SRC.tmp"
    tar --no-same-owner -xJf "$OUT/downloads/$GCC_SRC" -C "$SRC.tmp" --strip-components=1
    mv "$SRC.tmp" "$SRC"
fi
export PATH="$PREFIX/bin:$PATH"
OPENSDK=$SRC/amiga-gcc/projects/openamigasdk

# 2. OpenNDK as the compiler's NDK. The compiler searches
#    m68k-amigaos/ndk-include after libnix's headers.
NDKINC=$PREFIX/m68k-amigaos/ndk-include
rm -rf "$NDKINC"; mkdir -p "$NDKINC"
cp -a "$OPENSDK/OpenNDK/Include_H/." "$NDKINC/"
cp -a "$OPENSDK/Include/." "$NDKINC/"
STOVE=$PREFIX sh "$OPENSDK/OpenNDK/lib/build.sh" >/dev/null
cp "$OPENSDK/OpenNDK/lib/build/m68k-amigaos/libamiga.a" "$PREFIX/m68k-amigaos/lib/libamiga.a"

# 3. CyberGraphX and AHI developer headers from the pinned AROS tree. The
#    CyberGraphX calls come from AROS's cybergraphics.conf (conf_to_sfd.py).
AROS=$OUT/aros
checkout "$AROS_REPOSITORY" "$AROS" "$AROS_REF" 2>/dev/null || true
git -C "$AROS" checkout -q "$AROS_REF" -- \
    workbench/devs/diskimage/include/amigaos/cybergraphx/cybergraphics.h \
    workbench/devs/diskimage/include/amigaos/clib/cybergraphics_protos.h \
    workbench/libs/cgfx/cybergraphics.conf \
    workbench/devs/AHI/Include/C workbench/devs/AHI/Include/SFD
H=$OUT/sdk-headers
rm -rf "$H"; mkdir -p "$H"/{cybergraphx,clib,inline,proto,devices,libraries}
cp "$AROS/workbench/devs/diskimage/include/amigaos/cybergraphx/cybergraphics.h" "$H/cybergraphx/"
cp "$AROS/workbench/devs/diskimage/include/amigaos/clib/cybergraphics_protos.h" "$H/clib/"
cp "$AROS/workbench/devs/AHI/Include/C/devices/ahi.h" "$H/devices/"
cp "$AROS/workbench/devs/AHI/Include/C/libraries/ahi_sub.h" "$H/libraries/"
# AROS's copies say IPTR, its pointer-sized integer: ULONG on 68k AmigaOS.
sed -i 's/\bIPTR\b/ULONG/g' "$H"/cybergraphx/*.h "$H"/clib/*.h "$H"/devices/*.h "$H"/libraries/*.h
python3 "$HERE/conf_to_sfd.py" "$AROS/workbench/libs/cgfx/cybergraphics.conf" "$H/cybergraphics_lib.sfd" \
    --base CyberGfxBase --libname cybergraphics.library \
    --include '<exec/types.h>' --include '<utility/tagitem.h>' --include '<utility/hooks.h>' \
    --include '<graphics/rastport.h>' --include '<graphics/view.h>'
AHI_SFD=$AROS/workbench/devs/AHI/Include/SFD/ahi_lib.sfd
sfdc -q --target=m68k-gcc-amigaos --mode=macros --output="$H/inline/cybergraphics.h" "$H/cybergraphics_lib.sfd"
sfdc -q --target=m68k-amigaos --mode=proto --output="$H/proto/cybergraphics.h" "$H/cybergraphics_lib.sfd"
sfdc -q --target=m68k-gcc-amigaos --mode=macros --output="$H/inline/ahi.h" "$AHI_SFD"
sfdc -q --target=m68k-amigaos --mode=proto --output="$H/proto/ahi.h" "$AHI_SFD"
sfdc -q --target=m68k-amigaos --mode=clib --output="$H/clib/ahi_protos.h" "$AHI_SFD"
python3 "$SRC/openamigagcc/scripts/fix_sfdc_base_order.py" "$H/inline/cybergraphics.h" "$H/inline/ahi.h"
cp -a "$H/." "$NDKINC/"

# 4. vasm for SDL's c2p and blitter sources.
checkout https://github.com/mbitsnbites/vasm-mirror.git "$OUT/vasm" $VASM_COMMIT
make -s -C "$OUT/vasm" CPU=m68k SYNTAX=mot -j"$JOBS" >/dev/null

# 5. OpenRTG's SDL2 and SDL2_mixer. FreeType (for SDL2_ttf) is not needed by
#    fheroes2 and is left out; the other satellites give the headers.
RTG=$OUT/openamigartg
checkout https://github.com/DalsinAI/openamigartg.git "$RTG" $OPENRTG_COMMIT
SDLOUT=$OUT/sdl2
SDLDIR=$RTG/library/modules/sdl2
STOVE=$PREFIX SDK_HEADERS=$H VASM=$OUT/vasm/vasmm68k_mot TARBALLS=$OUT/downloads JOBS=$JOBS \
    sh "$SDLDIR/build.sh" "$SDLOUT" module stub
pin() { python3 -c "import json,sys; print(json.load(open('$SDLDIR/UPSTREAM.json'))[sys.argv[1]][sys.argv[2]])" "$1" "$2"; }
for lib in sdl2_image sdl2_mixer sdl2_ttf sdl2_net libxmp; do
    t=$OUT/downloads/$(pin $lib file)
    fetch "$(pin $lib url)" "$t" "$(pin $lib sha256)"
    rm -rf "$SDLOUT/sdl2-work/$(pin $lib dir)"
    tar xf "$t" -C "$SDLOUT/sdl2-work"
    for p in "$SDLDIR"/patches/$lib/*.patch; do
        [[ -f "$p" ]] && patch -s -p1 -d "$SDLOUT/sdl2-work/$(pin $lib dir)" < "$p"
    done
done
make -s -f "$SDLDIR/satellites/Makefile" -j"$JOBS" STOVE="$PREFIX" OUT="$SDLOUT" WORK="$SDLOUT/sdl2-work" \
    headers abi "$SDLOUT/SDL2_mixer.module" "$SDLOUT/libSDL2_mixer.a"
python3 "$SDLDIR/fpcr_check.py" "$PREFIX/bin/m68k-amigaos-objdump" \
    "$SDLOUT/SDL2.module" "$SDLOUT/SDL2_mixer.module" "$SDLOUT/libSDL2.a" "$SDLOUT/libSDL2_mixer.a"

# 6. One sysroot overlay for CMake: SDL headers and link stubs.
SYS=$OUT/sysroot
rm -rf "$SYS"; mkdir -p "$SYS/include" "$SYS/lib"
cp -a "$SDLOUT/include/SDL2" "$SYS/include/"
cp "$SDLOUT/libSDL2.a" "$SDLOUT/libSDL2_mixer.a" "$SYS/lib/"
(cd "$SDLOUT" && sha256sum SDL2.module SDL2_mixer.module libSDL2.a libSDL2_mixer.a) > "$OUT/SHA256SUMS"
printf 'PREFIX=%s\nSYSROOT=%s\nOPENSDK=%s\n' "$PREFIX" "$SYS" "$OPENSDK" > "$OUT/sdk.env"
cat "$OUT/SHA256SUMS"
echo "fheroes2 SDK ready: $OUT/sdk.env"
