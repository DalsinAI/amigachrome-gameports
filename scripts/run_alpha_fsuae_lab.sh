#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Real FS-UAE boot/display smoke test, not Alpha Centauri qualification.
set -euo pipefail
out=${1:?Usage: FS_UAE_BINARY=/path/to/fs-uae run_alpha_fsuae_lab.sh NEW_OUTPUT_DIRECTORY}
test ! -e "$out"
mkdir -p "$out/home"
out=$(cd "$out" && pwd)
src=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
binary=$(command -v "${FS_UAE_BINARY:-fs-uae}")
export HOME="$out/home"
export XDG_CONFIG_HOME="$HOME/.config" XDG_DATA_HOME="$HOME/.local/share" XDG_CACHE_HOME="$HOME/.cache"
export SDL_VIDEODRIVER=offscreen EGL_PLATFORM=surfaceless LIBGL_ALWAYS_SOFTWARE=1
export SDL_AUDIODRIVER=dummy ALSOFT_DRIVERS=null
export FS_CAPTURE_DIRECTORY="$out"
cc -shared -fPIC -O2 -Wall -Wextra -Werror "$src/fsuae_capture_swap.c" -o "$out/capture_swap.so" -ldl
export LD_PRELOAD="$out/capture_swap.so${LD_PRELOAD:+:$LD_PRELOAD}"
cat > "$out/boot.fs-uae" <<EOF
[fs-uae]
amiga_model = A1200
cpu = 68040
chip_memory = 2048
fast_memory = 8192
kickstart_file = internal
fullscreen = 0
window_width = 960
window_height = 720
automatic_input_grab = 0
joystick_port_0_mode = mouse
joystick_port_1_mode = nothing
EOF
"$binary" --version > "$out/version.txt"
sha256sum "$binary" "$out/capture_swap.so" > "$out/binaries.sha256"
set +e
timeout -k 2s 25s "$binary" "$out/boot.fs-uae" > "$out/emulator.log" 2>&1
status=$?
set -e
if [ "$status" != 0 ] && [ "$status" != 124 ]; then cat "$out/emulator.log"; exit "$status"; fi
python3 - "$out" <<'PY'
from pathlib import Path
import hashlib,json,sys
from PIL import Image
out=Path(sys.argv[1]);frames=[]
for frame in [150,500,900]:
    p=out/f'fsuae-frame-{frame}.ppm'
    with Image.open(p) as im:
        assert im.size==(960,720),im.size
        colors=len(im.getcolors(im.width*im.height))
        assert colors>1,'Framebuffer is uniform; inspect the emulator logs'
        png=p.with_suffix('.png');im.save(png)
    frames.append(dict(frame=frame,width=960,height=720,colors=colors,sha256=hashlib.sha256(png.read_bytes()).hexdigest()))
logs=list((out/'home').rglob('*log*txt'))
assert any('CPU: 68040 FPU: 68040.' in p.read_text(errors='replace') for p in logs),'CPU/FPU configuration not observed'
receipt=dict(scope='Real emulator boot/display only; Alpha Centauri application NOT loaded or qualified',emulatorVersion=(out/'version.txt').read_text().strip(),cpu='68040',fpu='68040',rom='Bundled AROS replacement',frames=frames)
(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps(receipt))
PY
