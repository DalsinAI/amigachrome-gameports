# Unified Game Port Backlog

Snapshot date: **9 October 2026**

Primary target: **AmigaChrome AC090 / AmigaOS 3.x / 68040 + FPU**

This is the single discussion backlog for game ports. It deliberately puts active ports,
source-pinned work, near-term candidates and stretch targets in one list so priority can be
decided across the whole field instead of inheriting the old Tier A / B / C grouping.

**Priority is intentionally TBD for every entry.** The next prioritisation pass should rank
the whole list together using: **P0 = finish now, P1 = next wave, P2 = planned, P3 = stretch, PARK = research / low-return for now**.

Status words are descriptive, not release claims. **DONE means RELEASE** and still requires
the normal clean-build, provenance, packaging and AC090 runtime gates.

| Port | Current state | Likely lane | Priority | Notes / next meaningful gate |
|---|---|---|---|---|
| OpenOMF / OMFAGA | ACTIVE - build fix | ACGame / native AGA | TBD | GCC16/0008 lane exists. Canonical AGA-profile state bug fixed; latest runner exposed a local ENet compatibility-header path error before compile. Then AC090 arena first light. |
| Neverball / Neverputt | BUILD GREEN - runtime testing | SDL / OpenGPU | TBD | GCC16/0008 build passes. Retest softpipe and accelerated virgl/OpenRTG path on AC090 and confirm black-frame issue is gone. |
| C-Dogs SDL | BUILD GREEN - runtime testing | SDL / OpenGPU | TBD | GCC16/0008 build passes. Strong compiler A/B canary because old stove hit divide-by-zero alert 0x80000005. |
| The Ur-Quan Masters | BUILD GREEN - runtime pending | SDL / OpenGPU | TBD | GCC16/0008 full build passes. User-supplied content required; menu, solar-system/combat, audio and clean-exit gates remain. |
| Chocolate Doom | BUILD GREEN - runtime pending | SDL / OpenGPU | TBD | GCC16/0008 clean build passes. Run with authorised IWAD/Freedoom and exercise input, audio, config/save and exit. |
| OpenJazz | BUILD GREEN - runtime pending | SDL / OpenGPU | TBD | GCC16/0008 clean build passes. Original game data remains external. |
| SDLPoP | BUILD GREEN - runtime pending | SDL / OpenGPU | TBD | GCC16/0008 clean build passes. Level-one runtime, animation, input, audio and clean exit next. |
| AssaultCube | BUILD GREEN - runtime pending | SDL / OpenGPU / OpenInput | TBD | GCC16/0008 client and server both build. Client final-link GL issue fixed. Bot-map/textured-scene first light next; audio integration follows. |
| NXEngine-evo | REBUILD DEFERRED | SDL / OpenGPU | TBD | Previous compile exists. Current clean fixed-stove rebuild was deferred by host/toolchain load rather than a known port defect. |
| Serious Sam Classic | SOURCE PINNED | SDL / OpenGPU | TBD | Start with The First Encounter. Major 3D/OpenGPU target. |
| Warzone 2100 | SOURCE PINNED | SDL / OpenGPU | TBD | Large 3D/application stress target; intended OpenUp edition candidate. |
| Doom 3 GPL source | SOURCE PINNED | OpenGPU | TBD | Long-range idTech renderer/engine stress target; original data external. |
| OpenLara | SOURCE PINNED | SDL / OpenGPU | TBD | Strong 3D engine candidate; original Tomb Raider data external. |
| OpenTTD | SOURCE PINNED | SDL / OpenGPU | TBD | Large C++ application covering graphics, audio, filesystem, networking and persistence. |
| The Battle for Wesnoth | SOURCE PINNED | SDL / OpenGPU | TBD | Large C++/SDL strategy-engine target with substantial data/localisation footprint. |
| The Dark Mod | SOURCE PINNED | OpenGPU | TBD | idTech 4-derived stretch target; engine source only, runtime content separate. |
| Hedgewars | SOURCE PINNED | SDL / OpenGPU | TBD | SDL/audio/networking/physics target with sizeable mixed-licence asset payload. |
| Anarch | CANDIDATE | ACGame | TBD | Tiny C software FPS; excellent native 040/AGA stress case. |
| SDL Sopwith | CANDIDATE | ACGame / SDL | TBD | Small C/SDL title and likely low-integration-cost win. |
| Julius | CANDIDATE | ACGame / SDL | TBD | Caesar III reimplementation; mature software 2D workload. Original game data external. |
| VVVVVV | CANDIDATE | SDL / OpenGPU | TBD | Mature SDL code and modest 2D workload. Review data packaging. |
| Wolf4SDL | CANDIDATE | ACGame / SDL | TBD | Classic software renderer. Original Wolfenstein data external. |
| Omnispeak | CANDIDATE | ACGame / SDL | TBD | Commander Keen reimplementation with a light software workload. |
| fheroes2 | CANDIDATE | SDL / OpenGPU | TBD | Mature Heroes II recreation with many low-end ports; original data external. |
| Fallout Community Edition | CANDIDATE | SDL / OpenGPU | TBD | Constrained 2D workload and strong game value. Original Fallout data required. |
| OpenXcom | CANDIDATE | SDL / OpenGPU | TBD | Mature turn-based 2D engine; dependency/YAML review required. |
| JFDuke3D | CANDIDATE | SDL / OpenGPU | TBD | Mature Build-engine port and useful 040 stress case; commercial data external. |
| Meritous | CANDIDATE | ACGame / SDL | TBD | Small SDL action/roguelike; verify current upstream health. |
| Brogue CE | CANDIDATE | ACGame / SDL | TBD | Small C roguelike with graphical frontend; UI/input adaptation needed. |
| Fallout 2 Community Edition | CANDIDATE | SDL / OpenGPU | TBD | Same general fit as Fallout CE; original Fallout 2 data required. |
| Exult | CANDIDATE | SDL / OpenGPU | TBD | Long-lived portable Ultima VII engine; larger C++ codebase and external data. |
| KeeperFX | CANDIDATE | SDL / OpenGPU | TBD | Older engine architecture and strong game value; review low-level assumptions/data. |
| Dune II The Maker | CANDIDATE | ACGame / SDL | TBD | Old-school RTS workload with modest rendering; review upstream age/portability. |
| Freeciv | CANDIDATE | SDL / OpenGPU | TBD | Mature C and useful networking stress; choose a lightweight client frontend. |
| Open Golf | CANDIDATE | SDL / OpenGPU | TBD | Small C 3D title; useful early GL/OpenGPU exercise. |
| JFShadowWarrior | CANDIDATE | SDL / OpenGPU | TBD | Build-engine sibling to JFDuke3D; commercial data external. |
| CorsixTH | CANDIDATE | SDL / OpenGPU | TBD | Theme Hospital engine with SDL-era architecture; Lua and larger C++ dependency surface. |
| OpenLoco | CANDIDATE | SDL / OpenGPU | TBD | Attractive management title and 2D workload; modern C++ requirements. |
| OpenRCT2 | CANDIDATE | SDL / OpenGPU | TBD | Highly desirable and portable, but large/modern with meaningful memory footprint. |
| DevilutionX | CANDIDATE | SDL / OpenGPU | TBD | Mature Diablo engine; modern C++ and original data required. |
| ScummVM | REVIVAL CANDIDATE | SDL / OpenGPU / OpenAudio / OpenInput | P1 | AROS patches, classic 68k and modern OS4/MorphOS ports exist. Start with a deliberately small engine set and strongest Amiga-family backend. |
| Naev | CANDIDATE | SDL / OpenGPU | TBD | 2D space game with SDL/OpenGL heritage; Lua/GL/dependency footprint. |
| Stratagus | CANDIDATE | SDL / OpenGPU | TBD | Mature RTS engine; Lua and data/import workflow are the main integration points. |
| Wargus | CANDIDATE | SDL / OpenGPU | TBD | Warcraft II game module/content workflow on Stratagus; original data boundary applies. |
| War1gus | CANDIDATE | SDL / OpenGPU | TBD | Warcraft I game module/content workflow on Stratagus; original data boundary applies. |
| Doom64-RE | CANDIDATE | SDL / OpenGPU | TBD | Useful bridge between classic Doom and later 3D rendering; asset/data review required. |
| CatacombGL | CANDIDATE | SDL / OpenGPU | TBD | Small classic FPS with modern renderer; GL adaptation is the main question. |
| AvP Forever | CANDIDATE | SDL / OpenGPU | TBD | Attractive late-90s 3D target with more substantial renderer/audio work. |
| NakedAVP | CANDIDATE | SDL / OpenGPU | TBD | Alternate AvP codebase; compare renderer/portability surface before choosing. |
| Lugaru | CANDIDATE | SDL / OpenGPU | TBD | Open-source 3D action game; OpenGL/C++ workload. |
| wipEout rewrite | CANDIDATE | SDL / OpenGPU | TBD | Potential OpenGPU showcase; renderer plus original-data boundary. |
| C&C Tiberian Dawn source | CANDIDATE | SDL / OpenGPU | TBD | Historically appropriate large C++ target; Windows-era assumptions need review. |
| C&C Red Alert source | CANDIDATE | SDL / OpenGPU | TBD | Same family as Tiberian Dawn; large C++/platform adaptation job. |
| OpenEnroth | CANDIDATE | SDL / OpenGPU | TBD | Might & Magic engine recreation; modern C++ and large runtime/data scope. |
| OpenNox | INVESTIGATE | Unknown / tooling | TBD | Attractive classic-engine target, but Go toolchain makes native m68k difficult; investigate alternatives first. |
| RetroArch | REVIVAL CANDIDATE - platform enabler | Open-family frontend / libretro | P1 | 68k AmigaOS RetroArch 1.20 already exists, with OS4/MorphOS relatives and core packs. Modernise the 68k backend onto OpenGPU/OpenAudio/OpenInput/OpenMulticore and prove one lightweight core first. |
| MAME | REVIVAL / SCOPING | OpenGPU / OpenAudio / OpenInput | P2 | AROS 0.36, classic 0.106-ish and MorphOS 0.148 lineages give useful prior art. Compare a period-appropriate MAME generation with libretro MAME cores rather than assuming current monolithic MAME. |
| PCSX-ReARMed (interpreter first) | CANDIDATE | RetroArch / libretro | P2 | Preferred PlayStation-specific experiment. Start with interpreter mode under RetroArch; assess endian/alignment, software GPU path and realistic AC090 performance. User-supplied BIOS/disc images remain external. |
| FinalBurn Neo | REVIVAL CANDIDATE | SDL / OpenGPU or libretro | P1 | Current MorphOS SDL2 prior art makes this a strong arcade target and potentially a better near-term fit than full modern MAME. Compare standalone and libretro routes. |
| DOSBox | REVIVAL CANDIDATE | SDL / OpenGPU / OpenAudio | P2 | AROS, classic Amiga and MorphOS precedent exists. Review interpreter versus dynamic-core/JIT options and native backend work. |
| Mednafen | PRIOR ART / CANDIDATE | SDL / OpenGPU / OpenAudio | P2 | AmigaOS 4 port includes PlayStation support. Mine backend and PS1 lessons; selected cores may be more useful than the whole frontend. |
| VICE | REVIVAL CANDIDATE | RetroArch / libretro or standalone | P2 | AROS/MorphOS precedent. Prefer core-first under RetroArch unless standalone integration buys something important. |
| mGBA / VBA-M | REVIVAL CANDIDATE | RetroArch / libretro | P2 | AROS VBA-M and OS4 mGBA precedent. Prefer a core-first route. |
| Basilisk II | REVIVAL CANDIDATE | Native 68k Macintosh emulator | P2 | Strong candidate: Basilisk II already has an AmigaOS 3.x port and can use the real 68k processor on AmigaOS rather than emulating the CPU. Modernise graphics/audio/input/network/filesystem integration for AC090. Mac ROM/System software remain user supplied. |
| Mini vMac | CANDIDATE | Native / OpenGPU / OpenAudio | P2 | Lightweight classic Macintosh route. Assess a 68k-host build and whether its simpler machine model makes a useful companion to Basilisk II. Macintosh ROM/System software remain external. |
| Hatari | REVIVAL CANDIDATE | SDL / OpenGPU / OpenAudio / OpenInput | P2 | Atari ST/STe/TT/Falcon emulator with Amiga-family port history. Use EmuTOS for a legal free first-light path, then optional user TOS ROMs for compatibility. |
| ARAnyM | CANDIDATE | SDL / OpenGPU / OpenAudio | P2 | Atari TT/Falcon-oriented emulator. Evaluate against Hatari for high-end Atari coverage rather than duplicating effort blindly. |
| E-UAE / UAE family | PRIOR ART / TOOL | Existing Amiga emulation lineage | PARK | Keep as architecture/tooling prior art only unless a specific subsystem lesson or compatibility use-case emerges. |
| Endless Sky | STRETCH | SDL / OpenGPU | TBD | Long-range capability target. |
| Pioneer | STRETCH | OpenGPU | TBD | Long-range 3D engine/capability target. |
| OpenMW | STRETCH | OpenGPU | TBD | Long-range capability target with very large engine/dependency surface. |
| OpenAge | STRETCH | SDL / OpenGPU | TBD | Long-range capability target. |
| Widelands | STRETCH | SDL / OpenGPU | TBD | Long-range strategy/application target. |
| Permafrost Engine | STRETCH | OpenGPU | TBD | Long-range engine capability target. |
| RVGL | STRETCH | OpenGPU | TBD | Long-range 3D/OpenGPU target. |
| TORCS | STRETCH | OpenGPU | TBD | Long-range racing/3D capability target. |
| Trigger Rally | STRETCH | OpenGPU | TBD | Long-range racing/3D capability target. |
| SuperTuxKart | STRETCH | OpenGPU | TBD | Long-range renderer, memory and CPU stress target. |
| VDrift | STRETCH | OpenGPU | TBD | Long-range racing/physics/3D target. |
| Red Eclipse | STRETCH | OpenGPU | TBD | Long-range FPS/OpenGL capability target. |
| Xonotic | STRETCH | OpenGPU | TBD | Very ambitious long-range FPS/renderer target. |
| CroftEngine | STRETCH | OpenGPU | TBD | Modern Tomb Raider engine rewrite; long-range capability target. |
| Tomb Engine | STRETCH | OpenGPU | TBD | Long-range Tomb Raider engine target. |
| Rigs of Rods | STRETCH | OpenGPU | TBD | Very large simulation/physics/renderer target. |
| Speed Dreams | STRETCH | OpenGPU | TBD | Large racing/physics/3D target. |
| 0 A.D. | STRETCH | OpenGPU | TBD | Very large RTS/engine/dependency stress target. |

## Priority discussion fields

When we rank this backlog, useful dimensions are:

- **User value** — how desirable the game is on AmigaChrome.
- **Probability of success** — likelihood of reaching first light without a major platform project.
- **Platform value** — whether it proves ACGame, OpenGPU, OpenAudio, OpenInput, networking or another Open-family layer.
- **Effort** — small / medium / large / very large.
- **Data friction** — whether redistributable data exists or the user must supply commercial assets.
- **Dependency friction** — compiler/runtime/library work required before the game itself can progress.
- **Showcase value** — usefulness for release demos and demonstrating AC090 capability.
- **Reuse value** — whether solving the port unlocks a family of related titles or engines.

The priority column should be filled only after comparing the whole list together.
