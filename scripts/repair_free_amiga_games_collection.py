#!/usr/bin/env python3
"""Repair a deployed New Amiga Games drawer and merge incremental downloads.

Repairs performed:
- rename itch downloads that are raw 880 KiB/1760 KiB Amiga disk images but
  were saved with a .zip suffix;
- extract LHA archives into their per-game test drawers while preserving the
  original archive;
- merge incremental download records into the catalogue;
- regenerate the human-readable catalogue and summary.

Nothing is deleted. Replaced status drawers and files are moved to the caller's
archive directory first.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import re
import shutil
from pathlib import Path, PurePosixPath
from typing import Any

import lhafile  # type: ignore

CATEGORY_DIRS = {
    "ready": "01 Ready to Test",
    "demo": "02 Demos and Betas",
    "needs_data": "03 Needs Original Data",
    "unresolved": "99 Unresolved",
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def slug(value: str, limit: int = 80) -> str:
    value = re.sub(r"[^A-Za-z0-9._' -]+", " ", value)
    return re.sub(r"\s+", " ", value).strip(" .")[:limit].rstrip(" .") or "Untitled"


def safe_member(name: str) -> str | None:
    path = PurePosixPath(name.replace("\\", "/"))
    if path.is_absolute() or ".." in path.parts:
        return None
    parts = [slug(part) for part in path.parts if part not in ("", ".")]
    return "/".join(parts) if parts else None


def category_folder(root: Path, record: dict[str, Any]) -> Path:
    return root / CATEGORY_DIRS.get(record.get("category"), CATEGORY_DIRS["unresolved"]) / slug(record["title"])


def archive_existing(path: Path, backup: Path) -> None:
    if not path.exists():
        return
    backup.parent.mkdir(parents=True, exist_ok=True)
    candidate = backup
    number = 2
    while candidate.exists():
        candidate = backup.with_name(f"{backup.name}-{number}")
        number += 1
    shutil.move(str(path), str(candidate))


def rename_false_zip_to_adf(root: Path, record: dict[str, Any]) -> bool:
    archive_rel = record.get("archive")
    if not archive_rel:
        return False
    archive = root / archive_rel
    if archive.suffix.lower() != ".zip" or not archive.is_file():
        return False
    head = archive.read_bytes()[:4]
    if head.startswith(b"PK\x03\x04"):
        return False
    if archive.stat().st_size not in (901120, 1802240):
        return False

    new_archive = archive.with_suffix(".adf")
    if new_archive.exists():
        if sha256(new_archive) != sha256(archive):
            raise RuntimeError(f"ADF target already exists with different content: {new_archive}")
        archive.unlink()
    else:
        archive.rename(new_archive)

    drawer = category_folder(root, record)
    old_name = record.get("filename") or archive.name
    old_copy = drawer / old_name
    new_copy = drawer / new_archive.name
    if old_copy.exists():
        if new_copy.exists():
            if sha256(new_copy) != sha256(old_copy):
                raise RuntimeError(f"ADF drawer target exists with different content: {new_copy}")
            old_copy.unlink()
        else:
            old_copy.rename(new_copy)

    record["archive"] = str(new_archive.relative_to(root))
    record["filename"] = new_archive.name
    record["extract"] = "adf"
    record["extracted_files"] = 0
    return True


def extract_lha(root: Path, record: dict[str, Any]) -> int:
    archive_rel = record.get("archive")
    if not archive_rel:
        return 0
    archive_path = root / archive_rel
    if archive_path.suffix.lower() not in (".lha", ".lzh") or not archive_path.is_file():
        return 0
    target = category_folder(root, record)
    target.mkdir(parents=True, exist_ok=True)
    archive = lhafile.LhaFile(str(archive_path))
    count = 0
    try:
        for info in archive.infolist():
            name = safe_member(info.filename)
            if not name or info.filename.endswith(("/", "\\")):
                continue
            out = target / name
            out.parent.mkdir(parents=True, exist_ok=True)
            data = archive.read(info.filename)
            if out.exists() and out.read_bytes() == data:
                continue
            out.write_bytes(data)
            count += 1
    finally:
        close = getattr(archive, "close", None)
        if close:
            close()
    record["extract"] = "lha"
    record["extracted_files"] = sum(1 for path in target.rglob("*") if path.is_file() and path.name != "STATUS.txt")
    return count


def merge_incremental(root: Path, backup: Path, catalogue: dict[str, Any], incremental: Path) -> list[str]:
    if not incremental.is_file():
        return []
    payload = json.loads(incremental.read_text(encoding="utf-8"))
    merged = []
    by_title = {row["title"].casefold(): row for row in catalogue["entries"]}
    for item in payload.get("entries") or []:
        title = item["title"]
        source_file = Path(item["file"])
        if not source_file.is_file():
            raise RuntimeError(f"incremental file missing: {source_file}")
        category = item.get("category") or "demo"
        if category not in CATEGORY_DIRS:
            category = "demo"
        drawer = root / CATEGORY_DIRS[category] / slug(title)
        old = None
        for group in CATEGORY_DIRS.values():
            candidate = root / group / slug(title)
            if candidate.exists() and candidate != drawer:
                old = candidate
                break
        if old:
            archive_existing(old, backup / "replaced-status-drawers" / old.parent.name / old.name)
        if drawer.exists():
            archive_existing(drawer, backup / "replaced-game-drawers" / drawer.parent.name / drawer.name)
        drawer.mkdir(parents=True, exist_ok=True)

        archive_dir = root / "90 Original Archives" / slug(title)
        archive_dir.mkdir(parents=True, exist_ok=True)
        extension = item.get("extension") or source_file.suffix
        filename = slug(title, 100) + extension
        archive_target = archive_dir / filename
        if archive_target.exists():
            archive_existing(archive_target, backup / "replaced-archives" / slug(title) / filename)
        shutil.copy2(source_file, archive_target)
        shutil.copy2(source_file, drawer / filename)
        metadata = item.get("metadata")
        if metadata and Path(metadata).is_file():
            shutil.copy2(metadata, drawer / "itch-metadata.json")

        record = by_title.get(title.casefold())
        if record is None:
            record = {
                "number": len(catalogue["entries"]) + 1,
                "title": title,
                "sources": item.get("sources") or [],
                "note": item.get("note") or "",
            }
            catalogue["entries"].append(record)
            by_title[title.casefold()] = record
        record.update({
            "url": item.get("url"),
            "sources": item.get("sources") or record.get("sources") or [],
            "category": category,
            "note": item.get("note") or record.get("note") or "",
            "status": "downloaded",
            "status_label": "downloaded",
            "message": item.get("message") or "Downloaded from the publisher's public zero-cost page.",
            "archive": str(archive_target.relative_to(root)),
            "filename": filename,
            "bytes": archive_target.stat().st_size,
            "sha256": sha256(archive_target),
            "extract": extension.lstrip(".") or "file",
            "extracted_files": 0,
            "resolved_url": item.get("url"),
        })
        status = [
            title,
            "=" * len(title),
            "",
            "Status: downloaded",
            f"Category: {category}",
            f"Sources: {', '.join(record['sources'])}",
            f"URL: {record['url']}",
            f"File: {filename}",
            f"Bytes: {record['bytes']}",
            f"SHA-256: {record['sha256']}",
        ]
        if record.get("note"):
            status.append(f"Note: {record['note']}")
        (drawer / "STATUS.txt").write_text("\n".join(status) + "\n", encoding="latin-1", errors="replace")
        merged.append(title)
    return merged


def load_catalogue_writer(path: Path):
    spec = importlib.util.spec_from_file_location("collector", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load catalogue writer: {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", required=True, type=Path)
    parser.add_argument("--backup", required=True, type=Path)
    parser.add_argument("--collector", required=True, type=Path)
    parser.add_argument("--incremental", type=Path)
    args = parser.parse_args()

    root = args.root.resolve()
    backup = args.backup.resolve()
    catalogue_path = root / "CATALOGUE.json"
    catalogue = json.loads(catalogue_path.read_text(encoding="utf-8"))
    backup.mkdir(parents=True, exist_ok=True)
    shutil.copy2(catalogue_path, backup / "CATALOGUE.before.json")
    for name in ("CATALOGUE.txt", "SUMMARY.txt", "SOURCES.txt"):
        if (root / name).is_file():
            shutil.copy2(root / name, backup / f"{name}.before")

    merged = merge_incremental(root, backup, catalogue, args.incremental) if args.incremental else []
    renamed = 0
    lha_files = 0
    for record in catalogue["entries"]:
        if record.get("status") != "downloaded":
            continue
        if rename_false_zip_to_adf(root, record):
            renamed += 1
        lha_files += extract_lha(root, record)

    counts: dict[str, int] = {}
    for record in catalogue["entries"]:
        counts[record["status"]] = counts.get(record["status"], 0) + 1
    catalogue["counts"] = counts
    catalogue_path.write_text(json.dumps(catalogue, indent=2, ensure_ascii=True) + "\n", encoding="utf-8")

    writer = load_catalogue_writer(args.collector)
    writer.write_catalogue(root, catalogue["entries"], {
        "as_of": catalogue.get("as_of"),
        "scope": catalogue.get("scope"),
    })
    downloaded = counts.get("downloaded", 0)
    total = sum(int(record.get("bytes", 0)) for record in catalogue["entries"])
    summary = [
        "New Amiga Games - Deployment Summary",
        "=====================================",
        "",
        f"As of: {catalogue.get('as_of')}",
        f"Manifest entries: {len(catalogue['entries'])}",
        f"Downloaded packages: {downloaded}",
        f"Downloaded archive bytes: {total}",
        f"ADF filenames repaired: {renamed}",
        f"LHA files made directly testable: {lha_files}",
        "",
        "Open CATALOGUE.txt for the full game-by-game list.",
        "No paid access or login restriction was bypassed.",
    ]
    (root / "SUMMARY.txt").write_text("\n".join(summary) + "\n", encoding="latin-1")
    print(json.dumps({
        "merged": merged,
        "renamed_adf": renamed,
        "lha_files_written": lha_files,
        "counts": counts,
        "archive_bytes": total,
        "files": sum(1 for path in root.rglob("*") if path.is_file()),
    }, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
