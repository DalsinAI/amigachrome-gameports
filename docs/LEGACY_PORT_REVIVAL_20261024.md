# Older Amiga game-port revival review — 24 October 2026

The owner selected OpenTyrian and Enigma for active intake. The live order is
in [PORT_BACKLOG.md](../PORT_BACKLOG.md); this review preserves the evidence
and does not create a second queue. Fresh AC090 builds target 68040 + FPU,
AmigaOS 3.x, current open3 and the Open-family APIs.

## Existing-port references

| Target | Existing Amiga evidence | Why revisit / first decision |
|---|---|---|
| OpenTyrian | [Aminet README](https://se.aminet.net/game/actio/OpenTyrian.readme) | Current SDL2 source, native data/config paths and current input/audio qualification. |
| Enigma | [OS4Depot listing](https://os4depot.net/?function=showfile&file=game/puzzle/enigma.lha) | Modern source and normal included levels; Xerces-C and curl SDK integration are first. |
| FooBillard | [OS4Depot 3.0a](https://os4depot.net/?function=showfile&file=game/board/foobillard.lha) | Small fixed-function 3D target; compare renderer support before choosing the heavier successor. |
| LinCity-NG | [OS4Depot](https://eu.os4depot.net/?function=showfile&file=game/board/lincity-ng.lha) | City UI, SDL, filesystem and persistence exercise; audit current dependencies. |
| REminiscence | [MorphOS Aminet README](https://se.aminet.net/game/misc/REminiscence.readme) | Classic 2D engine; source rebuild and original-data importer required. |
| Cannonball | [OS4Depot](https://os4depot.net/?function=showfile&file=game/driving/cannonball.lha) | OutRun engine, useful renderer/input test; original ROM data stays external. |
| Stratagus/Wargus | [OS4Depot Stratagus](https://os4depot.net/?function=showfile&file=game/strategy/stratagus.lha) | Already in backlog; current engine/module and data-import review. |
| Exult | [Current RTG Aminet README](https://se.aminet.net/game/role/Exult_RTG.readme) | Maintained port: compare and integrate useful fixes rather than assuming abandonment. |
| Descent/DXX | [D1X-Rebirth RTG README](https://se.aminet.net/game/shoot/D1X_Rebirth_RTG.readme) | Maintained Amiga baseline; compare current features and runtime fit first. |

Source pins belong in the fetch catalogue only after audit. The unpinned
revival candidates above are accepted backlog work, not build-enabled ports.
Freeware, shareware, GPL engine code and retail game data have separate terms.
No original retail data is committed by this intake.
