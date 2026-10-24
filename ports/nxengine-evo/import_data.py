#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Dalsin Limited
"""Prepare local Cave Story data for NXEngine-evo without modifying its archive."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import zipfile
import zlib

# Byte identities from pinned upstream src/extract/extractfiles.cpp.
# Older NX data uses English track names; current NX expects original names.
ORG_IDENTITIES = {
    "access.org": (1138, 0xd965dddb),
    "balcony.org": (3082, 0x892345ca),
    "ginsuke.org": (5970, 0xb02093b8),
    "bdown.org": (2570, 0xf80dd62a),
    "cemetery.org": (4578, 0x2ce377cc),
    "lastbtl.org": (2770, 0x10dec9d5),
    "ending.org": (17898, 0xa9ed4834),
    "wanpaku.org": (19626, 0xb651047e),
    "fireeye.org": (21354, 0x6b5ff989),
    "fanfale1.org": (914, 0xaefd547b),
    "fanfale2.org": (1074, 0x3a5170a6),
    "fanfale3.org": (922, 0x85813929),
    "gameover.org": (1138, 0x1f87b446),
    "grand.org": (13466, 0xdb4795ac),
    "vivi.org": (10458, 0xce2e68c1),
    "gravity.org": (20578, 0x64a9318d),
    "weed.org": (23706, 0xa27883b6),
    "hell.org": (18386, 0x93bbf277),
    "requiem.org": (9722, 0xfc64d0d0),
    "jenka.org": (8306, 0xb42d7eaa),
    "jenka2.org": (11986, 0xc095cbe1),
    "maze.org": (14786, 0x0292cf2c),
    "lastbt3.org": (21122, 0x8888dac9),
    "lastcave.org": (18122, 0x469b38b9),
    "mdown2.org": (21074, 0x83d08aed),
    "ironh.org": (13450, 0x3ce4cdbe),
    "oside.org": (25634, 0x1e33b095),
    "plant.org": (11378, 0x3911e040),
    "kodou.org": (10418, 0x92ef0330),
    "quiet.org": (4770, 0x0e95a468),
    "escape.org": (7618, 0x65a4bb85),
    "anzen.org": (9194, 0x779e83c2),
    "wanpak2.org": (15994, 0xd09341e2),
    "ballos.org": (3570, 0x373988ad),
    "curly.org": (25738, 0xf5ace8b0),
    "toroko.org": (18482, 0xc202de07),
    "mura.org": (10634, 0x6a6aa627),
    "dr.org": (2162, 0xc64dc450),
    "marine.org": (16962, 0xb533d72a),
    "white.org": (23714, 0xcff0fb34),
    "zonbie.org": (5346, 0xd217cc29),
}


def add_music_aliases(files, identities=ORG_IDENTITIES):
    aliases = {}
    songs = {(len(data), zlib.crc32(data)): (name, data)
             for name, data in files.values() if name.casefold().startswith("org/")}
    for name, identity in identities.items():
        target = "org/" + name
        if target.casefold() not in files and identity in songs:
            original, data = songs[identity]
            files[target.casefold()] = (target, data)
            aliases[target] = original
    return aliases


def prepare(archive, engine_data, out):
    archive, engine_data, out = map(Path, (archive, engine_data, out))
    if out.exists() or out.is_symlink():
        raise ValueError("Use a new output directory; existing data is never replaced")
    if not (engine_data / "sprites.sif").is_file():
        raise ValueError("engine-data must name the pinned source export's data directory")
    files = {}
    with zipfile.ZipFile(archive) as source:
        entries = source.infolist()
        if len(entries) > 10000 or sum(e.file_size for e in entries) > 512 * 1024 * 1024:
            raise ValueError("Unexpectedly large Cave Story archive")
        nested = "data/data/npc.tbl" in source.namelist()
        for entry in entries:
            name = PurePosixPath(entry.filename)
            if name.is_absolute() or ".." in name.parts or "\\" in entry.filename or ":" in entry.filename:
                raise ValueError("Unsafe archive path")
            if (entry.external_attr >> 16) & 0o170000 == 0o120000:
                raise ValueError("Archive symlinks are not supported")
            if entry.is_dir():
                continue
            parts = name.parts
            if not parts or parts[0] != "data":
                raise ValueError("Expected a data/ archive")
            relative = PurePosixPath(*parts[2:] if nested and parts[:2] == ("data", "data") else parts[1:])
            if not relative.parts:
                raise ValueError("Empty asset path")
            key = str(relative).casefold()
            if key in files:
                raise ValueError("Conflicting archive asset paths: " + str(relative))
            files[key] = (str(relative), source.read(entry))
        # Current engine-owned fonts/metadata supersede old mirror copies.
        for path in sorted(engine_data.rglob("*")):
            if path.is_symlink():
                raise ValueError("Engine data symlinks are not supported")
            if path.is_file():
                relative = path.relative_to(engine_data).as_posix()
                files[relative.casefold()] = (relative, path.read_bytes())
    aliases = add_music_aliases(files)
    for required in ("npc.tbl", "loading.pbm", "stage/start.pxm", "sprites.sif", "tilekey.dat"):
        if required not in files:
            raise ValueError("Required Cave Story asset absent: " + required)
    for name, _ in files.values():
        parent = PurePosixPath(name).parent
        while str(parent) != ".":
            if str(parent).casefold() in files:
                raise ValueError("File/directory asset collision")
            parent = parent.parent
    for name, data in files.values():
        target = out / "data" / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
    receipt = {"archive_sha256": hashlib.sha256(archive.read_bytes()).hexdigest(),
               "music_aliases_by_upstream_crc": aliases,
               "nested_original_data_flattened": nested, "files": len(files),
               "policy": "Local external data; no redistribution grant inferred",
               "file_sha256": {name: hashlib.sha256(data).hexdigest() for name, data in sorted(files.values())}}
    (out / "IMPORT.json").write_text(json.dumps(receipt, indent=2) + "\n")
    return receipt


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archive", type=Path, required=True)
    parser.add_argument("--engine-data", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps({k: v for k, v in prepare(args.archive, args.engine_data, args.out).items()
                      if k != "file_sha256"}, indent=2))
