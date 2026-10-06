# Contributors

## Creator and maintainer

- **SacredTrees** ([@SacredTrees](https://github.com/SacredTrees)): created AmigaChrome Game Ports, designs it and maintains it.

## The AmigaChrome team

We are the AI agents who build AmigaChrome alongside SacredTrees:

- **Agnus**, our coordinator, who keeps every thread moving.
- **Thufir**, **Kynes** and **Galen**, the earlier agents who started the work on SacredTrees's PC. Thufir also made the AROS game ports in this repository.
- **The Claude Code threads**, each one taking a piece of the work from design to release.

## Copyright holder

Our glue, scripts, build recipes, port layer (`port-layer/`) and documentation are Copyright (c) 2026 Dalsin Limited, released under the MIT licence (`LICENSE`) unless a file says otherwise. The games we port are not ours: each engine stays copyright its authors under its own licence, and game data keeps its own rights.

## Fetched at build time, not committed

No upstream game source is in this repository. The build recipes fetch each project at the pin in `gameports/catalog.json` (also in `PORT_STATUS.md`), and anyone distributing binaries built from them must follow that project's licence.

| Component | Pin | Authors | Licence |
| --- | --- | --- | --- |
| The Ur-Quan Masters 0.8.0 | `uqm-0.8.0-src.tgz`, SHA-256 `24f2f7db…4f5a` | The UQM team (core team Serge van den Boom, Mika Kolehmainen, Michael Chapman Martin, Chris Nelson and Alex Volkov, and the others in its `AUTHORS`), from Star Control II by Fred Ford and Paul Reiche III, copyright Toys for Bob, Inc. | GPL-2.0-or-later (code); content CC BY-NC-SA 2.5 |
| Chocolate Doom | `895f581c5d91497bdda0516612da803fe5843e28` | Simon Howard, James Haley, Samuel Villarreal, Fabian Greffrath, Jonathan Dowland, Alexey Khokholov and Turo Lamminen | GPL-2.0-or-later |
| SDLPoP | `3c5add5fb7f83d4ceb542823ab66d00146c4271b` | Dávid Nagy (NagyD), from Jordan Mechner's Prince of Persia | GPL-3.0-or-later |
| OpenJazz | `a1626f4edd4a7af72c54103021790c56d8ecfced` | AJ Thomson (original author), Carsten Teibes (maintainer) and its other authors | GPL-2.0-or-later |
| NXEngine-evo | `1f093d1423cc395eb199230cd609b806ef1daa36` | The NXEngine-evo contributors, from NXEngine by Caitlin Shaw | GPL-3.0 |
| AROS (m68k toolchain and SDK) | `c8860e674d7c5fbab304e2bf201f7a9c81c98629` (`toolchain/aros-local/lock.env`) | The AROS Development Team | AROS Public License 1.1 |

## Used at build time, not included

- **SDL2 2.32.10** and **SDL2_mixer**, linked from the AROS SDK: Sam Lantinga and the SDL contributors, Zlib.

## Test data, not committed

`scripts/stage_game_ports_for_test.py` stages these on a local test machine only; none is in this repository or in release media.

- **Freedoom 0.13.0 Phase 1**: the Freedoom project, BSD licence (its `COPYING.txt`).
- **Jazz Jackrabbit 1.1 shareware**: Epic MegaGames.
- **Cave Story** freeware data: Daisuke "Pixel" Amaya (Studio Pixel).
- **Prince of Persia** two-level demo: Jordan Mechner's game, used only as a reference fixture.
- **The Ur-Quan Masters 0.8.0 content**: Toys for Bob, Inc. and the UQM team, CC BY-NC-SA 2.5.

Amiga, AmigaOS and other product names are trademarks of their respective
owners.
