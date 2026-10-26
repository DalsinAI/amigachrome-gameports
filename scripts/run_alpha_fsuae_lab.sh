#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Isolated emulator bootstrap only; not an Alpha Centauri gameplay test.
set -euo pipefail
out=${1:?Usage: run_alpha_fsuae_lab.sh NEW_OUTPUT_DIRECTORY}
test ! -e "$out"
mkdir -p "$out/home" "$out/disk"
out=$(cd "$out" && pwd)
export HOME="$out/home"
export XDG_CONFIG_HOME="$HOME/.config"
export XDG_DATA_HOME="$HOME/.local/share"
export XDG_CACHE_HOME="$HOME/.cache"
export LIBGL_ALWAYS_SOFTWARE=1
export SDL_AUDIODRIVER=dummy
cat > "$out/boot.fs-uae" <<EOF
[fs-uae]
amiga_model = A1200
cpu = 68040
chip_memory = 2048
fast_memory = 8192
kickstart_file = internal
graphics_card = uaegfx-z2
graphics_card_memory = 4096
hard_drive_0 = $out/disk
hard_drive_0_label = CloudLab
fullscreen = 0
window_width = 960
window_height = 720
automatic_input_grab = 0
joystick_port_0_mode = mouse
joystick_port_1_mode = nothing
uae_sound_output = none
EOF
dpkg-query -W fs-uae xvfb libgl1-mesa-dri > "$out/packages.txt"
glxinfo -B > "$out/software-display.txt" 2>&1
fs-uae "$out/boot.fs-uae" > "$out/emulator.log" 2>&1 &
emulator_pid=$!
cleanup() {
  kill "$emulator_pid" 2>/dev/null || true
  wait "$emulator_pid" 2>/dev/null || true
}
trap cleanup EXIT
for moment in 5 15; do
  sleep "$moment"
  if ! kill -0 "$emulator_pid" 2>/dev/null; then
    cat "$out/emulator.log"
    exit 1
  fi
  xdotool search --onlyvisible --name "FS-UAE" > "$out/window-ids-$moment.txt"
  import -window root "$out/fsuae-boot-$moment.png"
done
find "$HOME" -type f \( -iname '*.log' -o -iname '*log*.txt' \) -print > "$out/runtime-log-paths.txt"
printf 'Actual FS-UAE process stayed alive through both captures.\nBundled AROS replacement ROM; empty test disk.\nAlpha Centauri application and original assets have NOT been loaded.\n' > "$out/SCOPE.txt"
sha256sum "$out"/fsuae-boot-*.png > "$out/screenshots.sha256"
