#!/usr/bin/env python3
"""Prepare exact upstream inputs for the AGA/040 game-port workstream.

Network access is permitted only by prepare. Check and verify are local.
The prepared tree is therefore safe to consume from an offline Kitchen build.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import tarfile
import urllib.request
from pathlib import Path, PurePosixPath

CATALOG_REL = Path("gameports/catalog.json")
WORK_REL = Path("build/game-ports")
PREPARED_SCHEMA = "amigachrome-aga-game-ports-prepared"


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for block in iter(lambda: fh.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def read_catalog(root: Path, catalog_path: Path | None = None) -> dict:
    path = catalog_path or root / CATALOG_REL
    data = json.loads(path.read_text(encoding="utf-8"))
    if data.get("schema") != 1 or not isinstance(data.get("ports"), dict):
        raise ValueError("unsupported game-port catalog")
    for game_id, spec in data["ports"].items():
        if not game_id or not isinstance(spec, dict):
            raise ValueError("invalid game id/specification")
        source = spec.get("source")
        if not isinstance(source, dict) or source.get("kind") not in {"git", "archive"}:
            raise ValueError(f"{game_id}: unsupported source contract")
        if source["kind"] == "git":
            commit = str(source.get("commit") or "")
            if len(commit) != 40 or any(c not in "0123456789abcdef" for c in commit.lower()):
                raise ValueError(f"{game_id}: git source is not pinned to a 40-character commit")
        else:
            digest = str(source.get("sha256") or "")
            if len(digest) != 64 or any(c not in "0123456789abcdef" for c in digest.lower()):
                raise ValueError(f"{game_id}: archive source has no SHA-256 pin")
    dependencies = data.get("buildDependencies") or {}
    if not isinstance(dependencies, dict):
        raise ValueError("buildDependencies must be an object")
    for dep_id, source in dependencies.items():
        if not dep_id or not isinstance(source, dict) or source.get("kind") != "git":
            raise ValueError(f"{dep_id}: unsupported build dependency contract")
        commit = str(source.get("commit") or "")
        if len(commit) != 40 or any(ch not in "0123456789abcdef" for ch in commit.lower()):
            raise ValueError(f"{dep_id}: build dependency is not pinned to a 40-character commit")
    return data


def selected_ports(catalog: dict, game: str) -> list[tuple[str, dict]]:
    ports = catalog["ports"]
    if game == "all":
        return sorted(ports.items(), key=lambda kv: (int(kv[1].get("order", 999)), kv[0]))
    if game not in ports:
        raise ValueError(f"unknown game port: {game}")
    return [(game, ports[game])]


def run(cmd: list[str], *, cwd: Path | None = None) -> str:
    proc = subprocess.run(
        cmd, cwd=cwd, check=True, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, text=True
    )
    return proc.stdout.strip()


def git_head(path: Path) -> str:
    return run(["git", "rev-parse", "HEAD"], cwd=path)


def prepare_git(game_id: str, source: dict, dest: Path) -> dict:
    expected = source["commit"].lower()
    if dest.is_dir() and (dest / ".git").is_dir():
        got = git_head(dest).lower()
        if got == expected:
            if bool(source.get("submodules")):
                run(["git", "submodule", "update", "--init", "--recursive", "--depth", "1"], cwd=dest)
            return {"kind": "git", "commit": got, "path": str(dest)}
        shutil.rmtree(dest)
    elif dest.exists():
        if dest.is_dir():
            shutil.rmtree(dest)
        else:
            dest.unlink()
    dest.parent.mkdir(parents=True, exist_ok=True)
    run(["git", "init", str(dest)])
    run(["git", "remote", "add", "origin", str(source["url"])], cwd=dest)
    run(["git", "fetch", "--depth", "1", "origin", expected], cwd=dest)
    run(["git", "checkout", "--detach", "FETCH_HEAD"], cwd=dest)
    got = git_head(dest).lower()
    if got != expected:
        raise RuntimeError(f"{game_id}: fetched {got}, expected {expected}")
    if bool(source.get("submodules")):
        run(["git", "submodule", "update", "--init", "--recursive", "--depth", "1"], cwd=dest)
    return {"kind": "git", "commit": got, "path": str(dest)}


def _safe_tar_members(tf: tarfile.TarFile):
    members = tf.getmembers()
    for member in members:
        p = PurePosixPath(member.name)
        if p.is_absolute() or ".." in p.parts:
            raise ValueError(f"unsafe archive path: {member.name}")
        if member.isdev() or member.issym() or member.islnk():
            raise ValueError(f"unsupported archive member type: {member.name}")
    return members


def prepare_archive(game_id: str, source: dict, cache: Path, dest: Path) -> dict:
    cache.mkdir(parents=True, exist_ok=True)
    filename = str(source.get("filename") or f"{game_id}.archive")
    archive = cache / filename
    expected = str(source["sha256"]).lower()
    if not archive.is_file() or sha256_file(archive).lower() != expected:
        tmp = archive.with_suffix(archive.suffix + ".part")
        tmp.unlink(missing_ok=True)
        urllib.request.urlretrieve(str(source["url"]), tmp)
        got = sha256_file(tmp).lower()
        if got != expected:
            tmp.unlink(missing_ok=True)
            raise RuntimeError(f"{game_id}: archive SHA-256 {got}, expected {expected}")
        os.replace(tmp, archive)
    if dest.exists():
        shutil.rmtree(dest)
    dest.mkdir(parents=True, exist_ok=True)
    with tarfile.open(archive, "r:*") as tf:
        members = _safe_tar_members(tf)
        tf.extractall(dest, members=members, filter="data")
    children = [p for p in dest.iterdir()]
    source_root = dest
    if len(children) == 1 and children[0].is_dir():
        source_root = children[0]
    return {
        "kind": "archive",
        "sha256": expected,
        "archive": str(archive),
        "path": str(source_root),
    }


def prepare(root: Path, catalog: dict, game: str) -> dict:
    work = root / WORK_REL
    sources = work / "sources"
    cache = work / "cache"
    result = {
        "schema": PREPARED_SCHEMA,
        "version": 1,
        "ports": {},
        "dependencies": {},
        "catalog": str(root / CATALOG_REL),
    }
    for dep_id, source in sorted((catalog.get("buildDependencies") or {}).items()):
        dest = work / "dependencies" / dep_id
        result["dependencies"][dep_id] = prepare_git(dep_id, source, dest)
    for game_id, spec in selected_ports(catalog, game):
        source = spec["source"]
        dest = sources / game_id
        if source["kind"] == "git":
            item = prepare_git(game_id, source, dest)
        else:
            item = prepare_archive(game_id, source, cache, dest)
        item["title"] = spec.get("title", game_id)
        result["ports"][game_id] = item
    marker = work / "PREPARED.json"
    if marker.is_file() and game != "all":
        try:
            old = json.loads(marker.read_text(encoding="utf-8"))
            if old.get("schema") == PREPARED_SCHEMA and isinstance(old.get("ports"), dict):
                result["ports"] = {**old["ports"], **result["ports"]}
                if isinstance(old.get("dependencies"), dict):
                    result["dependencies"] = {**old["dependencies"], **result["dependencies"]}
        except (OSError, json.JSONDecodeError):
            pass
    marker.parent.mkdir(parents=True, exist_ok=True)
    tmp = marker.with_suffix(".tmp")
    tmp.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    os.replace(tmp, marker)
    return result


def verify(root: Path, catalog: dict, game: str) -> dict:
    marker = root / WORK_REL / "PREPARED.json"
    data = json.loads(marker.read_text(encoding="utf-8"))
    if data.get("schema") != PREPARED_SCHEMA or not isinstance(data.get("ports"), dict):
        raise ValueError("prepared marker is invalid")
    checked = {}
    checked_dependencies = {}
    for dep_id, source in sorted((catalog.get("buildDependencies") or {}).items()):
        item = (data.get("dependencies") or {}).get(dep_id)
        if not isinstance(item, dict):
            raise ValueError(f"{dep_id}: build dependency not prepared")
        path = Path(str(item.get("path") or ""))
        if not path.is_dir():
            raise ValueError(f"{dep_id}: prepared dependency path is missing")
        got = git_head(path).lower()
        if got != source["commit"].lower():
            raise ValueError(f"{dep_id}: prepared dependency commit changed: {got}")
        checked_dependencies[dep_id] = {"commit": got, "path": str(path)}
    for game_id, spec in selected_ports(catalog, game):
        item = data["ports"].get(game_id)
        if not isinstance(item, dict):
            raise ValueError(f"{game_id}: not prepared")
        source = spec["source"]
        path = Path(str(item.get("path") or ""))
        if not path.exists():
            raise ValueError(f"{game_id}: prepared source path is missing")
        if source["kind"] == "git":
            got = git_head(path).lower()
            if got != source["commit"].lower():
                raise ValueError(f"{game_id}: prepared commit changed: {got}")
            checked[game_id] = {"commit": got, "path": str(path)}
        else:
            archive = Path(str(item.get("archive") or ""))
            got = sha256_file(archive).lower()
            if got != source["sha256"].lower():
                raise ValueError(f"{game_id}: prepared archive changed: {got}")
            checked[game_id] = {
                "sha256": got, "path": str(path), "archive": str(archive)
            }
    return {"ok": True, "ports": checked, "dependencies": checked_dependencies}


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("action", choices=("check", "prepare", "verify"))
    ap.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    ap.add_argument("--catalog", type=Path)
    ap.add_argument("--game", default="all")
    args = ap.parse_args(argv)
    root = args.root.expanduser().resolve()
    catalog = read_catalog(root, args.catalog)
    if args.action == "check":
        print(json.dumps(
            {"ok": True, "ports": [x[0] for x in selected_ports(catalog, args.game)]},
            indent=2
        ))
    elif args.action == "prepare":
        print(json.dumps(prepare(root, catalog, args.game), indent=2, sort_keys=True))
    else:
        print(json.dumps(verify(root, catalog, args.game), indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
