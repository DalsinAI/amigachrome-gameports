#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Dalsin Limited
"""Check target SDK dependencies and configure an exact, offline Enigma export.

This is a configure gate, not a completed Enigma build recipe. Missing Amiga
libraries are reported explicitly; host libraries are never a fallback.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from build_sdl3_port import digest, verify_stove

REQUIRED = {
    "SDL2": ("lib/libSDL2.a", "include/SDL2/SDL.h"),
    "SDL2_image": ("lib/libSDL2_image.a", "include/SDL2/SDL_image.h"),
    "SDL2_ttf": ("lib/libSDL2_ttf.a", "include/SDL2/SDL_ttf.h"),
    "SDL2_mixer": ("lib/libSDL2_mixer.a", "include/SDL2/SDL_mixer.h"),
    "Xerces-C": ("lib/libxerces-c.a", "include/xercesc/util/PlatformUtils.hpp"),
    "curl": ("lib/libcurl.a", "include/curl/curl.h"),
    "zlib": ("lib/libz.a", "include/zlib.h"),
}


def inventory(sdk, overlay=None):
    roots = ((overlay,) if overlay is not None else ()) + (sdk, sdk / "port-deps")
    found, missing = {}, []
    for name, files in REQUIRED.items():
        entries = {}
        for relative in files:
            path = next((r / relative for r in roots if (r / relative).is_file()), None)
            if path is None:
                missing.append(f"{name}: {relative}")
            else:
                entries[relative] = dict(path=str(path), sha256=digest(path))
        found[name] = entries
    return dict(found=found, missing=missing)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--source", type=Path, required=True)
    ap.add_argument("--stove", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--sdk-overlay", type=Path, help="explicit additional target SDK prefix; never a host fallback")
    args = ap.parse_args()
    source, stove, out = (p.resolve() for p in (args.source, args.stove, args.out))
    if out.exists():
        ap.error("use a new output directory")
    for variable in ("STOVE", "AMIGA_STOVE"):
        if os.environ.get(variable) and Path(os.environ[variable]).resolve() != stove:
            ap.error(f"{variable} conflicts with --stove")
    overlay = args.sdk_overlay.resolve() if args.sdk_overlay else None
    if overlay is not None and not overlay.is_dir():
        ap.error("SDK overlay directory is missing")
    compiler = verify_stove(stove)
    pin = json.loads((ROOT / "gameports/catalog.json").read_text())["ports"]["enigma"]["source"]["commit"]
    subprocess.run(["git", "-C", str(source), "cat-file", "-e", pin + "^{commit}"], check=True)
    sdk = stove / "prefix/m68k-amigaos"
    report = dict(source_commit=pin, compiler_release=compiler["release"],
                  compiler_archive_sha256=compiler["archive_sha256"], sdk_overlay=str(overlay) if overlay else None, **inventory(sdk, overlay))
    out.mkdir(parents=True)
    (out / "dependencies.json").write_text(json.dumps(report, indent=2) + "\n")
    if report["missing"]:
        print(json.dumps(report, indent=2))
        return 2
    work = out / "source"
    work.mkdir()
    with tempfile.TemporaryFile() as archive:
        subprocess.run(["git", "-C", str(source), "archive", pin], stdout=archive, check=True)
        archive.seek(0)
        subprocess.run(["tar", "-xf", "-", "-C", str(work)], stdin=archive, check=True)
    command = ["cmake", "-S", str(work), "-B", str(out / "engine"),
               f"-DCMAKE_TOOLCHAIN_FILE={ROOT / 'toolchain/amigaos3-opengpu.cmake'}",
               f"-DAMIGA_STOVE={stove}", "-DAMIGA_CPU_FLAGS=-m68040 -m68881 -fno-delete-null-pointer-checks",
               f"-DCMAKE_FIND_ROOT_PATH={sdk};{sdk / 'port-deps'}",
               f"-DCMAKE_PREFIX_PATH={sdk};{sdk / 'port-deps'}", "-DCMAKE_BUILD_TYPE=Release"]
    if overlay:
        command += [f"-DAMIGA_SDK_OVERLAYS={overlay}"]
        flags = f'-noixemul -m68040 -m68881 -pthread -fno-delete-null-pointer-checks -isystem "{overlay / "include"}"'
        command += [f"-DCMAKE_C_FLAGS={flags}", f"-DCMAKE_CXX_FLAGS={flags}"]
    with (out / "configure.log").open("w") as log:
        result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT)
    report.update(command=command, configure_returncode=result.returncode)
    (out / "configure.json").write_text(json.dumps(report, indent=2) + "\n")
    return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
