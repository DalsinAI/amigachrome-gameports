# AmigaChrome Game Ports

Open-source game-port and compatibility work for **m68k AROS** and **AmigaChrome**.

This repository is being populated early as a public engineering safety snapshot. The first ports listed here have **compiled successfully for m68k AROS / 68040**, but they have **not yet completed first-light runtime testing on AmigaChrome Instance-22**.

## Current compiled ports

| Port | Compile | Runtime test | Target |
|---|---|---|---|
| The Ur-Quan Masters 0.8.0 | PASS | PENDING | m68k AROS / 68040 |
| Chocolate Doom | PASS | PENDING | m68k AROS / 68040 |
| SDLPoP | PASS | PENDING | m68k AROS / 68040 |
| OpenJazz | PASS | PENDING | m68k AROS / 68040 |
| NXEngine-evo | PASS | PENDING | m68k AROS / 68040 |

See [PORT_STATUS.md](PORT_STATUS.md) for exact upstream pins, hashes and test gates.


## Repository layout

- `scripts/` — source preparation, build orchestration and field-test staging code.
- `port-layer/acgame/` — AmigaChrome AGA presentation code, smoke test and unit tests.
- `port-layer/uqm/` — UQM AROS bootstrap patcher and port notes.
- `ports/*/build.sh` — standalone reproducible build recipes for the five compile-success ports.
- `toolchain/aros-local/` — the AROS m68k toolchain bootstrap recovered from the AmigaChrome worktree.
- `gameports/catalog.json` — exact upstream source pins.

See [BUILDING.md](BUILDING.md) for a clean rebuild path on another Linux machine.

## Scope

The repository contains AmigaChrome build recipes, compatibility notes, shims, test plans and source provenance. Upstream engine source remains in the upstream repositories and is fetched at exact pinned revisions.

Game data, ROMs, proprietary assets and local test caches are **not** part of this repository.

## Status language

- **COMPILED** means the pinned source has produced an m68k AROS executable.
- **FIRST LIGHT** means the executable has launched on an AmigaChrome A1200/AGA guest.
- **TESTED** means graphics, input, audio and clean exit have been exercised.
- **AGA** means the port has passed its native/AmigaChrome AGA presentation gate.

Compiled is deliberately not treated as tested.

## Licence

AmigaChrome-authored glue, scripts and documentation are MIT licensed unless a file says otherwise. Upstream projects retain their own licences. Game data retains its own rights and is not redistributed here.

No upstream game source is committed here: the build recipes fetch each project at its pinned commit. Binaries built from them are covered by the upstream licence, so anyone distributing them must follow it, including offering the corresponding source.

| Port | Upstream | Licence |
|---|---|---|
| The Ur-Quan Masters 0.8.0 | https://sc2.sourceforge.net/ | GPL-2.0 (code); content packages under CC BY-NC-SA 2.5 |
| Chocolate Doom | https://github.com/chocolate-doom/chocolate-doom | GPL-2.0-or-later |
| SDLPoP | https://github.com/NagyD/SDLPoP | GPL-3.0-or-later |
| OpenJazz | https://github.com/AlisterT/openjazz | GPL-2.0-or-later |
| NXEngine-evo | https://github.com/nxengine/nxengine-evo | GPL-3.0 |

Each project's own licence file at the pinned commit is authoritative.

Copyright (c) 2026 Dalsin Limited.
