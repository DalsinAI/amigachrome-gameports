# AmigaChrome Game Ports

Open-source game-port and compatibility work for **m68k AROS**, **AmigaOS 3.x** and **AmigaChrome**.

The repository contains compiled revival ports, the Neverball/OpenGPU release lane, and the next 3D source-intake targets. Compile success is useful evidence, but it is **not** synonymous with release readiness. See [RELEASE_POLICY.md](RELEASE_POLICY.md).

## Current port state

| Port | Build state | Runtime state | Target / notes |
|---|---|---|---|
| The Ur-Quan Masters 0.8.0 | COMPILED | PENDING | m68k / 68040 |
| Chocolate Doom | COMPILED | PENDING | m68k / 68040 |
| SDLPoP | COMPILED | PENDING | m68k / 68040 |
| OpenJazz | COMPILED | PENDING | m68k / 68040 |
| NXEngine-evo | COMPILED | PENDING | m68k / 68040 |
| Neverball / Neverputt | COMPILED + PACKAGE VERIFIED | TESTING | softpipe title/menu renders; accelerated virgl black-frame issue remains |
| AssaultCube | CLIENT + SERVER COMPILED | PENDING | OpenGPU first-light client; audio still stubbed for initial bring-up |
| Serious Sam Classic | SOURCE PINNED | NOT STARTED | TFE first |
| Warzone 2100 | SOURCE PINNED | NOT STARTED | later large 3D/application stress target |
| Doom 3 | SOURCE PINNED | NOT STARTED | long-range renderer/engine stress target |
| OpenLara | SOURCE PINNED | NOT STARTED | 3D engine / OpenGPU target |
| OpenTTD | SOURCE PINNED | NOT STARTED | large C++/SDL application target |
| The Battle for Wesnoth | SOURCE PINNED | NOT STARTED | large C++/SDL strategy target |
| The Dark Mod | SOURCE PINNED | NOT STARTED | long-range idTech 4-derived stretch target |
| Hedgewars | SOURCE PINNED | NOT STARTED | SDL/audio/networking/physics target |

See [PORT_STATUS.md](PORT_STATUS.md) and the individual files under `ports/` for exact source pins, hashes, content boundaries and test gates.

## Repository layout

- `scripts/` — source preparation, build orchestration and field-test staging code.
- `port-layer/acgame/` — AmigaChrome AGA presentation code, smoke test and unit tests.
- `port-layer/uqm/` — UQM AROS bootstrap patcher and port notes.
- `ports/*/` — per-port build recipes, patches, release notes and qualification gates.
- `toolchain/aros-local/` — the AROS m68k toolchain bootstrap recovered from the AmigaChrome worktree.
- `gameports/catalog.json` — exact upstream source pins and content policy.

See [BUILDING.md](BUILDING.md) for the rebuild path.

## Source preparation and network policy

Upstream engine source remains in the upstream repositories and is referenced by exact pinned revisions or archive hashes.

Source preparation is **offline by default**. Existing valid prepared sources and cached archives are reused locally. If preparation would require an external fetch, it stops unless the caller explicitly supplies `--allow-network` after approval. An existing build or preparation script is not permission to access the network.

Game data, ROMs, proprietary assets and local test caches are **not** part of this repository unless a port's upstream licence explicitly permits redistribution and the release recipe records that provenance.

## Status language

- **SOURCE PINNED** means the exact upstream input has been selected but no successful target build is claimed.
- **COMPILED** means the pinned source has produced the target m68k executable.
- **FIRST LIGHT** means the executable has launched on an AmigaChrome guest.
- **TESTING** means runtime qualification is underway and at least one required release gate remains open.
- **TESTED** means the stated graphics, input, audio, persistence and clean-exit gates have been exercised.
- **RELEASE** means the port has passed its complete release gates, package/provenance checks and clean-instance runtime qualification.

**DONE means RELEASE.** Compiled, First Light and Testing are intermediate states.

## Licence

AmigaChrome-authored glue, scripts and documentation are MIT licensed unless a file says otherwise. Upstream projects retain their own licences. Game data retains its own rights and is not redistributed unless its licence and the port's release policy permit it.

No upstream game engine source is committed here. Prepared sources live outside Git at their pinned revision. Binaries built from them remain subject to the upstream licence, including source-offer and notice obligations where applicable.

| Port | Upstream | Licence / data boundary |
|---|---|---|
| The Ur-Quan Masters 0.8.0 | https://sc2.sourceforge.net/ | GPL-2.0-or-later code; content packages separate |
| Chocolate Doom | https://github.com/chocolate-doom/chocolate-doom | GPL-2.0-or-later; IWAD external |
| SDLPoP | https://github.com/NagyD/SDLPoP | GPL-3.0-or-later; commercial game data external |
| OpenJazz | https://github.com/AlisterT/openjazz | GPL-2.0-or-later; Jazz data external |
| NXEngine-evo | https://github.com/nxengine/nxengine-evo | GPL-3.0; Cave Story data external |
| Neverball | https://github.com/Neverball/neverball | GPL-2.0-or-later with upstream third-party notices/data terms preserved |
| AssaultCube | https://github.com/assaultcube/AC | zlib-like engine source; media licences reviewed separately |
| Serious Sam Classic | https://github.com/tx00100xt/SeriousSamClassic | GPL-2.0 engine; original game data external |
| Warzone 2100 | https://github.com/Warzone2100/warzone2100 | GPL-2.0 project plus third-party/submodule notices |
| Doom 3 | https://github.com/id-Software/DOOM-3 | GPL-3.0 source; original game data external |
| OpenLara | https://github.com/XProger/OpenLara | engine source only for intake; original Tomb Raider data external |
| OpenTTD | https://github.com/OpenTTD/OpenTTD | preserve upstream and third-party notices; runtime asset packs reviewed separately |
| The Battle for Wesnoth | https://github.com/wesnoth/wesnoth | preserve upstream code/data/translation licensing |
| The Dark Mod | https://github.com/stgatilov/darkmod_src | official engine mirror; missions/full installation external |
| Hedgewars | https://github.com/hedgewars/hw | preserve upstream code and mixed asset licensing |

Each project's own licence files at the pinned revision are authoritative.

Copyright (c) 2026 Dalsin Limited.

## Contributors

AmigaChrome Game Ports is created and maintained by [SacredTrees](https://github.com/SacredTrees) with the AmigaChrome agent team, copyright Dalsin Limited. Everyone whose work it includes is credited in [`CONTRIBUTORS.md`](CONTRIBUTORS.md).
