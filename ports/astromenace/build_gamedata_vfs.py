#!/usr/bin/env python3
"""Build AstroMenace gamedata.vfs as canonical little-endian VFS v1.6.

The upstream first-launch path builds this archive in the target process.  On
68k that is both slow and, before the endian fix, incompatible with the
existing little-endian models.pack.  Build the redistributable archive once on
the CI/build host and ship it beside the executable.  The game still retains
its corrected target-side packer as a recovery path.
"""

from __future__ import annotations

import ast
from dataclasses import dataclass
from pathlib import Path
import re
import struct
import sys


@dataclass(frozen=True)
class Entry:
    name: str
    source: Path
    offset: int
    size: int


def parse_game_data(source: Path) -> list[str]:
    text = (source / "src" / "utils" / "fs2vfs.cpp").read_text(encoding="utf-8")
    marker = "const std::string GameData[] = {"
    start = text.find(marker)
    if start < 0:
        raise RuntimeError("GameData array marker missing")
    start += len(marker)
    end = text.find("\n};", start)
    if end < 0:
        raise RuntimeError("GameData array end missing")
    body = text[start:end]
    values: list[str] = []
    for literal in re.findall(r'"(?:\\.|[^"\\])*"', body):
        value = ast.literal_eval(literal)
        if not isinstance(value, str) or not value:
            raise RuntimeError(f"invalid GameData value: {literal}")
        values.append(value)
    if not values:
        raise RuntimeError("GameData array was empty")
    return values


def parse_build_number(source: Path) -> int:
    text = (source / "src" / "build_config.h").read_text(encoding="utf-8")
    match = re.search(r"^#define\s+GAME_VFS_BUILD\s+(\d+)\s*$", text, re.M)
    if not match:
        raise RuntimeError("GAME_VFS_BUILD missing")
    return int(match.group(1))


def parse_models_pack(path: Path) -> tuple[bytes, list[tuple[str, int, int]]]:
    data = path.read_bytes()
    if len(data) < 16 or data[:4] != b"VFS_" or data[4:8] != b"v1.6":
        raise RuntimeError(f"bad models.pack header: {path}")
    _build, table_offset = struct.unpack_from("<II", data, 8)
    if table_offset < 16 or table_offset > len(data):
        raise RuntimeError(f"bad models.pack table offset {table_offset}")

    entries: list[tuple[str, int, int]] = []
    names: set[str] = set()
    pos = table_offset
    while pos < len(data):
        if pos + 2 > len(data):
            raise RuntimeError("truncated models.pack name length")
        name_size = struct.unpack_from("<H", data, pos)[0]
        pos += 2
        if not name_size or pos + name_size + 8 > len(data):
            raise RuntimeError("truncated models.pack table entry")
        name = data[pos : pos + name_size].decode("utf-8")
        pos += name_size
        offset, size = struct.unpack_from("<II", data, pos)
        pos += 8
        if name in names:
            raise RuntimeError(f"duplicate models.pack entry: {name}")
        if offset < 16 or offset + size > table_offset:
            raise RuntimeError(f"models.pack entry outside data area: {name}")
        names.add(name)
        entries.append((name, offset, size))

    if pos != len(data) or not entries:
        raise RuntimeError("invalid or empty models.pack table")
    return data, entries


def build(source: Path, output: Path) -> None:
    source = source.resolve()
    raw_root = source / "gamedata"
    models_path = raw_root / "models" / "models.pack"
    build_number = parse_build_number(source)
    game_data = parse_game_data(source)
    models_bytes, model_entries = parse_models_pack(models_path)

    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_name(output.name + ".writing")
    records: list[tuple[str, int, int]] = []
    names: set[str] = set()

    with temporary.open("wb+") as out:
        out.write(b"VFS_")
        out.write(b"v1.6")
        out.write(struct.pack("<I", build_number))
        out.write(struct.pack("<I", 0))

        for name, source_offset, size in model_entries:
            if name in names:
                raise RuntimeError(f"duplicate VFS entry: {name}")
            offset = out.tell()
            out.write(models_bytes[source_offset : source_offset + size])
            records.append((name, offset, size))
            names.add(name)

        for name in game_data:
            if name in names:
                raise RuntimeError(f"duplicate VFS entry: {name}")
            path = raw_root / Path(name)
            if not path.is_file():
                raise RuntimeError(f"missing gamedata file: {name}")
            payload = path.read_bytes()
            offset = out.tell()
            out.write(payload)
            records.append((name, offset, len(payload)))
            names.add(name)

        table_offset = out.tell()
        for name, offset, size in records:
            encoded = name.encode("utf-8")
            if len(encoded) > 0xFFFF:
                raise RuntimeError(f"VFS name too long: {name}")
            if offset > 0xFFFFFFFF or size > 0xFFFFFFFF:
                raise RuntimeError(f"VFS entry exceeds v1.6 limits: {name}")
            out.write(struct.pack("<H", len(encoded)))
            out.write(encoded)
            out.write(struct.pack("<II", offset, size))

        if table_offset > 0xFFFFFFFF:
            raise RuntimeError("VFS table offset exceeds v1.6 limits")
        out.seek(12)
        out.write(struct.pack("<I", table_offset))
        out.flush()

    temporary.replace(output)

    # Read the finished header and table back before allowing packaging.
    final = output.read_bytes()
    sign, version, actual_build, actual_table = struct.unpack_from("<4s4sII", final, 0)
    if (sign, version, actual_build, actual_table) != (
        b"VFS_", b"v1.6", build_number, table_offset
    ):
        raise RuntimeError("finished VFS header verification failed")
    if len(records) < 100 or actual_table >= len(final):
        raise RuntimeError("finished VFS content verification failed")

    print(
        f"built {output}: {len(records)} files, {len(final)} bytes, "
        f"build {build_number}, table {table_offset}"
    )


def main() -> int:
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} ASTROMENACE_SOURCE OUTPUT_VFS", file=sys.stderr)
        return 2
    build(Path(sys.argv[1]), Path(sys.argv[2]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
