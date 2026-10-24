# Civilization II, III, IV and Alpha Centauri: native port review

Reviewed 24 October 2026. Target: AmigaChrome AC090, AmigaOS 3.x,
68040 + FPU, GCC 16, the OpenGPU/OpenAudio/OpenInput/OpenMulticore/OpenTLS
interfaces and their native SDL adapters. No replacement Mesa/SDL platform is
proposed. This is source review and a limited compilation probe, not a playable
or release qualification for any of these games.

## Decision

Accept **Alpha Centauri / Alien Crossfire** into the programme at the owner's
explicit request; the owner has Alpha Centauri. Confirm whether that copy
includes Alien Crossfire before selecting the GLSMAC asset set. Keep two routes:
GLSMAC for a standalone native engine, and Thinker/original-executable
reconstruction for fidelity to the complete original game. They are different
projects, not interchangeable versions of a completed port.

**Civ II has the strongest newly discovered near-term route.** Richard Penman's
C/C++ reconstruction is materially more suitable than the C# Civ2-clone covered
in the initial chat answer. Its unchanged SDL host source compiled successfully
with our actual m68k compiler during this review. The whole game has not built.

**Civ IV has a real standalone engine lead**, Cv4MiniEngine, so the available
work goes beyond the original GameCore SDK. It offers a possible logic-first,
simple-display route, with substantial CPU/runtime/endian work.

**Civ III remains the weakest immediate native fit**: OpenCiv3's current build
uses Godot 4.4.1 and .NET 8. C3X is valuable reverse-engineering prior art but
still patches the original Windows executable.

## Exact source snapshots inspected

