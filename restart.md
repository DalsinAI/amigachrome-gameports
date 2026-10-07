# Restart: AmigaChrome Game Ports

_Written 6 October 2026 at about 23:55 UTC, while all work is paused on @SacredTrees's word (23:28 UTC). Read this first when work resumes; the newest capsule and the live PR list win if they disagree._

## What this repo is

Open-source games ported to 68k AROS and AmigaOS on AmigaChrome: UQM, Chocolate Doom, SDLPoP, OpenJazz, NXEngine-evo and more, with upstream sources fetched at build time.

## Where it stands

Quiet. The README lists each port's upstream licence (#1) and CONTRIBUTORS.md is in (#2). Game porting is parked on @SacredTrees's word; Thufir's sprint notes are in /mnt/project-files/games/thufir-game-ports.md.

## Merged lately

- #2 (987505b, 2026-10-06): Credit who made AmigaChrome Game Ports: CONTRIBUTORS.md
- #1 (99f0107, 2026-10-04): README: list each port's upstream licence

## Open pull requests

- None.

## Next step

1. When unparked: rebuild the ports against openamigasdl (SDL 2) and ACGame from amigachrome-guest #23.

## Waiting on @SacredTrees

- The go to unpark game porting.

## Who owns it

No active thread (Thufir's sprint; Gamepad thread for pads).

## Capsules

Restart capsules for this repo's workstreams, in amigachrome's `capjumps/` shelf:

- [`20261006_AmigaChrome_Pause_Restart_Capsule.zip`](https://github.com/DalsinAI/amigachrome/tree/main/capjumps)

Team rules that still hold: commits as SacredTrees with no co-author lines; third-party code only on "yes with review" (licence checked, commit and sha256 pinned, fetched at build, never committed); deploys with deploy_dev.py only, on a typed line.
