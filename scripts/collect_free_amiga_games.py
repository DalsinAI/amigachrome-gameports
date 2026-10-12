#!/usr/bin/env python3
"""Collect freely downloadable 2026 classic-Amiga games for Instance-32.

The collector is intentionally conservative:
- it downloads only public zero-cost files exposed by the publisher;
- it does not bypass login, payment, or access controls;
- engines requiring original commercial data are kept in a separate drawer;
- every original archive, source URL and SHA-256 is retained;
- an ambiguous or failed item remains in the catalogue rather than disappearing.

Itch downloads are delegated to itchio-downloader, which reports paid,
restricted and no-upload pages without attempting to evade them.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tarfile
import time
import unicodedata
import urllib.parse
import zipfile
from pathlib import Path, PurePosixPath
from typing import Any

import requests
from bs4 import BeautifulSoup

USER_AGENT = "AmigaChrome-free-Amiga-games/1.0 (catalogue and compatibility testing)"
ARCHIVE_EXTENSIONS = (
    ".zip", ".lha", ".lzh", ".lzx", ".7z", ".tar", ".tgz", ".tar.gz",
    ".gz", ".bz2", ".xz", ".adf", ".dms", ".hdf", ".img", ".iso"
)
CATEGORY_DIRS = {
    "ready": "01 Ready to Test",
    "demo": "02 Demos and Betas",
    "needs_data": "03 Needs Original Data",
    "unresolved": "99 Unresolved",
}


def slug(value: str, limit: int = 72) -> str:
    text = unicodedata.normalize("NFKD", value).encode("ascii", "ignore").decode("ascii")
    text = re.sub(r"[^A-Za-z0-9._' -]+", " ", text)
    text = re.sub(r"\s+", " ", text).strip(" .")
    return (text or "Untitled")[:limit].rstrip(" .")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def safe_member(name: str) -> str | None:
    name = name.replace("\\", "/")
    path = PurePosixPath(name)
    if path.is_absolute() or ".." in path.parts:
        return None
    parts = [slug(part, 80) for part in path.parts if part not in ("", ".")]
    if not parts:
        return None
    return "/".join(parts)


def unique_path(path: Path) -> Path:
    if not path.exists():
        return path
    stem, suffix = path.stem, path.suffix
    for number in range(2, 1000):
        candidate = path.with_name(f"{stem}-{number}{suffix}")
        if not candidate.exists():
            return candidate
    raise RuntimeError(f"too many duplicate paths for {path}")


def download_http(session: requests.Session, url: str, destination: Path) -> dict[str, Any]:
    destination.parent.mkdir(parents=True, exist_ok=True)
    with session.get(url, stream=True, timeout=(30, 300), allow_redirects=True) as response:
        response.raise_for_status()
        content_type = (response.headers.get("content-type") or "").lower()
        disposition = response.headers.get("content-disposition") or ""
        filename = ""
        match = re.search(r'filename\*?=(?:UTF-8\'\')?"?([^";]+)', disposition, re.I)
        if match:
            filename = urllib.parse.unquote(match.group(1).strip())
        if not filename:
            filename = Path(urllib.parse.urlparse(response.url).path).name or destination.name
        filename = slug(filename, 120)
        target = unique_path(destination.parent / filename)
        with target.open("wb") as handle:
            for chunk in response.iter_content(1024 * 1024):
                if chunk:
                    handle.write(chunk)
    if target.stat().st_size < 128:
        raise RuntimeError(f"download is implausibly small: {target.stat().st_size} bytes")
    first = target.read_bytes()[:256].lstrip().lower()
    if ("text/html" in content_type or first.startswith((b"<!doctype html", b"<html"))) and target.suffix.lower() not in (".html", ".htm"):
        raise RuntimeError("download resolved to an HTML page rather than a game file")
    return {
        "path": str(target),
        "resolved_url": response.url,
        "content_type": content_type,
        "bytes": target.stat().st_size,
    }


def resolve_aminet(session: requests.Session, page_url: str, work: Path) -> dict[str, Any]:
    response = session.get(page_url, timeout=45)
    response.raise_for_status()
    soup = BeautifulSoup(response.text, "html.parser")
    links: list[str] = []
    for anchor in soup.select("a[href]"):
        href = urllib.parse.urljoin(page_url, anchor.get("href", ""))
        lower = urllib.parse.urlparse(href).path.lower()
        if lower.endswith(ARCHIVE_EXTENSIONS):
            links.append(href)
    if not links:
        parsed = urllib.parse.urlparse(page_url)
        package_path = parsed.path.replace("/package/", "/", 1).rstrip("/")
        for extension in (".lha", ".lzx", ".zip"):
            links.append(urllib.parse.urlunparse(("https", parsed.netloc, package_path + extension, "", "", "")))
    errors = []
    for link in dict.fromkeys(links):
        try:
            return download_http(session, link, work / "aminet-download")
        except Exception as exc:
            errors.append(f"{link}: {exc}")
    raise RuntimeError("Aminet download could not be resolved: " + " | ".join(errors[:6]))


def resolve_github(session: requests.Session, repo_url: str, work: Path) -> dict[str, Any]:
    parsed = urllib.parse.urlparse(repo_url)
    parts = [part for part in parsed.path.split("/") if part]
    if len(parts) < 2:
        raise RuntimeError("not a GitHub repository URL")
    owner, repo = parts[:2]
    headers = {"Accept": "application/vnd.github+json"}
    token = os.environ.get("GH_TOKEN") or os.environ.get("GITHUB_TOKEN")
    if token:
        headers["Authorization"] = f"Bearer {token}"
    candidates: list[dict[str, Any]] = []
    for endpoint in (
        f"https://api.github.com/repos/{owner}/{repo}/releases/latest",
        f"https://api.github.com/repos/{owner}/{repo}/releases?per_page=10",
    ):
        response = session.get(endpoint, headers=headers, timeout=45)
        if response.status_code == 404:
            continue
        response.raise_for_status()
        payload = response.json()
        releases = payload if isinstance(payload, list) else [payload]
        for release in releases:
            for asset in release.get("assets") or []:
                name = asset.get("name") or ""
                lower = name.lower()
                if lower.endswith(ARCHIVE_EXTENSIONS) or any(token in lower for token in ("amiga", "68k", "os3", "aga", "rtg")):
                    candidates.append(asset)
        if candidates:
            break
    if not candidates:
        raise RuntimeError("no downloadable release asset; source repository recorded only")
    candidates.sort(
        key=lambda asset: (
            0 if any(token in (asset.get("name") or "").lower() for token in ("amiga", "68k", "os3", "aga", "rtg")) else 1,
            -int(asset.get("size") or 0),
        )
    )
    asset = candidates[0]
    result = download_http(session, asset["browser_download_url"], work / slug(asset.get("name") or "github-download"))
    result["release_asset"] = asset.get("name")
    return result


def run_itch(helper: Path, node: str, entry: dict[str, Any], work: Path) -> dict[str, Any]:
    command = [node, str(helper), entry["url"], str(work), slug(entry["title"], 90)]
    process = subprocess.run(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        timeout=360,
        check=False,
        env={**os.environ, "NO_COLOR": "1"},
    )
    marker = None
    for line in process.stdout.splitlines():
        if line.startswith("ITCH_RESULT_JSON="):
            marker = line.split("=", 1)[1]
    if marker is None:
        raise RuntimeError(f"itch downloader returned no result (rc={process.returncode}): {process.stdout[-2000:]}")
    payload = json.loads(marker)
    payload["log"] = process.stdout[-8000:]
    if not payload.get("status"):
        return payload
    file_path = Path(payload.get("filePath") or "")
    if not file_path.is_file():
        raise RuntimeError(f"itch downloader reported success without a file: {file_path}")
    payload["path"] = str(file_path)
    payload["bytes"] = file_path.stat().st_size
    return payload


def extract_zip(path: Path, target: Path) -> int:
    count = 0
    with zipfile.ZipFile(path) as archive:
        for info in archive.infolist():
            name = safe_member(info.filename)
            if not name:
                continue
            out = target / name
            if info.is_dir():
                out.mkdir(parents=True, exist_ok=True)
                continue
            out.parent.mkdir(parents=True, exist_ok=True)
            with archive.open(info) as source, out.open("wb") as destination:
                shutil.copyfileobj(source, destination)
            count += 1
    return count


def extract_tar(path: Path, target: Path) -> int:
    count = 0
    with tarfile.open(path, "r:*") as archive:
        for member in archive.getmembers():
            name = safe_member(member.name)
            if not name or not member.isfile():
                continue
            source = archive.extractfile(member)
            if source is None:
                continue
            out = target / name
            out.parent.mkdir(parents=True, exist_ok=True)
            with source, out.open("wb") as destination:
                shutil.copyfileobj(source, destination)
            count += 1
    return count


def extract_lha(path: Path, target: Path) -> int:
    import lhafile  # type: ignore

    count = 0
    with lhafile.LhaFile(str(path)) as archive:
        for info in archive.infolist():
            name = safe_member(info.filename)
            if not name or info.filename.endswith(("/", "\\")):
                continue
            out = target / name
            out.parent.mkdir(parents=True, exist_ok=True)
            out.write_bytes(archive.read(info.filename))
            count += 1
    return count


def extract_7z(path: Path, target: Path) -> int:
    import py7zr  # type: ignore

    with py7zr.SevenZipFile(path, mode="r") as archive:
        names = [safe_member(name) for name in archive.getnames()]
        if any(name is None for name in names):
            raise RuntimeError("7z archive contains an unsafe path")
        archive.extractall(path=target)
        return sum(1 for item in target.rglob("*") if item.is_file())


def extract_archive(path: Path, target: Path) -> tuple[str, int]:
    lower = path.name.lower()
    target.mkdir(parents=True, exist_ok=True)
    try:
        if lower.endswith(".zip"):
            return "zip", extract_zip(path, target)
        if lower.endswith((".tar", ".tar.gz", ".tgz", ".tar.bz2", ".tar.xz")):
            return "tar", extract_tar(path, target)
        if lower.endswith((".lha", ".lzh")):
            return "lha", extract_lha(path, target)
        if lower.endswith(".7z"):
            return "7z", extract_7z(path, target)
    except Exception as exc:
        return f"extract_failed: {exc}", 0
    return "kept_as_file", 0


def copy_metadata(work: Path, target: Path) -> None:
    for metadata in work.glob("*metadata.json"):
        shutil.copy2(metadata, unique_path(target / slug(metadata.name, 120)))


def status_file(folder: Path, entry: dict[str, Any], result: dict[str, Any]) -> None:
    lines = [
        entry["title"],
        "=" * len(entry["title"]),
        "",
        f"Status: {result.get('status_label', result.get('status', 'unknown'))}",
        f"Category: {entry['category']}",
        f"Sources: {', '.join(entry.get('sources') or [])}",
        f"URL: {entry.get('url') or '(unresolved)'}",
    ]
    if entry.get("note"):
        lines.append(f"Note: {entry['note']}")
    if result.get("message"):
        lines.append(f"Message: {result['message']}")
    if result.get("archive"):
        lines.append(f"Archive: {result['archive']}")
    if result.get("sha256"):
        lines.append(f"SHA-256: {result['sha256']}")
    if result.get("bytes") is not None:
        lines.append(f"Bytes: {result['bytes']}")
    if result.get("extract"):
        lines.append(f"Extraction: {result['extract']}")
    folder.mkdir(parents=True, exist_ok=True)
    (folder / "STATUS.txt").write_text("\n".join(lines) + "\n", encoding="latin-1", errors="replace")


def collect(manifest_path: Path, output: Path, helper: Path, node: str) -> list[dict[str, Any]]:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    output.mkdir(parents=True, exist_ok=True)
    for name in CATEGORY_DIRS.values():
        (output / name).mkdir(parents=True, exist_ok=True)
    archive_root = output / "90 Original Archives"
    archive_root.mkdir(parents=True, exist_ok=True)

    session = requests.Session()
    session.headers.update({"User-Agent": USER_AGENT})
    records: list[dict[str, Any]] = []
    scratch_root = output.parent / "_downloads"
    scratch_root.mkdir(parents=True, exist_ok=True)

    for index, entry in enumerate(manifest["entries"], 1):
        title = entry["title"]
        folder_name = slug(title)
        category = entry.get("category") or "unresolved"
        if category not in CATEGORY_DIRS:
            category = "unresolved"
        destination = output / CATEGORY_DIRS[category] / folder_name
        work = scratch_root / f"{index:03d}-{folder_name}"
        shutil.rmtree(work, ignore_errors=True)
        work.mkdir(parents=True, exist_ok=True)
        record: dict[str, Any] = {
            "number": index,
            "title": title,
            "url": entry.get("url"),
            "sources": entry.get("sources") or [],
            "category": category,
            "note": entry.get("note") or "",
            "status": "pending",
        }

        if not entry.get("url"):
            record.update(status="unresolved", status_label="unresolved", message=entry.get("note") or "No verified URL")
            status_file(destination, entry, record)
            records.append(record)
            print(f"[{index}/{len(manifest['entries'])}] unresolved {title}", flush=True)
            continue

        url = entry["url"]
        try:
            host = urllib.parse.urlparse(url).netloc.lower()
            if host.endswith("itch.io"):
                result = run_itch(helper, node, entry, work)
                if not result.get("status"):
                    fail = result.get("failReason") or "download_failed"
                    record.update(
                        status=fail,
                        status_label=fail,
                        message=result.get("message") or "itch download failed",
                        downloader_log=result.get("log", ""),
                    )
                    status_file(destination, entry, record)
                    records.append(record)
                    print(f"[{index}/{len(manifest['entries'])}] {fail} {title}: {record['message']}", flush=True)
                    continue
            elif host.endswith("aminet.net"):
                result = resolve_aminet(session, url, work)
            elif host.endswith("github.com"):
                try:
                    result = resolve_github(session, url, work)
                except Exception as exc:
                    record.update(status="source_only", status_label="source only", message=str(exc))
                    status_file(destination, entry, record)
                    records.append(record)
                    print(f"[{index}/{len(manifest['entries'])}] source-only {title}", flush=True)
                    continue
            else:
                result = download_http(session, url, work / "download")

            downloaded = Path(result["path"])
            if downloaded.stat().st_size > 1024 * 1024 * 1024:
                raise RuntimeError("single download exceeds the 1 GiB safety limit")

            archive_dir = archive_root / folder_name
            archive_dir.mkdir(parents=True, exist_ok=True)
            archive_path = unique_path(archive_dir / slug(downloaded.name, 120))
            shutil.copy2(downloaded, archive_path)
            digest = sha256(archive_path)

            destination.mkdir(parents=True, exist_ok=True)
            method, extracted = extract_archive(archive_path, destination)
            if method == "kept_as_file" or extracted == 0:
                shutil.copy2(archive_path, unique_path(destination / archive_path.name))
            copy_metadata(work, destination)

            record.update(
                status="downloaded",
                status_label="downloaded",
                message=result.get("message") or "downloaded",
                archive=str(archive_path.relative_to(output)),
                filename=archive_path.name,
                bytes=archive_path.stat().st_size,
                sha256=digest,
                extract=method,
                extracted_files=extracted,
                resolved_url=result.get("resolved_url") or url,
            )
            status_file(destination, entry, record)
            records.append(record)
            print(
                f"[{index}/{len(manifest['entries'])}] downloaded {title}: "
                f"{archive_path.stat().st_size} bytes, {method}, {extracted} files",
                flush=True,
            )
        except subprocess.TimeoutExpired:
            record.update(status="timeout", status_label="timeout", message="download exceeded six minutes")
            status_file(destination, entry, record)
            records.append(record)
            print(f"[{index}/{len(manifest['entries'])}] timeout {title}", flush=True)
        except Exception as exc:
            record.update(status="error", status_label="error", message=str(exc))
            status_file(destination, entry, record)
            records.append(record)
            print(f"[{index}/{len(manifest['entries'])}] error {title}: {exc}", flush=True)
        time.sleep(0.20)
    return records


def write_catalogue(output: Path, records: list[dict[str, Any]], manifest: dict[str, Any]) -> None:
    counts: dict[str, int] = {}
    for record in records:
        counts[record["status"]] = counts.get(record["status"], 0) + 1

    lines = [
        "New Amiga Games - 2026 free-download test collection",
        "=====================================================",
        "",
        f"As of: {manifest.get('as_of')}",
        f"Manifest entries: {len(records)}",
        f"Downloaded packages: {counts.get('downloaded', 0)}",
        f"Paid/restricted pages skipped: {counts.get('paid', 0)}",
        f"Unresolved entries: {counts.get('unresolved', 0)}",
        f"Other failures/source-only items: {len(records) - counts.get('downloaded', 0) - counts.get('paid', 0) - counts.get('unresolved', 0)}",
        "",
        "No payment, login or access control was bypassed. Engines requiring original",
        "commercial data are separated under 03 Needs Original Data; no proprietary",
        "game data was downloaded for them.",
        "",
        "Status legend: DOWNLOADED, PAID, NO_UPLOADS, SOURCE_ONLY, UNRESOLVED, ERROR.",
        "",
    ]
    for record in records:
        lines.extend([
            f"{record['number']:03d}. {record['title']}",
            f"     Status: {record['status'].upper()}",
            f"     Group: {CATEGORY_DIRS.get(record['category'], CATEGORY_DIRS['unresolved'])}",
            f"     Sources: {', '.join(record['sources'])}",
            f"     URL: {record.get('url') or '(unresolved)'}",
        ])
        if record.get("filename"):
            lines.append(
                f"     File: {record['filename']} ({record.get('bytes', 0)} bytes; "
                f"SHA-256 {record.get('sha256')})"
            )
        if record.get("message"):
            lines.append(f"     Note: {record['message']}")
        if record.get("note"):
            lines.append(f"     Rights/data note: {record['note']}")
        lines.append("")

    (output / "CATALOGUE.txt").write_text("\n".join(lines), encoding="latin-1", errors="replace")
    (output / "CATALOGUE.json").write_text(
        json.dumps(
            {
                "schema": 1,
                "as_of": manifest.get("as_of"),
                "scope": manifest.get("scope"),
                "counts": counts,
                "entries": records,
            },
            indent=2,
            ensure_ascii=True,
        )
        + "\n",
        encoding="utf-8",
    )

    source_lines = [
        "Sources",
        "=======",
        "",
        "Primary editorial sources reviewed:",
        "- Amigos Retro Gaming (itch.io creator catalogue)",
        "- Amigatronics",
        "- Generation Amiga",
        "",
        "A current 2026 free-game roundup was used only as a completeness",
        "cross-check where the named editorial sources did not expose a clean index.",
        "",
    ]
    for record in records:
        source_lines.append(f"{record['title']} | {', '.join(record['sources'])} | {record.get('url') or 'unresolved'}")
    (output / "SOURCES.txt").write_text("\n".join(source_lines) + "\n", encoding="latin-1", errors="replace")

    readme = """New Amiga Games
