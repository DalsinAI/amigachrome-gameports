#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Run the real native Alpha world in a fresh FS-UAE/AROS laboratory."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import time


DEFAULT_KEYS = (
    ",".join(f"{1200 + i * 40}:43:9" for i in range(18))
    + ",2700:95:1073741919,3400:25:118,4000:21:114,4600:20:113"
)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def guest_path(root, *parts):
    """Respect existing Amiga names on a case-sensitive host filesystem."""
    for part in parts:
        matches = [p for p in root.iterdir() if p.name.lower() == part.lower()] if root.is_dir() else []
        if len(matches) > 1:
            raise ValueError(f"Ambiguous guest name: {root / part}")
        root = matches[0] if matches else root / part
    return root


def file_arg(value):
    path = Path(value).expanduser().resolve()
    if not path.is_file():
        raise argparse.ArgumentTypeError(f"File does not exist: {path}")
    return path


def dir_arg(value):
    path = Path(value).expanduser().resolve()
    if not path.is_dir():
        raise argparse.ArgumentTypeError(f"Directory does not exist: {path}")
    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--aros-disk", required=True, type=dir_arg, help="Existing genuine folder-backed AROS system disk")
    parser.add_argument("--assets", required=True, type=dir_arg, help="Prepared demo assets including official Borehole.SC")
    parser.add_argument("--world-binary", required=True, type=file_arg, help="Built native candidate standalone world HUNK")
    parser.add_argument("--fs-uae", required=True, type=file_arg, help="Actual FS-UAE Linux executable")
    parser.add_argument("--sdl-headers", required=True, type=dir_arg, help="Genuine SDL2 include directory containing SDL.h")
    parser.add_argument("--sdl-module", required=True, type=file_arg, help="Built genuine OpenGPU SDL2.module")
    parser.add_argument("--out", required=True, type=Path, help="New output directory; must not exist")
    parser.add_argument("--seconds", type=float, default=120, help="Host execution limit (default:120 seconds)")
    parser.add_argument("--sequence", default=DEFAULT_KEYS, help="SDL frame:scancode:keycode triples (default:TAB18 times,NW,V,R,Q)")
    parser.add_argument("--cc", default="cc", help="Host C compiler for actual capture/input helpers")
    parser.add_argument("--prepare-only", action="store_true", help="Stage and compile helpers without starting FS-UAE")
    args = parser.parse_args()
    if args.seconds <= 0:
        parser.error("--seconds must be positive")
    if args.sequence and not re.fullmatch(r"\d+:\d+:\d+(,\d+:\d+:\d+)*", args.sequence):
        parser.error("--sequence must contain frame:scancode:keycode triples")
    if not (args.sdl_headers / "SDL.h").is_file():
        parser.error("--sdl-headers must contain genuine SDL.h")
    if args.world_binary.read_bytes()[:4] != b"\0\0\x03\xf3":
        parser.error("--world-binary is not an Amiga HUNK executable")
    for name in ("alpha.txt", "Borehole.SC", "texture.pcx", "Units.pcx", "console.pcx", "ALPHC___.TTF", "arialn.ttf"):
        if not guest_path(args.assets, name).is_file():
            parser.error(f"Missing required original asset: {name}")
    out = args.out.expanduser().resolve()
    if out.exists():
        parser.error(f"Output already exists: {out}")
    if any(out.is_relative_to(source) for source in (args.aros_disk, args.assets)):
        parser.error("Output must be outside both source disks")
    out.mkdir(parents=True)
    system, game = out / "aros", out / "game"
    print(f"Cloning input disks into {out}", flush=True)
    # Dereference source symlinks: all guest writes must land in this new run.
    shutil.copytree(args.aros_disk, system)
    shutil.copytree(args.assets, game)
    prior_checkpoints = set(game.rglob("session.aclocal"))
    # A copied result from an earlier run must never satisfy this run's gate.
    guest_log_path = guest_path(game, "world-run.log")
    guest_log_path.write_bytes(b"")
    shutil.copy2(args.world_binary, game / "alpha-world")
    module = guest_path(system, "Libs", "OpenGPU", "SDL2.module")
    module.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(args.sdl_module, module)
    screenmode = guest_path(system, "Prefs", "Env-Archive", "SYS", "screenmode.prefs")
    screenmode.parent.mkdir(parents=True, exist_ok=True)
    # Genuine AROS ScreenModePrefs: four reserved ULONGs, DisplayID, four UWORDs.
    scrm = struct.pack(">5I4H", 0, 0, 0, 0, 0x10071400, 1280, 1024, 16, 0)
    pref = b"PREFPRHD" + struct.pack(">I", 6) + bytes(6) + b"SCRM" + struct.pack(">I", len(scrm)) + scrm
    screenmode.write_bytes(b"FORM" + struct.pack(">I", len(pref)) + pref)
    startup = guest_path(system, "S", "User-Startup")
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text(
        "Stack 1048576\nSetEnv SDL_OPENGPU 0\nSetEnv SDL_RENDER_DRIVER software\n"
        "CD DH1:\nEcho \"Launching native Boreholes local actions\"\n"
        "DH1:alpha-world base alpha.txt Borehole.SC texture.pcx Units.pcx --local >DH1:world-run.log\n"
        "Echo \"WORLD_RC $RC\" >>DH1:world-run.log\n", encoding="ascii")
    home = out / "home"
    home.mkdir()
    config = out / "machine.fs-uae"
    rom = guest_path(system, "boot", "amiga", "aros-rom.bin")
    ext = guest_path(system, "boot", "amiga", "aros-ext.bin")
    for path in (rom, ext):
        if not path.is_file():
            raise ValueError(f"Missing genuine AROS ROM: {path}")
    config.write_text(f"""[fs-uae]
amiga_model = A1200
cpu = 68040
motherboard_ram = 65536
fast_memory = 0
kickstart_file = {rom}
kickstart_ext_file = {ext}
hard_drive_0 = {system}
hard_drive_0_label = AROS
hard_drive_1 = {game}
hard_drive_1_label = AlphaDemo
graphics_card = uaegfx-z2
graphics_card_memory = 4
fullscreen = 0
window_width = 1280
window_height = 1024
automatic_input_grab = 0
joystick_port_0_mode = mouse
""", encoding="utf-8")
    scripts = Path(__file__).resolve().parent
    capture, inputs = out / "capture_swap.so", out / "input_sequence.so"
    common = [args.cc, "-shared", "-fPIC", "-O2", "-Wall", "-Wextra", "-Werror"]
    subprocess.run(common + [str(scripts / "fsuae_capture_swap.c"), "-o", str(capture), "-ldl"], check=True)
    subprocess.run(common + ["-I", str(args.sdl_headers), str(scripts / "fsuae_input_sequence.c"), "-o", str(inputs), "-ldl"], check=True)
    receipt = {
        "scope": "Actual native Alpha frontend in FS-UAE/AROS; not AmigaOS3.2 or complete turn/AI qualification",
        "binarySHA256": digest(args.world_binary), "sdlModuleSHA256": digest(args.sdl_module),
        "fsUAESHA256": digest(args.fs_uae), "arosROMSHA256": digest(rom), "arosExtROMSHA256": digest(ext),
        "captureSourceSHA256": digest(scripts / "fsuae_capture_swap.c"),
        "inputSourceSHA256": digest(scripts / "fsuae_input_sequence.c"),
        "screenMode": {"id": "0x10071400", "width": 1280, "height": 1024, "depth": 16},
        "sequence": args.sequence, "preparedOnly": args.prepare_only,
        "assetsSHA256": {name: digest(guest_path(game, name)) for name in ("alpha.txt", "Borehole.SC", "texture.pcx", "Units.pcx", "console.pcx")},
    }
    if args.prepare_only:
        (out / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
        print(json.dumps(receipt), flush=True)
        return 0
    env = os.environ.copy()
    env.update(HOME=str(home), XDG_CONFIG_HOME=str(home / ".config"),
               XDG_DATA_HOME=str(home / ".local/share"), XDG_CACHE_HOME=str(home / ".cache"),
               SDL_VIDEODRIVER="offscreen", EGL_PLATFORM="surfaceless", LIBGL_ALWAYS_SOFTWARE="1",
               SDL_AUDIODRIVER="dummy", ALSOFT_DRIVERS="null", FS_CAPTURE_DIRECTORY=str(out),
               FS_CAPTURE_EVERY="250", FS_INPUT_SEQUENCE=args.sequence,
               LD_PRELOAD=f"{inputs}:{capture}")
    with (out / "emulator.log").open("wb") as log:
        child = subprocess.Popen([str(args.fs_uae), str(config)], cwd=out, env=env, stdout=log, stderr=subprocess.STDOUT)
        deadline = time.monotonic() + args.seconds
        guest_finished = False
        while child.poll() is None and time.monotonic() < deadline:
            # The guest shell writes this only after the native application
            # returns. Stop our child when complete instead of burning CPU on
            # an idle desktop until the hard deadline.
            if re.search(r"^WORLD_RC \d+\s*$", guest_log_path.read_text(errors="replace"), re.MULTILINE):
                guest_finished = True
                break
            time.sleep(0.25)
        timed_out = child.poll() is None and not guest_finished
        if child.poll() is None:
            child.terminate()
            try:
                returncode = child.wait(timeout=2)
            except subprocess.TimeoutExpired:
                child.kill()
                returncode = child.wait()
        else:
            returncode = child.returncode
    guest_log = guest_log_path.read_text(errors="replace")
    matches = re.findall(r"^WORLD_RC (\d+)\s*$", guest_log, re.MULTILINE)
    receipt.update(emulatorReturnCode=returncode, emulatorTimedOut=timed_out,
                   emulatorStoppedAfterGuest=guest_finished,
                   guestReturnCode=int(matches[-1]) if matches else None,
                   worldDrawObserved="DRAW selected=" in guest_log,
                   localActions=[line for line in guest_log.splitlines() if line.startswith("LOCAL ACTION ")],
                   newCheckpoints=[{"path": str(p.relative_to(out)), "size": p.stat().st_size, "sha256": digest(p)}
                                   for p in sorted(set(game.rglob("session.aclocal")) - prior_checkpoints)])
    receipt["movementObserved"] = "status=Move: moved (cost " in guest_log
    receipt["reloadObserved"] = "status=Reload: ok" in guest_log
    frames = sorted(out.glob("fsuae-frame-*.ppm"), key=lambda p: int(p.stem.rsplit("-", 1)[1]))
    receipt["framebuffers"] = [{"path": p.name, "sha256": digest(p)} for p in frames]
    try:
        from PIL import Image
        for p in frames:
            with Image.open(p) as image:
                image.save(p.with_suffix(".png"))
    except ImportError:
        receipt["imageConversion"] = "Pillow unavailable; genuine PPM framebuffers retained"
    passed = receipt["guestReturnCode"] == 0 and receipt["worldDrawObserved"] and bool(frames)
    receipt["guestWorldSmokePassed"] = passed
    (out / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps(receipt), flush=True)
    return 0 if passed else 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (ValueError, OSError, subprocess.CalledProcessError) as error:
        print(f"Alpha native lab: {error}", file=sys.stderr)
        sys.exit(1)