| Project | Revision | Relevant files |
|---|---|---|
| [richardpenman/civ2](https://github.com/richardpenman/civ2) | `e3c40da88689ed79903debf4bff436bd140b79fa` | STATUS.md; tools/build_native.py, build_port.py, fetch-toolchain.sh; src/host/{port.h,port_main.c,host.h,host_menu.h,host_sdl.c,wasi.c}; src/platform/win32_vk.h |
| [C7-Game/OpenCiv3](https://github.com/C7-Game/OpenCiv3) | `e2856ee6d2171b3fd1cd231c3bd6f9daba5b6f20` | README.md; C7/C7.csproj; C7Engine/C7Engine.csproj |
| [axx0/Civ2-clone](https://github.com/axx0/Civ2-clone) | `e1b2fbb0f14e9e33fbab9d5cc068313b52175495` | repository tree and current documented .NET/Raylib build route; supporting alternative, not the selected Civ II route |
| [fortsnek9348/Cv4MiniEngine](https://github.com/fortsnek9348/Cv4MiniEngine) | `51d73f519d4b26988e3d8dfc162351f0dd910127` | README.md, BUILDING.md, CMakeLists.txt, LICENSE.txt; CommonStuff SIMD headers; RawBinaryStream.h |
| [afwbkbc/glsmac](https://github.com/afwbkbc/glsmac) | `0fe9ef16ba7022569eaa43295dbe47f643683601` | README.md, CHANGELOG.md, CMakeLists.txt; OpenGL.cpp; Orthographic.cpp, OrthographicData.cpp, ShaderProgram.*; Thread.cpp, Engine.cpp; types/Buffer.cpp |
| [induktio/thinker](https://github.com/induktio/thinker) | `dabe62f2d1abba03de40f3dd3048494843f6a9af` | Technical.md; src/engine.cpp; documented launcher/terranx.exe boundary |
| [maxpetul/C3X](https://github.com/maxpetul/C3X) | `117bb1699376754738f3ed5e04f4c27b42845919` | project README and injection architecture; supporting prior art only |
| [WebAssembly/wabt](https://github.com/WebAssembly/wabt/tree/1.0.41) | upstream toolchain tag `1.0.41` | wasm-rt.h, wasm-rt-impl.*, wasm-rt-mem-impl.c and src/template/wasm2c.declarations.c |

Reconstruction accuracy, feature-completeness and performance statements from
upstream are upstream reports, not reproduced Amiga results. The Windows
OpenCiv2 fork's README describes a different replay count from the pinned
upstream STATUS.md; use the pinned upstream evidence rather than combining the
two numbers into a new claim.

## What sharing 32-bit space does and does not solve

The original PC programs' 32-bit data models are helpful: their designs do not
inherently demand a 64-bit address space. A 32-bit m68k compiler can preserve
many integer widths, subject to explicit structure layout and alignment checks.
This is a reason to investigate, not a reason to reject them as too modern.

It does not make x86 instructions executable on m68k, supply Windows APIs or
.NET, preserve little-endian files, or reproduce x86 SIMD/calling conventions.
The modern replacement engines can also have requirements quite different from
the original games. Available contiguous RAM, stack, framebuffers and OS address
reservations must be measured; a nominal 4 GB address space is not 4 GB of
available game memory. Keep the 68040/FPU build as the baseline; AC090-specific
optimisations can follow as optional paths.

## Civilization II: strongest native intake

[The pinned status](https://github.com/richardpenman/civ2/blob/e3c40da88689ed79903debf4bff436bd140b79fa/STATUS.md)
reports executable game functions reconstructed in C/C++, an SDL2-based
implementation of the Windows services used by the game, native Linux32 and
WebAssembly-to-C builds, and differential comparisons with the original.
Multiplayer and some audiovisual/UI paths remain outside that comparison.
This is a reconstructed engine, not an official source release.

Two engineering routes exist:

1. **Preferred first experiment: wasm32 -> wasm2c -> native m68k C.** Run the
   WASI compiler and wasm2c on the build PC; only the resulting C and small
   runtime execute on the Amiga. There is no browser or WebAssembly interpreter
   in this route. It preserves the guest's 32-bit pointers as offsets, reducing
   the risk from the reconstruction's original layout assumptions. The target
   performance cost of translated memory accesses is unmeasured.
2. **Later alternative: compile the reconstructed C++ directly for m68k.** This
   avoids translated linear-memory accesses but needs a wider audit of packed
   layouts, casts, unaligned accesses, little-endian data and pointer arithmetic.
   Matching pointer widths alone does not prove this route correct.

### Concrete probe completed

The unmodified pinned `src/host/host_sdl.c`, with its actual host/menu/virtual-key
headers, compiled with:

```text
OpenAmigaGCC 16.2.0-open3
-m68040 -m68881 -mcrt=nix20 -O2 -Wall -Werror=implicit-function-declaration
native stove SDL2 headers
```

Compilation returned 0 with no diagnostics. The output is recognised as an
AmigaOS object, SHA-256
`dbe4e288b7498fa48487aa799687bc669d63b7262b9b6c73b6568913c10987f5`.
This establishes header/API-level compilation only: no final link, sound,
display, original game execution or speed result is claimed.

### Definite changes needed

- **Host-memory boundary:** `src/host/port.h:17-20` assumes `data + offset` and
  uses raw memcpy for GET32/PUT32/PUT64. The pinned WABT template instead uses
  `data_end - offset - size` on big-endian hosts. HostEvent is also copied raw
  in port_main.c. A naive byte-swap-only patch is insufficient: strings, paths,
  event records, WASI buffers, framebuffers and cursor images all cross this
  boundary. Choose either a forward little-endian generated-memory backend
  with explicit scalar conversion, or a complete marshal layer matching
  WABT's reversed representation. Prove it with small generated-module tests
  before building the game.
- **Runtime allocation/traps:** WABT supports malloc/realloc and explicit
  bounds checks on 32-bit targets. Use and test those rather than assuming
  desktop mmap/guard-page behaviour. The game build requests **64 MiB initial
  linear memory and a 16 MiB module stack**; the latter is within linear
  memory, not an additional 16 MiB allocation. Target C stack and other
  allocations are additional and need measurements.
- **Pixels:** SDL ARGB8888 receives module bytes directly today. Convert the
  representation at the boundary before assuming colours or cursors are right.
- **Audio:** upstream explicitly requests AUDIO_S16LSB for 16-bit PCM. Preserve
  that declaration and test our native audio adapter's conversion; do not
  automatically swap twice because the host is big-endian.
- **WASI/files:** adapt POSIX calls and root-directory emulation to Amiga volume
  paths, file-size/time types and existing native C library support. Keep saves
  writable and outside original assets. Queue PCM through native SDL/OpenAudio.

First meaningful gate: generated-memory scalar/string/event/pixel round trips
on m68k, then an MGE title/menu, new game, found city, end turns, save/reload and
clean exit against a pinned original reference. Audio, advisors, diplomacy,
long-game turns and original replay comparisons follow. Do not claim full
compatibility from a title screen.

## Civilization III: two large routes

[OpenCiv3's game project](https://github.com/C7-Game/OpenCiv3/blob/e2856ee6d2171b3fd1cd231c3bd6f9daba5b6f20/C7/C7.csproj)
selects Godot.NET.Sdk 4.4.1 and net8.0. Its separate C7Engine project is also
net8.0, with MoonSharp, JSON, logging and other dependencies. Upstream calls
the game pre-alpha and says mechanics and late-game content are incomplete.

- Porting it unchanged entails a working m68k .NET runtime plus an appropriate
  Godot renderer/platform port. That is a system programme before it is a game
  port; a separate engine project does not remove its managed-runtime needs.
- A native C/C++ engine can reuse documented data formats and algorithms, with
  an OpenGPU 2D tile/sprite frontend. That is feasible as a planned rewrite,
  but it also inherits missing game features rather than gaining complete
  Civilization III merely by showing the assets.
- [C3X](https://github.com/maxpetul/C3X) supplies C patches and recovered
  structures/functions for Conquests. Its TCC injection still depends on the
  original executable; it is not a standalone engine we can relink for m68k.

Recommendation: retain as research, inventory data readers and game-rule
coverage first. A map viewer/new-game shell is a reasonable first milestone,
clearly labelled as such. Do not start by promising the whole Godot/.NET stack.

## Civilization IV: more possible than the SDK alone suggests

[Cv4MiniEngine](https://github.com/fortsnek9348/Cv4MiniEngine/tree/51d73f519d4b26988e3d8dfc162351f0dd910127)
reimplements engine services around the game core and presents a terminal UI.
Upstream reports single-player games, cities, diplomacy, victories and
save/load; multiplayer, music and several other features are missing. Its
documented save compatibility is asymmetric: reads vanilla BTS saves, but its
own saves lack a checksum required by vanilla. These are upstream reports.

This means an original-style 3D renderer is **not required to attempt Civ IV
gameplay**. A simple native tile/text frontend is a viable design direction.
It would not initially be the original game's visual presentation.

Concrete blockers in the current source:

- CMake requests **C++26** and unconditionally adds `-march=x86-64-v3` on the
  non-MSVC route. Removing that option alone is insufficient: CommonStuff
  actually compiles a SIMD A* source, Simd.h includes AVX implementations, and
  those contain x86 intrinsics. Disabling optional GameCore enhancements does
  not make all common code scalar.
- Supply a scalar pathfinder/vector implementation first; add optional
  OpenMulticore work only after deterministic single-core turns are proven.
- Python **2.7.18**, the Python bindings and Civ IV scripts are real runtime
  dependencies. Audit the native Python route and library integration.
- SFML **3.1** is used for audio/system services, with pugixml, zlib, S3TC,
  CLI11 and additional helpers. Replace the narrow audio boundary with native
  OpenAudio/SDL services rather than porting unrelated desktop audio stacks.
- `IRawBinaryStream` writes/reads trivially-copyable native bytes. Define
  little-endian, fixed-width serialisation for saves/assets, including the
  engine's Windows-versus-Unix character-size assumptions; audit call sites.
- The terminal frontend expects desktop console input/Unicode facilities. Our
  Shell support must be tested; an OpenGPU text/tile frontend may be the
  cleaner eventual user interface.

First gate: compile the scalar core and required Python boundary for m68k,
load a BTS fixture, advance a deterministic small-map turn and save/reload
without rendering. Add native UI/audio afterwards. The repository's own licence
note says the Firaxis GameCore code has separate terms; do not treat the
repository's GPL notice as relicensing the original game code/assets.

## Alpha Centauri / Alien Crossfire: accepted owner priority

### Route A: GLSMAC standalone native engine

[GLSMAC](https://github.com/afwbkbc/glsmac/tree/0fe9ef16ba7022569eaa43295dbe47f643683601)
is C++17 with SDL2, OpenGL, GLEW/GLU, FreeType, SDL2_image, uuid and yaml-cpp.
Its own scripting engine executes the .gls.js game/UI scripts; it is not a
dependency on a browser's JavaScript runtime. It reads original SMAC/SMACX
assets and does not need terranx.exe to execute game code.

The source has a current production-choice implementation, but the README's
roadmap still puts complete units/technology/victory/AI/diplomacy in later
stages. A successful port of this snapshot would not deliver the complete
original game. Inspect feature coverage in the code as work progresses; do
not infer completion from the screenshots or the dated changelog alone.

Concrete port work:

- Replace x86-specific Portable32/64 and -march=native CMake flags with the
  68040/FPU toolchain configuration. Build the resource embedder on the PC;
  CMake currently tries to run the built tool during the target build.
- Both orthographic shader paths use **GLSL 330**. Inventory their actual GL
  calls against OpenGPU, then implement a native render backend or a measured
  compatible subset. Do not assume OpenGPU supports them because a GL header
  compiled, and do not import a separate Mesa stack to hide the gap.
- `types/Buffer.cpp` serialises native integers/floats/vector structs using
  memcpy, including 64-bit integer values. Introduce a fixed byte order for
  save/network data and regression fixtures shared with the PC build.
- Engine.cpp creates main, network, garbage-collection and game thread roles;
  Thread.cpp uses std::thread. Audit synchronisation and native support before
  mapping work onto OpenMulticore. A single-core mode needs explicit ownership
  and scheduling; it cannot be achieved safely by deleting locks.
- Measure allocations early. The README recommends **2-4 GB RAM for its PC
  60 FPS target**, which is not a measured minimum for our reduced settings.
  Prove a tiny map, constrained texture cache and bounded working set first.

First gate: decode the owner's assets, display a small map/UI through OpenGPU,
exercise input, and save/reload the same map with endian fixtures. Separately
record which original gameplay systems remain absent. Audio follows through
native OpenAudio/SDL, not a requirement to wait for OpenAL.

### Route B: faithful original-engine reconstruction

[Thinker](https://github.com/induktio/thinker/tree/dabe62f2d1abba03de40f3dd3048494843f6a9af)
contains substantial C++ replacements for engine/AI functions, recovered data
structures and executable hooks. It is valuable prior art, but engine.cpp
still binds globals to fixed original-PC addresses, and the launcher patches
the original Alien Crossfire v2.0 executable in memory. It is not a portable
complete engine. Rebuilding the DLL for 68k does not supply its missing callees.

A faithful native project needs an inventory of every remaining original
function/import, replacing or statically translating x86 code and Windows
services, endian-safe data structures, then a deterministic turn/save harness
against the owner's original game. Preserve an original-game reference while
developing native code. Translation or PC emulation can be separate routes,
but neither is an already-tested native port.

Recommendation: begin with GLSMAC feasibility and the original-executable
dependency inventory, then choose based on working-set/rendering results and
the owner's preference for original fidelity versus an evolving remake. Keep
Alpha Centauri on the list even if GLSMAC fails its first memory/render gate.

## Work order and qualification

Do not displace the existing P0 LBA2/OpenLoco/gameplay work. Add Civ II as P1
source intake, Alpha Centauri as owner-priority P1 research with a large build
scope, Civ IV as P2 research and Civ III as P3 research. They become exact-pinned
build-catalogue entries only after source/data/dependency gates are recorded.

For each candidate: independent 68040+FPU build; current native Open libraries;
known ROM/runtime hashes; no old OS 3.2.3 disk dependencies; visible input,
turn progression, save/load, audio and clean exit; repeat from a clean profile.
The pending IEEE flags/power fix is a platform dependency to qualify when it
lands, not evidence that any game already passes floating-point tests.

No ROM, Instance-32 disk, Cradle configuration or deployed library was changed
by this review. No original game assets were published. Original data should be
imported from the required edition and stored outside git; an Archive.org item
title alone does not prove the edition, contents, integrity or redistributability.

## Archive.org test-data sources located

The owner requested Archive.org sources. Public catalogue and file metadata were
retrieved on 24 October 2026; the payloads below were **not downloaded or
extracted in this review**. These are candidates for edition/content verification,
not qualified test fixtures or claims of permission from an uploader.
Checksums are Archive.org's published SHA-1 values, not locally verified results.
Keep payloads, installed assets, fonts and any binary-derived data outside git.

| Intended test | Archive item | Selected original file | Bytes | Published SHA-1 |
|---|---|---|---:|---|
| Alpha Centauri + expansion candidate | [Planetary Pack backup](https://archive.org/details/acpp_20201226) | `acpp.iso` | 610926592 | `df0b2e9578cc0cff625c7618c947d4898cd43258` |
| Civilization II MGE | [MGE v1.0](https://archive.org/details/civ-2-mge-v-1.0) | `Civ2-MGE v1.0.bin` | 716172240 | `59d696b58713febe15e5550c68df285971d235c7` |
| Civilization II MGE disc tracks | same MGE item | `Civ2-MGE v1.0.cue` | 551 | `04afcf5053a7ee98e27beb1f935700411b93e1dc` |
| Civilization III + Conquests candidate | [Civilization III Complete](https://archive.org/details/civilization-iii-complete) | `Civ3Complete-BINCUE.7z` | 1817079017 | `12c107445dc14461b3b1d8183437fcde26c68682` |
| Civilization IV + BTS candidate | [Complete DVD Europe](https://archive.org/details/CIVCOMPLETEEU) | `CIVCOMPLETEEU.ISO` | 2544402432 | `2382e25098fa7147a11f8b444399711055597f60` |
| Small Alpha Centauri asset/parser experiment only | [Alpha Centauri demo](https://archive.org/details/SidMeiersAlphaCentauriDemo) | `smac_demo1_1m.exe` | 51401728 | `beb1146bbcb4f4ad4cb6492acf10b2d05d56a240` |
| Vanilla Civ IV reference only, not BTS | [Civilization IV demo](https://archive.org/details/SidMeiersCivilizationIvDemo) | `Civ4_Demo.zip` | 329530194 | `4fd521d55c312c357ed415e6b2e5ca49fa42e765` |

Import gates:

1. Fetch only the selected edition's files, verify the published digest, then
   record a local SHA-256 and extraction manifest. Extract data without running
   unknown installers where practical; an installed reference environment is
   separate from the native build.
2. For Civ II, retain the CUE and music tracks. Check the original executable
   version and required resource hashes against the pinned reconstruction's
   expected build. MGE v1.0 is an edition lead, not proof of matching executable
   revision. The wasm module build needs the original game installation.
3. For Alpha Centauri, verify the Planetary Pack really contains the expansion
   and required PCX/WAV/text resources. Thinker's executable route specifically
   expects Alien Crossfire v2.0; its documented GOG reference terranx.exe is
   3084288 bytes, SHA-1 `4b19c1fe3266b5ebc4305cd182ed6e864e3a1c4a`.
   GLSMAC's asset import has a different gate and does not execute that file.
4. Verify Conquests and BTS presence and patch revisions for Civ III/IV; vanilla
   demos are not replacements for expansion data. No protection bypass or
   patched executable is needed merely to inspect/import assets.
