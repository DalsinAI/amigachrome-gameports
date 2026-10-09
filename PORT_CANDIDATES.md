# Game Port Candidate Catalogue

Initial screening source: [bobeff/open-source-games](https://github.com/bobeff/open-source-games).

This is a **candidate list**, not a build claim. Projects move into `gameports/catalog.json`
only after an exact upstream revision, build strategy, dependency boundary and data/licensing
policy have been reviewed.

Primary target: **AmigaChrome AC090 / AmigaOS 3.x / 68040 + FPU**.

## Platform fit

Two porting lanes matter:

- **ACGame lane** — best for indexed/software-rendered titles. ACGame now provides the
  AmigaOS/AROS AGA+RTG backend, fast 8-bit C2P, OpenGfx-aware RTG presentation,
  keyboard/mouse, OpenInput pads with lowlevel.library fallback and timing.
  ACGame PCM audio is still a stub.
- **Open SDL/GPU lane** — SDL2/OpenGPU/OpenInput/OpenAudio/AHI remains the better fit for
  true-colour SDL, OpenGL and more modern engines.

The compiler qualification rule still applies: DONE means RELEASE, and all final binaries
must be rebuilt and runtime-qualified with the approved fixed GCC 16.2 stove.

## Already active or source-pinned

- AssaultCube
- Warzone 2100
- Doom 3
- OpenLara
- OpenTTD
- The Battle for Wesnoth
- The Dark Mod
- Hedgewars
- Neverball / Neverputt
- Chocolate Doom
- OpenJazz
- SDLPoP
- NXEngine-evo
- C-Dogs SDL
- The Ur-Quan Masters
- Serious Sam Classic

## Tier A — strong near-term candidates

These look like the best next places to spend porting time after the current revival wave.
They are generally older C/C++ engines, software/SDL-friendly, or already known to run on
resource-constrained systems.

| Candidate | Likely lane | Why it is attractive | Main caution |
|---|---|---|---|
| Anarch | ACGame | Tiny C software FPS; excellent 040/AGA stress case | Verify renderer/input assumptions |
| SDL Sopwith | ACGame / SDL | Small C/SDL title; very low integration risk | Audio path |
| VVVVVV | SDL/OpenGPU | Mature SDL code, modest 2D workload | Data/licence packaging |
| Julius | ACGame / SDL | Caesar III reimplementation; software 2D and mature C | Original game data external |
| Wolf4SDL | ACGame / SDL | Classic C/SDL software renderer | Original game data external |
| Omnispeak | ACGame / SDL | Commander Keen reimplementation, light software workload | Original data external |
| Meritous | ACGame / SDL | Small SDL action/roguelike | Verify current upstream health |
| Brogue CE | ACGame / SDL | Small C roguelike with graphical frontend | UI/input adaptation |
| fheroes2 | SDL/OpenGPU | Mature HoMM II recreation with many low-end ports | C++ size and original data |
| Fallout Community Edition | SDL/OpenGPU | Reimplementation with constrained 2D workload | Original Fallout data required |
| Fallout 2 Community Edition | SDL/OpenGPU | Same advantages as Fallout CE | Original Fallout 2 data required |
| Exult | SDL/OpenGPU | Long-lived portable Ultima VII engine | Large C++ codebase/data external |
| OpenXcom | SDL/OpenGPU | Mature turn-based engine and natural 2D workload | C++/YAML dependency set |
| KeeperFX | SDL/OpenGPU | Older engine architecture, strong game value | Data, low-level assumptions |
| Dune II The Maker | ACGame / SDL | Old-school RTS workload, modest rendering | Upstream age/portability review |
| Freeciv | SDL/OpenGPU | Mature C code and useful networking stress | Choose a lightweight client frontend |
| Open Golf | SDL/OpenGPU | Small C 3D title, useful early OpenGPU test | GL/API requirements |
| JFDuke3D | SDL/OpenGPU | Mature Build-engine port and useful 040 stress | Commercial data external |
| JFShadowWarrior | SDL/OpenGPU | Same Build-engine advantages | Commercial data external |

## Tier B — good candidates, larger integration job

| Candidate | Why | Main caution |
|---|---|---|
| CorsixTH | Theme Hospital engine, mature SDL-era architecture | Lua + larger C++ dependency surface |
| OpenLoco | Excellent management title and 2D workload | Modern C++ requirements |
| OpenRCT2 | Very desirable and heavily portable | Large/modern codebase and memory footprint |
| DevilutionX | Mature, highly portable Diablo engine | Modern C++ and original data |
| ScummVM | Massive value and mature portability | Huge multi-engine codebase; scope carefully |
| Naev | 2D space game with SDL/OpenGL heritage | Lua/GL/dependency footprint |
| Stratagus / Wargus / War1gus | Mature RTS engine family | Lua + data/import workflow |
| Doom64-RE | Interesting bridge between classic Doom and later 3D | Renderer and asset/data requirements |
| CatacombGL | Small classic FPS with modern renderer | OpenGL adaptation |
| AvP Forever / NakedAVP | Great late-90s 3D target | More substantial renderer/audio work |
| Lugaru | Open-source 3D action game | OpenGL/C++ workload |
| wipEout rewrite | Excellent OpenGPU showcase | 3D renderer and original game data boundary |
| C&C Tiberian Dawn / Red Alert source | Historically appropriate large C++ targets | Windows-era platform assumptions |
| OpenEnroth | Might & Magic engine recreation | Modern C++ and large data/runtime scope |
| OpenNox | Attractive classic-engine target | Go toolchain makes native 68k difficult; investigate alternatives first |

## Tier C — OpenGPU / system stretch targets

These are worth retaining as long-range capability tests, not first-wave ports.

- Endless Sky
- Pioneer
- OpenMW
- OpenAge
- Widelands
- Permafrost Engine
- RVGL
- TORCS
- Trigger Rally
- SuperTuxKart
- VDrift
- Red Eclipse
- Xonotic
- CroftEngine
- Tomb Engine
- Rigs of Rods
- Speed Dreams
- 0 A.D.

## Low priority for native 68k

Projects whose primary implementation depends on a runtime/toolchain that is a larger port
than the game itself should not consume first-wave effort. Examples from the source list
include:

- Unity-based projects such as Daggerfall Unity;
- Godot 4 projects;
- Bevy/Rust projects;
- Java/Kotlin projects such as FreeCol, Mindustry and Unciv;
- .NET-heavy projects such as OpenRA and some modern reimplementations;
- Panda3D-based projects;
- browser/TypeScript-only games.

They can be revisited if AmigaChrome gains the relevant runtime, but today they are poor
native 68k candidates.

## Suggested next intake after the current wave

When the fixed GCC stove is approved and the existing ports have been rebuilt, the most
useful next source-intake batch is:

1. Anarch
2. SDL Sopwith
3. Julius
4. VVVVVV
5. Wolf4SDL
6. Omnispeak
7. fheroes2
8. Fallout Community Edition
9. OpenXcom
10. JFDuke3D

That batch deliberately mixes very small ACGame/software-rendering wins with larger SDL
titles, giving us useful coverage without immediately committing to another Doom 3-sized
project.
