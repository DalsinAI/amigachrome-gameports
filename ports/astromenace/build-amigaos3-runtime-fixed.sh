#!/bin/sh
set -eu

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
SRC=${1:-"$ROOT/build/game-ports/sources/astromenace"}
OUT=${2:-"$ROOT/build/os3/astromenace-runtime-fixed"}
DIAGNOSTICS=${ASTROMENACE_DIAGNOSTICS:-0}
RELEASE_ID=${ASTROMENACE_RELEASE_ID:-0011}
TOOLCHAIN=${STOVE:-"$HOME/AmigaChrome/stoves/os32-gcc16"}/prefix
LAUNCHER_CC=${LAUNCHER_CC:-"$TOOLCHAIN/bin/m68k-amigaos-gcc"}

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

# Keep a second in-process guard for launches of AstroMenace.bin from an
# already suitable environment. The public launcher below is the important
# fix: it establishes the child stack before the C++ runtime and constructors.
python3 "$HERE/patch_amiga_stack.py" "$SRC"

# Keep the regular builder as the compiler/library/package authority, but tell
# it not to overwrite the richer source tree prepared above.
ASTROMENACE_SOURCE_PREPATCHED=1 \
sh "$HERE/build-amigaos3.sh" "$SRC" "$OUT"

PKG="$OUT/package/AstroMenace"
LAUNCHER="$PKG/AstroMenace"
GAME_BIN="$PKG/AstroMenace.bin"
VFS="$PKG/gamedata.vfs"

[ -x "$LAUNCHER_CC" ] || {
    echo "missing Amiga launcher compiler: $LAUNCHER_CC" >&2
    exit 2
}

# A StackSwap in main is too late for a large C++ program: libnix and global
# constructors can consume stack before main. Preserve the built game as
# AstroMenace.bin and put a tiny C command at the public AstroMenace path. It
# starts the child synchronously through dos.library/SystemTags with a 1 MiB
# NP_StackSize, so every byte of C/C++ startup runs on the correct stack.
mv "$LAUNCHER" "$GAME_BIN"
"$LAUNCHER_CC" \
    -std=gnu11 -O2 -fomit-frame-pointer -fno-delete-null-pointer-checks \
    -m68040 -m68881 -noixemul \
    "$HERE/amiga_launcher.c" \
    -o "$LAUNCHER" \
    -lamiga
chmod 755 "$LAUNCHER" "$GAME_BIN"
file "$LAUNCHER" "$GAME_BIN"

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

Keep AstroMenace, AstroMenace.bin and gamedata.vfs together in this directory.
From an Amiga Shell, run:

  CD DH1:AstroMenace
  AstroMenace

Do not launch AstroMenace.bin directly. AstroMenace is a small native launcher
which creates the real game as a child process with a 1 MiB stack before the
C/C++ runtime and global constructors execute. No preceding Stack command is
required, even from a normal small-stack Shell.

The verified VFS is supplied ready to use; the duplicate unpacked gamedata tree
is intentionally omitted from this package.

Upstream source:
https://github.com/viewizard/astromenace
Pinned commit:
bbdb3ac5af2774c92b85c4d9b2a238f606911e66

AmigaChrome qualification:
- direct AstroMenace launcher run from a default small-stack Amiga Shell;
- child C/C++ startup on a 1 MiB NP_StackSize process;
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
Data format: VFS v1.6 build 250208, explicit little-endian readers
User command: AstroMenace pre-CRT stack launcher
Game executable: AstroMenace.bin
Startup stack: launcher creates a 1 MiB child before C/C++ runtime startup
In-process guard: private 1 MiB StackSwap helper when caller stack is smaller
Diagnostics included: $DIAGNOSTICS
EOF

(
    cd "$PKG"
    sha256sum AstroMenace AstroMenace.bin gamedata.vfs > SHA256SUMS
)
sha256sum "$VFS"
