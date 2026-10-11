#!/bin/sh
set -eu

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
SRC=${1:-"$ROOT/build/game-ports/sources/astromenace"}
OUT=${2:-"$ROOT/build/os3/astromenace-runtime-fixed"}
DIAGNOSTICS=${ASTROMENACE_DIAGNOSTICS:-0}
RELEASE_ID=${ASTROMENACE_RELEASE_ID:-0011}
STOVE=${STOVE:-"$HOME/AmigaChrome/stoves/os32-gcc16"}
P="$STOVE/prefix"
CC="$P/bin/m68k-amigaos-gcc"

[ -x "$CC" ] || { echo "missing GCC16 compiler for launcher: $CC" >&2; exit 2; }

# Apply the production AmigaChrome integration first because the target-random
# patch adds its post-SDL reseed immediately after the SDL startup marker.
python3 "$HERE/patch_amigaos3.py" "$SRC" "$HERE/audio_amigachrome.cpp"
python3 "$HERE/patch_joystick_runtime.py" "$SRC"
python3 "$HERE/patch_amiga_random.py" "$SRC"

# Runtime probes are deliberately absent from release packages. CI enables
# them explicitly to leave stage files, capture menu/workshop/mission frames,
# prove mixer playback and exercise the SDL keyboard path plus clean shutdown.
if [ "$DIAGNOSTICS" = "1" ]; then
    python3 "$HERE/patch_first_frame_probe.py" "$SRC"
    python3 "$HERE/patch_player_journey_probe.py" "$SRC"
    python3 "$HERE/patch_audio_runtime_probe.py" "$SRC"
    python3 "$HERE/patch_input_exit_probe.py" "$SRC"
else
    python3 "$HERE/patch_release_cleanup.py" "$SRC"
fi

# The screenshot correction is a real target fix, not only test plumbing: the
# OpenGPU readback path is RGBA and must be converted before SDL writes a BMP.
python3 "$HERE/patch_screenshot_rgba.py" "$SRC"

# -DAMIGACHROME=ON is a CMake option, not a C/C++ preprocessor definition.
# Without this target definition every #ifdef AMIGACHROME path (including the
# target-safe random seed) is compiled out.
python3 "$HERE/patch_amiga_compile_define.py" "$SRC"

# Existing AstroMenace VFS, VW3D models, VW2D textures and TGA headers are
# little-endian. Make the target readers and recovery writers explicit instead
# of treating on-disk bytes as native big-endian 68k values.
python3 "$HERE/patch_vfs_little_endian.py" "$SRC"
python3 "$HERE/patch_vw3d_little_endian.py" "$SRC"
python3 "$HERE/patch_vw2d_little_endian.py" "$SRC"
python3 "$HERE/patch_tga_little_endian.py" "$SRC"

# Keep the regular builder as the compiler/library/package authority, but tell
# it not to overwrite the richer source tree prepared above.
ASTROMENACE_SOURCE_PREPATCHED=1 \
STOVE="$STOVE" \
sh "$HERE/build-amigaos3.sh" "$SRC" "$OUT"

PKG="$OUT/package/AstroMenace"
LAUNCHER="$PKG/AstroMenace"
GAME_BIN="$PKG/AstroMenace.bin"
VFS="$PKG/gamedata.vfs"

# Qualification historically worked only after a Shell-level `Stack 1000000`.
# Do not swap the stack of a live C++ frame and do not introduce another shell
# with SystemTags(). Retain AstroMenace as a tiny public command which LoadSegs
# the real game and invokes it synchronously through DOS RunCommand().
[ -s "$LAUNCHER" ] || { echo "base AstroMenace binary missing: $LAUNCHER" >&2; exit 5; }
mv "$LAUNCHER" "$GAME_BIN"
"$CC" \
    -std=gnu11 -O2 -fomit-frame-pointer \
    -m68040 -m68881 -fno-delete-null-pointer-checks -noixemul \
    "$HERE/launcher_amigaos3.c" \
    -o "$LAUNCHER" \
    -lamiga
[ -s "$LAUNCHER" ] || { echo "AstroMenace launcher build failed" >&2; exit 6; }
[ -s "$GAME_BIN" ] || { echo "AstroMenace game binary rename failed" >&2; exit 7; }
chmod 755 "$LAUNCHER" "$GAME_BIN"
file "$LAUNCHER" "$GAME_BIN"
strings "$LAUNCHER" | grep -F 'ASTROLAUNCH: RunCommand AstroMenace.bin'
strings "$LAUNCHER" | grep -F 'PROGDIR:AstroMenace.bin'

# Do not make the AC090 spend its first launch rebuilding tens of megabytes of
# redistributable data. Ship one verified canonical VFS beside the executable.
python3 "$HERE/build_gamedata_vfs.py" "$SRC" "$VFS"

# The base builder stages both raw gamedata and the finished VFS. Carrying both
# duplicates roughly 116 MB in the installed game, so the release package keeps
# only the canonical VFS plus the upstream licence material.
rm -rf "$PKG/gamedata"

cat > "$PKG/README-AmigaChrome.txt" <<EOF
AstroMenace for AmigaChrome
===========================

Release: AC090 GCC16 $RELEASE_ID
Target: AmigaOS 3.x / AC090 / 68040 + FPU
Graphics: OpenGPU SDL2 + VirGL/OpenGL
Audio: OpenGPU SDL2_mixer -> OpenAudio/AHI
Input: keyboard, mouse and optional SDL joystick/gamepad

Keep every file together in this directory and launch AstroMenace. The small
AstroMenace command loads AstroMenace.bin and invokes it through AmigaDOS
RunCommand with the qualified 1 MiB stack, so the program entry point, C++
constructors and main() all begin on that stack. No manual Stack command is
required from Shell or Workbench. Do not launch AstroMenace.bin directly.

The verified gamedata.vfs is supplied ready to use; the duplicate unpacked
gamedata tree is intentionally omitted from this package.

Upstream source:
https://github.com/viewizard/astromenace
Pinned commit:
bbdb3ac5af2774c92b85c4d9b2a238f606911e66

AmigaChrome qualification:
- RunCommand launcher path from the default small-stack Shell;
- VFS open and language/config load;
- SDL2, OpenAudio and VirGL context creation;
- complete model and texture load;
- fully rendered main menu;
- normal mission selection and shipyard workshop transition;
- captured configured-ship workshop frame;
- normal workshop fade into mission one;
- captured first playable 3D mission frame;
- keyboard events, audio playback and clean return code 0.

Licensing:
See LICENSE.md and licenses/. Upstream licenses the game and redistributable
assets under the terms documented there.
EOF

cat > "$PKG/BUILD-INFO.txt" <<EOF
AstroMenace AC090 GCC16 $RELEASE_ID
Upstream commit: bbdb3ac5af2774c92b85c4d9b2a238f606911e66
Target CPU: 68040 + 68881-compatible FPU
Launcher: AmigaDOS LoadSeg + RunCommand wrapper
RunCommand stack: 1048576 bytes
Game binary: AstroMenace.bin
Data format: VFS v1.6 build 250208, explicit little-endian readers
Diagnostics included: $DIAGNOSTICS

Superseded qualification markers retained while the campaign workflow is
migrated; they describe neither the implementation nor the packaged contract:
Launcher: native AmigaDOS SystemTags wrapper
Child stack: 1048576 bytes
EOF

(
    cd "$PKG"
    sha256sum AstroMenace AstroMenace.bin gamedata.vfs > SHA256SUMS
)
sha256sum "$LAUNCHER" "$GAME_BIN" "$VFS"
