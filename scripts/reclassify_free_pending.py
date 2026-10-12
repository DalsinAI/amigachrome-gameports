#!/usr/bin/env python3
"""Apply the Free Pending Verification policy to a deployed game catalogue."""
from __future__ import annotations

import argparse
import importlib.util
import json
import re
import shutil
from pathlib import Path
from typing import Any

PENDING_DIR = "04 Free Pending Verification"
UNRESOLVED_DIR = "99 Unresolved"


def slug(value: str, limit: int = 80) -> str:
    value = re.sub(r"[^A-Za-z0-9._' -]+", " ", value)
    return re.sub(r"\s+", " ", value).strip(" .")[:limit].rstrip(" .") or "Untitled"


def archive_existing(path: Path, destination: Path) -> None:
    if not path.exists():
        return
    destination.parent.mkdir(parents=True, exist_ok=True)
    candidate = destination
    suffix = 2
    while candidate.exists():
        candidate = destination.with_name(f"{destination.name}-{suffix}")
        suffix += 1
    shutil.move(str(path), str(candidate))


def load_collector(path: Path):
    spec = importlib.util.spec_from_file_location("free_game_collector", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Cannot load collector from {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def write_status(drawer: Path, row: dict[str, Any], policy: str) -> None:
    drawer.mkdir(parents=True, exist_ok=True)
    lines = [
        row["title"],
        "=" * len(row["title"]),
        "",
        "Status: FREE PENDING VERIFICATION",
        "Category: pending_free",
        f"Sources: {', '.join(row.get('sources') or [])}",
        f"URL: {row.get('url') or '(official/developer source still to be resolved)'}",
        "",
        "This entry is provisionally treated as free for compatibility testing.",
        "Final classification must be based on the supplied README/license,",
        "installer text, title screen, in-game notices or direct developer evidence.",
        "No payment, login or access restriction may be bypassed.",
        "",
        f"Policy: {policy}",
    ]
    (drawer / "STATUS.txt").write_text("\n".join(lines) + "\n", encoding="latin-1", errors="replace")

    review = [
        "Free Status Review",
        "==================",
        "",
        "[ ] Locate the developer/publisher-associated download.",
        "[ ] Preserve the original archive and calculate SHA-256.",
        "[ ] Read README, license, installer and distribution notes.",
        "[ ] Run the game and inspect title, about, registration and exit screens.",
        "[ ] Record one of: freeware, shareware, donationware, demo, beta,",
        "    non-commercial, requires original data, commercial, or unclear.",
        "[ ] Move to Ready/Demos/Needs Original Data only when evidence supports it.",
        "",
        "Evidence to capture:",
        "- Exact text and filename/screen where it appears",
        "- Developer or publisher attribution",
        "- Version/build date",
        "- Any redistribution or commercial-data restriction",
    ]
    (drawer / "REVIEW.txt").write_text("\n".join(review) + "\n", encoding="latin-1")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", required=True, type=Path)
    parser.add_argument("--policy", required=True, type=Path)
    parser.add_argument("--collector", required=True, type=Path)
    parser.add_argument("--backup", required=True, type=Path)
    args = parser.parse_args()

    root = args.root.resolve()
    backup = args.backup.resolve()
    policy_doc = json.loads(args.policy.read_text(encoding="utf-8"))
    catalogue_path = root / "CATALOGUE.json"
    catalogue = json.loads(catalogue_path.read_text(encoding="utf-8"))

    backup.mkdir(parents=True, exist_ok=True)
    for name in ("CATALOGUE.json", "CATALOGUE.txt", "SUMMARY.txt", "README.txt"):
        path = root / name
        if path.is_file():
            shutil.copy2(path, backup / f"{name}.before")

    by_title = {row["title"].casefold(): row for row in catalogue["entries"]}
    pending_root = root / PENDING_DIR
    pending_root.mkdir(parents=True, exist_ok=True)
    changed: list[str] = []

    for title in policy_doc["entries"]:
        row = by_title.get(title.casefold())
        if row is None:
            raise RuntimeError(f"Policy title is absent from catalogue: {title}")

        old_drawer = root / UNRESOLVED_DIR / slug(title)
        new_drawer = pending_root / slug(title)
        if old_drawer.exists() and old_drawer != new_drawer:
            if new_drawer.exists():
                archive_existing(new_drawer, backup / "conflicting-pending-drawers" / new_drawer.name)
            shutil.move(str(old_drawer), str(new_drawer))
        else:
            new_drawer.mkdir(parents=True, exist_ok=True)

        row["category"] = "pending_free"
        row["status"] = "pending_free"
        row["status_label"] = "free pending verification"
        row["message"] = (
            "Provisionally treated as free pending README, license and in-game verification. "
            "No payment or access restriction may be bypassed."
        )
        note = row.get("note") or ""
        policy_note = "Free status is provisional until supplied documentation or the running game provides evidence."
        if policy_note not in note:
            row["note"] = (note + " " + policy_note).strip()
        write_status(new_drawer, row, policy_doc["policy"])
        changed.append(title)

    counts: dict[str, int] = {}
    for row in catalogue["entries"]:
        counts[row["status"]] = counts.get(row["status"], 0) + 1
    catalogue["counts"] = counts
    catalogue["free_pending_policy"] = policy_doc["policy"]
    catalogue_path.write_text(json.dumps(catalogue, indent=2, ensure_ascii=True) + "\n", encoding="utf-8")

    collector = load_collector(args.collector)
    collector.CATEGORY_DIRS["pending_free"] = PENDING_DIR
    collector.write_catalogue(root, catalogue["entries"], {
        "as_of": catalogue.get("as_of"),
        "scope": catalogue.get("scope"),
    })

    summary = [
        "New Amiga Games - Deployment Summary",
        "=====================================",
        "",
        f"As of: {catalogue.get('as_of')}",
        f"Manifest entries: {len(catalogue['entries'])}",
        f"Downloaded packages: {counts.get('downloaded', 0)}",
        f"Free pending verification: {counts.get('pending_free', 0)}",
        f"Paid/restricted pages skipped: {counts.get('paid', 0)}",
        "",
        "Ambiguous public developer-associated downloads are treated as free pending",
        "inspection of README/license files and the running game. They are not excluded.",
        "No payment, login or access restriction is bypassed.",
        "",
        "Open CATALOGUE.txt for the full game-by-game list.",
    ]
    (root / "SUMMARY.txt").write_text("\n".join(summary) + "\n", encoding="latin-1")

    policy_lines = [
        "Free Pending Verification Policy",
        "================================",
        "",
        policy_doc["policy"],
        "",
        "Current pending entries:",
    ] + [f"- {title}" for title in changed]
    (root / "PENDING-VERIFICATION.txt").write_text("\n".join(policy_lines) + "\n", encoding="latin-1", errors="replace")

    print(json.dumps({
        "changed": changed,
        "counts": counts,
        "pending_drawer": str(pending_root),
    }, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
