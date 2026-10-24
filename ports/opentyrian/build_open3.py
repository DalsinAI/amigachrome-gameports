#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Dalsin Limited
"""Build the pinned SDL2 engine offline with the verified Amiga open3 SDK."""
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


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--source", type=Path, required=True)
    ap.add_argument("--stove", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--jobs", type=int, default=2)
    args = ap.parse_args()
    source, stove, out = (p.resolve() for p in (args.source, args.stove, args.out))
    if out.exists() or out.is_symlink() or args.jobs < 1:
        ap.error("use a new output directory and a positive job count")
    for variable in ("STOVE", "AMIGA_STOVE"):
        if os.environ.get(variable) and Path(os.environ[variable]).resolve() != stove:
            ap.error(f"{variable} conflicts with --stove")
    compiler = verify_stove(stove)
    pin = json.loads((ROOT / "gameports/catalog.json").read_text())["ports"]["opentyrian"]["source"]["commit"]
    subprocess.run(["git", "-C", str(source), "cat-file", "-e", pin + "^{commit}"], check=True)
    epoch = subprocess.check_output(["git", "-C", str(source), "show", "-s", "--format=%ct", pin], text=True).strip()
    sdk = stove / "prefix/m68k-amigaos"
    inputs = {name: digest(sdk / name) for name in ("lib/libSDL2.a", "lib/libGL.a", "include/SDL2/SDL.h")}
    out.mkdir(parents=True)
    work = out / "source"
    work.mkdir()
    with tempfile.TemporaryFile() as archive:
        subprocess.run(["git", "-C", str(source), "archive", pin], stdout=archive, check=True)
        archive.seek(0)
        subprocess.run(["tar", "-xf", "-", "-C", str(work)], stdin=archive, check=True)
    patches = {}
    for patch in sorted((ROOT / "ports/opentyrian/patches").glob("*.patch")):
        with patch.open() as content:
            subprocess.run(["patch", "--batch", "--forward", "--fuzz=0", "-p1"],
                           cwd=work, stdin=content, check=True)
        patches[patch.name] = digest(patch)
    flags = f"-noixemul -m68040 -m68881 -fno-delete-null-pointer-checks -pthread -O2 -ffile-prefix-map={work}=."
    command = ["make", "-j", str(args.jobs), "PLATFORM=AMIGA", "WITH_NETWORK=false",
               f"CC={stove / 'prefix/bin/m68k-amigaos-gcc'}", f"CFLAGS={flags}",
               f"SDL_CPPFLAGS=-I{sdk / 'include/SDL2'}", f"SDL_LDFLAGS=-L{sdk / 'lib'}",
               "SDL_LDLIBS=-lSDL2 -lGL -lm -pthread", "TYRIAN_DIR=PROGDIR:data",
               f"VCS_IDREV=printf {pin}"]
    with (out / "build.log").open("w") as log:
        subprocess.run(command, cwd=work, stdout=log, stderr=subprocess.STDOUT,
                       env=dict(os.environ, SOURCE_DATE_EPOCH=epoch), check=True)
    receipt = dict(source_commit=pin, source_date_epoch=epoch, patches=patches,
                   compiler_release=compiler["release"], compiler_archive_sha256=compiler["archive_sha256"],
                   sdk_inputs=inputs, command=command, executable_sha256=digest(work / "opentyrian"),
                   scope="Offline source build only; network disabled; no runtime/release pass implied")
    (out / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps(receipt, indent=2))


if __name__ == "__main__":
    main()