===============

This drawer is a compatibility-test collection assembled from public,
zero-cost classic-Amiga downloads covered during 2026 by Amigos Retro Gaming,
Amigatronics and Generation Amiga.

Open CATALOGUE.txt first. Each game drawer has STATUS.txt with its source,
download result and archive hash.

01 Ready to Test
    Full or apparently complete zero-cost downloads.

02 Demos and Betas
    Demonstrations, alphas, betas, prototypes and work-in-progress releases.

03 Needs Original Data
    Free engines and ports. Supply only data from games you legally own.

90 Original Archives
    The unmodified public downloads, retained with SHA-256 hashes.

99 Unresolved
    Paid, restricted, missing, source-only or ambiguous items. No restriction
    was bypassed, and no commercial game data was copied.

Some entries may be ADF, DMS, HDF or archive files rather than installed
program drawers. They are deliberately retained as published for testing.
"""
    (output / "README.txt").write_text(readme, encoding="latin-1")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--itch-helper", required=True, type=Path)
    parser.add_argument("--node", default="node")
    args = parser.parse_args()

    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    if args.out.exists():
        shutil.rmtree(args.out)
    records = collect(args.manifest, args.out, args.itch_helper, args.node)
    write_catalogue(args.out, records, manifest)

    downloaded = sum(1 for record in records if record["status"] == "downloaded")
    if downloaded == 0:
        raise SystemExit("collector downloaded no game packages")
    print(json.dumps({
        "entries": len(records),
        "downloaded": downloaded,
        "bytes": sum(record.get("bytes", 0) for record in records),
        "statuses": {status: sum(1 for record in records if record["status"] == status)
                     for status in sorted({record["status"] for record in records})},
    }, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
