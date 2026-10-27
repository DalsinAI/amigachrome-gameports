#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Download genuine promotional test inputs outside the source repository.

This does not bundle or redistribute commercial game files. The resulting
directory is for an isolated local restoration test, not a distributable game.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import urllib.request
import zipfile


SOURCES = {
    "alien_crossfire_demo.zip": (
        "https://www.dosdays.co.uk/media/games/alpha/alien_crossfire_demo.zip",
        "9f15710b10b95346bb546b7531addac843193140efbf9e51ba39556623eaae91",
    ),
    "Borehole-ac2.exe": (
        "https://alphacentauri2.com/index.php?action=downloads;sa=downfile&id=36",
        "d4e05604605752ed9660cd85a9b7bec6ed31c3d8e2b72ac24a36b339b6ef7fa5",
    ),
}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def unpack(data, output, prefix=""):
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        for item in archive.infolist():
            if item.is_dir() or not item.filename.startswith(prefix):
                continue
            relative = PurePosixPath(item.filename[len(prefix):])
            if relative.is_absolute() or ".." in relative.parts:
                raise ValueError("Unsafe archive path")
            target = output.joinpath(*relative.parts)
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(archive.read(item))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--cache", required=True, type=Path)
    args = parser.parse_args()
    output = args.out.resolve()
    repository = Path(__file__).resolve().parent.parent
    if output == repository or repository in output.parents:
        parser.error("Use an output directory outside the source repository")
    output.mkdir(parents=True, exist_ok=False)
    args.cache.mkdir(parents=True, exist_ok=True)
    content = {}
    for name, (url, expected) in SOURCES.items():
        cached = args.cache / name
        if not cached.exists():
            # Identify the restoration client: the scenario mirror rejects Python
            # urllib's default agent with HTTP403 despite serving this public file.
            request = urllib.request.Request(url, headers={"User-Agent": "Alpha-native-lab/1.0"})
            with urllib.request.urlopen(request, timeout=90) as response:
                downloaded = response.read(64 * 1024 * 1024 + 1)
            if digest(downloaded) != expected:
                raise ValueError("Unexpected download digest: " + name)
            cached.write_bytes(downloaded)
        content[name] = cached.read_bytes()
        if digest(content[name]) != expected:
            raise ValueError("Unexpected cached digest: " + name)
    with zipfile.ZipFile(io.BytesIO(content["alien_crossfire_demo.zip"])) as archive:
        executable = archive.read("alien_crossfire_demo.exe")
    if digest(executable) != "0c4e4e9397ca876d69b9d199858cd31f272572e99a8c5d111a61fa0cb7a6076e":
        raise ValueError("Unexpected demo executable digest")
    unpack(executable, output, "programs/")
    unpack(content["Borehole-ac2.exe"], output / "Scenarios")
    scenario = output / "Scenarios/Borehole/Borehole.SC"
    (output / "Borehole.SC").write_bytes(scenario.read_bytes())
    files = {
        str(path.relative_to(output)): digest(path.read_bytes())
        for path in sorted(output.rglob("*")) if path.is_file()
    }
    receipt = {
        "purpose": "Private isolated native frontend test; do not redistribute these assets",
        "sources": {name: {"url": url, "sha256": sha} for name, (url, sha) in SOURCES.items()},
        "files": files,
    }
    (output / "asset-provenance.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(f"Staged {len(files)} genuine demo/scenario files in {output}")


if __name__ == "__main__":
    main()
