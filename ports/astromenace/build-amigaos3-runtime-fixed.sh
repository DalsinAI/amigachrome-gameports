#!/bin/sh
set -eu

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
SRC=${1:-"$ROOT/build/game-ports/sources/astromenace"}
OUT=${2:-"$ROOT/build/os3/astromenace-runtime-diagnostic"}

# Apply the normal AmigaChrome integration first because the target-random
# patch adds its post-SDL reseed immediately after the diagnostic SDL marker.
python3 "$HERE/patch_amigaos3.py" "$SRC" "$HERE/audio_amigachrome.cpp"
python3 "$HERE/patch_amiga_random.py" "$SRC"
python3 "$HERE/patch_first_frame_probe.py" "$SRC"

# -DAMIGACHROME=ON is a CMake option, not a C/C++ preprocessor definition.
# Without this target definition every #ifdef AMIGACHROME path (including the
# raw AMDBG markers and target-safe random seed) was compiled out.
python3 "$HERE/patch_amiga_compile_define.py" "$SRC"

# Existing AstroMenace VFS, VW3D models, VW2D textures and TGA headers are
# little-endian.  Make the target readers and recovery writers explicit
# instead of treating on-disk bytes as native big-endian 68k values.
python3 "$HERE/patch_vfs_little_endian.py" "$SRC"
python3 "$HERE/patch_vw3d_little_endian.py" "$SRC"
python3 "$HERE/patch_vw2d_little_endian.py" "$SRC"
python3 "$HERE/patch_tga_little_endian.py" "$SRC"

# Keep the regular builder as the compiler/library/package authority.
sh "$HERE/build-amigaos3.sh" "$@"

# Do not make the AC090 spend its first launch rebuilding tens of megabytes of
# redistributable data. Ship a verified canonical VFS beside the executable;
# the corrected target-side packer remains available as a recovery path.
python3 "$HERE/build_gamedata_vfs.py" \
    "$SRC" \
    "$OUT/package/AstroMenace/gamedata.vfs"

sha256sum "$OUT/package/AstroMenace/gamedata.vfs"
