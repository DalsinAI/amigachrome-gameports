#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Dalsin Limited
"""Build the normal SDL2 game offline, without modifying the upstream cache."""
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
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True, help="local upstream Git cache")
    parser.add_argument("--stove", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True, help="new output directory")
    parser.add_argument("--jobs", type=int, default=2)
    args = parser.parse_args()
    source, stove, out = (p.resolve() for p in (args.source, args.stove, args.out))
    if out.exists() or args.jobs < 1:
        parser.error("use a new output directory and a positive job count")
    for variable in ("STOVE", "AMIGA_STOVE"):
        if os.environ.get(variable) and Path(os.environ[variable]).resolve() != stove:
            parser.error(f"{variable} conflicts with --stove")
    compiler = verify_stove(stove)
    pin = json.loads((ROOT / "gameports/catalog.json").read_text())["ports"]["openjazz"]["source"]["commit"]
    subprocess.run(["git", "-C", str(source), "cat-file", "-e", pin + "^{commit}"], check=True)
    epoch = subprocess.check_output(["git", "-C", str(source), "show", "-s", "--format=%ct", pin], text=True).strip()
    environment = dict(os.environ, SOURCE_DATE_EPOCH=epoch)
    out.mkdir(parents=True)
    work = out / "source"
    work.mkdir()
    # git archive ignores local edits in the source cache. Only checked patches
    # enter the build; it does not reuse generated validation entry points.
    with tempfile.TemporaryFile() as archive:
        subprocess.run(["git", "-C", str(source), "archive", pin], stdout=archive, check=True)
        archive.seek(0)
        subprocess.run(["tar", "-xf", "-", "-C", str(work)], stdin=archive, check=True)
    patches = {}
    for patch in sorted((ROOT / "ports/openjazz/patches").glob("*.patch")):
        with patch.open() as content:
            subprocess.run(["patch", "--batch", "--forward", "--fuzz=0", "-p1"],
                           cwd=work, stdin=content, check=True)
        patches[patch.name] = digest(patch)
    sysroot = stove / "prefix/m68k-amigaos"
    sdk_files = ("lib/libSDL2.a", "lib/libGL.a", "include/SDL2/SDL.h")
    sdk = {name: digest(sysroot / name) for name in sdk_files}
    commands = [
        ["cmake", "-S", str(work), "-B", str(out / "engine"),
         f"-DCMAKE_TOOLCHAIN_FILE={ROOT / 'toolchain/amigaos3-opengpu.cmake'}",
         f"-DAMIGA_STOVE={stove}", "-DAMIGA_CPU_FLAGS=-m68040 -m68881 -fno-delete-null-pointer-checks",
         f"-DSDL2_DIR={sysroot / 'lib/cmake/SDL2'}", "-DCMAKE_BUILD_TYPE=Release",
         f"-DCMAKE_C_FLAGS_RELEASE=-O3 -DNDEBUG -ffile-prefix-map={work}=.",
         f"-DCMAKE_CXX_FLAGS_RELEASE=-O3 -DNDEBUG -ffile-prefix-map={work}=.",
         "-DNETWORK=OFF", "-DSCALE=OFF", "-DSDL_VERSION=2", "-DPORTABLE=ON"],
        ["cmake", "--build", str(out / "engine"), "--target", "OpenJazz", "-j", str(args.jobs)],
    ]
    for name, command in zip(("configure", "build"), commands):
        with (out / (name + ".log")).open("w") as log:
            subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True, env=environment)
    receipt = dict(upstream_commit=pin, patches=patches,
                   compiler_release=compiler["release"], compiler_archive_sha256=compiler["archive_sha256"],
                   sdk_inputs=sdk, commands=commands,
                   source_date_epoch=epoch,
                   executable_sha256=digest(out / "engine/OpenJazz"),
                   scope="Normal SDL2 executable; build receipt only, no runtime or release qualification")
    (out / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps(receipt, indent=2))


if __name__ == "__main__":
    main()
