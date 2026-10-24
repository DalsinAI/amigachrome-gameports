# SDL3 game and application port review - 24 October 2026

Primary target: **AmigaChrome AC090 / AmigaOS 3.x / 68040 + FPU**

This review widens the SDL3 intake beyond the first active ports. Inclusion here
is a porting candidate or active engineering lane, not a build or release claim.
Exact source pins move into `gameports/catalog.json` only after source, licence,
dependency and runtime-data review.

## Platform baseline

AmigaChrome now has an SDL3 lane through OpenAmiga SDL3 / SDL3.module, the
OpenGPU renderer, OpenAudio/AHI, OpenInput, GCC 16.2.0-open3 and the existing
Open-family SDK. The useful near-term candidates are therefore C/C++ projects
whose SDL3 dependency is a real portability boundary rather than projects whose
actual dependency is Vulkan, .NET, Rust, Zig or an x86-only recompilation stack.

## Active SDL3 ports

| Target | State | Priority | Why / next gate |
|---|---|---:|---|
| LBA2 Classic Community | ACTIVE | P0 | Existing open3/SDL3 build and AC090 smoke evidence. Finish visual/runtime qualification. |
| OpenLoco | ACTIVE | P0 | Existing open3/SDL3 build reaches startup on AC090. Finish S5/game-state endian barrier, then visible gameplay and audio. |

## High-value next SDL3 games

| Target | Upstream | Priority | Fit | First meaningful gate |
|---|---|---:|---|---|
| DevilutionX | https://github.com/diasurgical/DevilutionX | P1 | Upstream has an explicit SDL3 build path; strong 2D game value; original Diablo/Hellfire data external. | Menu -> new game -> town -> input -> audio -> save/load. |
| ScummVM | https://github.com/scummvm/scummvm | P1 | Current desktop build supports SDL3. One backend unlocks many engines. | Build a deliberately small engine set, launch one redistributable/free game, input/audio/save/exit. |
| xrick | https://github.com/senjinthedragon/xrick | P1 | C + SDL3, Rick Dangerous I/II, unusually good architectural and cultural fit for AC090. | Title -> first room -> controls -> audio -> clean exit. |
| Frozen Bubble SDL3 | https://github.com/dchau360/frozen-bubble-sdl3 | P1 | C++/SDL3 with image/mixer/ttf; 2D; bots and networking give useful subsystem coverage. | Local single-player first, then audio/input; network later. |
| Freeserf | https://github.com/freeserf/freeserf | P1 | Current master moved to SDL3 in October 2026; classic Settlers workload is a strong 68040 target. Original data external. | Main menu -> map -> scrolling/input -> audio -> save/load. |
| Street Rod SDL3 | https://github.com/kylofon/street-rod-sdl3 | P1 | C11 + SDL3, low rendering cost, original data external. | Title -> garage -> race -> save -> clean exit. |
| Street Rod 2 SDL3 | https://github.com/kylofon/street-rod-2-sdl3 | P1 | Same family; full game reimplementation gives a reusable second target. | Title -> garage -> race -> save -> clean exit. |
| Space Invaders SDL3 | https://github.com/uRadical/spaceinvaders | P1-LAB | Tiny native SDL3 game; useful release canary for renderer/audio/input rather than a flagship. | Deterministic playable loop with sound and clean exit. |
| Woof! | https://github.com/fabiangreffrath/woof | P2 | Modern Doom-family source port using SDL3. Duplicates some Chocolate Doom value but is an excellent SDL3 stress/canary. | Freedoom title -> map -> input -> audio -> save -> exit. |
| Project Picori / TMC PC port | https://github.com/999sian/tmc | P2 | Native C/SDL3 port built on the Minish Cap decompilation. Strong low-level fit; original game data boundary must stay explicit. | Boot/title with owner-supplied data, input/audio/save. |
| Principia | https://github.com/Bithack/principia | P2 | C++ SDL3 physics/sandbox with OpenGL and Lua; useful OpenGPU + CPU/physics test. | Menu -> simple level -> physics interaction -> audio/input/save. |
| OpenCMR2 | https://github.com/pablocpas/OpenCMR2 | P2 | Native SDL3 port of Colin McRae Rally 2.0 decompilation; excellent 3D/gamepad target, original data external. | Menu -> track -> drive one stage -> audio/input/exit. |
| SuperTux | https://github.com/SuperTux/supertux | P2 | Mature open-source platformer now carrying SDL3 support; larger C++/OpenGL footprint but no proprietary data barrier. | Menu -> level -> controller/audio/save. |
| Quetoo | https://github.com/jdolan/quetoo | P2 | C/id Tech 2 lineage and SDL3; useful OpenGPU FPS after AssaultCube. | Local map -> movement -> textured scene -> audio -> clean exit. |
| Richman 4 Remastered | https://github.com/lhr0909/richman4-remastered | P2 | C++/SDL3 board-game port; moderate rendering and good UI/input test. Owner data required. | Menu -> board -> input -> audio -> save. |
| Atrinik classic client | https://github.com/atrinik/classic | P2 | C17 SDL3 client/server/editor monorepo; client is a useful networked 2D workload. | Client window -> local/test server connect -> input/render/audio. |
| JA2 Pecel Bakwan | https://github.com/adhikasp/ja2-pecel-bakwan | P3 | SDL3 Jagged Alliance 2 reimplementation with larger modern dependency surface. | Dependency audit before any build work. |
| ChupathingyCE / OpenCE Halo CE | https://github.com/ChupathingyCE/chupathingyce | P3 | SDL3-backed Halo CE community build. High showcase value, very large engine/runtime scope. | Architecture/dependency audit only until P1/P2 queue clears. |
| OpenBumpy | https://github.com/wildpacman/OpenBumpy | P2 | C++20/SDL3 reconstruction of Bumpy's Arcade Fantasy; original data external. | Title -> first stage -> input/audio -> clean exit. |
| Dinosaur SDL3 | https://github.com/Jenway/dinosaurSDL3 | P1-LAB | Small C SDL3 game with native persistence and regression tests; ideal deterministic canary. | Play loop -> high-score persistence -> clean exit. |

## SDL3 applications, emulators and platform multipliers

| Target | Upstream | Priority | Fit | First meaningful gate |
|---|---|---:|---|---|
| ScummVM SDL3 | https://github.com/scummvm/scummvm | P1 | Application/platform multiplier: one SDL3 backend unlocks many adventure engines. | Minimal engine build with one free/redistributable title. |
| DOSBox-X SDL3 | https://github.com/CrownParkComputing/dosbox-x-sdl3 | P1 | Existing SDL3 fork; huge application/game multiplier. Interpreter-first on 68040, reduced feature set. | DOS prompt -> one tiny DOS game/app -> audio/input -> exit. |
| Retro-DOS | https://github.com/CrownParkComputing/Retro-Dosbox | P2 | SDL3 + Dear ImGui frontend over DOSBox-X. Useful later, but core DOSBox-X should land first. | Frontend/library with one local test title. |
| portty | https://github.com/chestso/portty | P2 | C terminal emulator using SDL3, FreeType and HarfBuzz; aligns with existing Open-family font work and provides a real non-game SDL3 app. | Shell PTY -> text render -> keyboard -> resize/scroll -> exit. |
| BubiC-8801MA | https://github.com/bubio/BubiC-8801MA | P2 | C++ PC-8801 emulator using SDL3/ImGui. Useful emulator workload if dependencies stay tractable. | Emulator UI -> legal/free boot media -> input/audio. |
| simple2d | https://github.com/simple2d/simple2d | P2-SDK | C SDL3 2D framework, more useful as an AmigaChrome developer/sample layer than an end-user app. | Build examples against SDL3.module/OpenGPU. |
| sdl3-gl | https://github.com/arkenidar/sdl3-gl | P1-LAB | Tiny C SDL3 + fixed-function OpenGL test; ideal OpenGPU regression harness. | Rotating OBJ scene through the supported GL/OpenGPU lane. |
| Ymir | https://github.com/ymir-emu/ymir | P3/PARK | SDL3 Sega Saturn emulator, but C++20 and Saturn emulation cost make AC090 performance doubtful. | Architecture/performance review only. |
| Atrinik editor | https://github.com/atrinik/editor | PARK | SDL3 but written in Rust; toolchain cost dominates the application. | Revisit only if Rust/m68k becomes a supported platform. |
| CrossDesk | https://github.com/kunkundi/crossdesk | PARK | SDL3 frontend but WebRTC/media/network dependency stack is much larger than the UI. | No port work until WebRTC stack is justified independently. |
| GSplus | https://github.com/digarok/gsplus | P2 | SDL3 Apple IIgs emulator and tools based on KEGS; GPL-3.0. Strong retro-computing fit. | Boot legal/free system media -> render/input/audio -> clean exit. |

## Research / stretch candidates

These use SDL3 but should not consume the current porting queue:

- **Generals / GeneralsX SDL3 ports** - attractive, but their current renderer path
  is tied to DXVK/Vulkan/MoltenVK-class infrastructure rather than our current
  OpenGPU lane.
- **OpenReliant (StarLancer)** - Zig plus SDL GPU/Vulkan; the Zig toolchain is a
  larger blocker than SDL3.
- **MDK SDL3** - C#/.NET runtime requirement makes SDL3 the easy part.
- **SFIII 3rd Strike Xbox recompilation** - present project targets x86-64 and
  depends on recompilation/runtime assumptions that do not directly translate
  to m68k.
- **Dreams to Reality recompilation** - interesting C-lift/SDL3 prior art, but
  generated-code and endian assumptions need research before any AC090 promise.
- **Ymir / modern high-end emulators** - technically SDL3, but CPU cost and
  modern C++ dependencies dominate.
- **OpenReliant and other SDL GPU API projects** - revisit after our SDL3 GPU API
  support is broader than the current renderer/OpenGL path.

## OpenAL readiness and intake state

All P1/P2 entries in this review are accepted into the live programme as INTAKE,
including the non-game applications and SDK/lab targets. P3/PARK entries remain
research. The live execution order is maintained in [PORT_BACKLOG.md](../PORT_BACKLOG.md).

The owner reports OpenAL nearing readiness. Prepare AstroMenace's dependency
lane now and qualify it as the first OpenAL consumer after the SDK/runtime
passes device/context, sample, streaming, teardown and repeat gates on AC090.
This is a conditional priority change, not an OpenAL availability or audio-pass
claim. SDL3 audio remains its own OpenAudio/AHI qualification lane; do not add
an OpenAL dependency to SDL3 targets unless their actual source requires it.
Keep LBA2/OpenLoco and the active playable finish campaign ahead of broad new
port implementation. Exact source pins, licences, dependencies and external
data must still be reviewed before a candidate enters the fetch/build catalogue.

## Programme order

1. Finish **LBA2** and **OpenLoco** first: they are already real SDL3/open3 ports.
2. Intake **DevilutionX, ScummVM, xrick, Frozen Bubble, Freeserf, Street Rod 1/2**.
3. Use **Space Invaders SDL3, Dinosaur SDL3** and **sdl3-gl** as small deterministic regression
   programs for SDL3.module/OpenGPU/OpenAudio/OpenInput.
4. Start the platform multipliers: **DOSBox-X SDL3**, then **Retro-DOS**.
5. Take the second wave: **Woof!, Project Picori, Principia, OpenCMR2, SuperTux,
   Quetoo, Richman 4, Atrinik client, OpenBumpy and GSplus**.
6. Keep modern-toolchain or renderer-heavy SDL3 projects in research/PARK until
   they stop requiring a larger platform port than the application itself.

## Acceptance rule

A target becomes **PLAYABLE** only after a real AC090 run reaches a useful
interactive state with graphics and input. It becomes **RELEASE** only after its
applicable audio, persistence, focus/window, clean-exit, provenance, packaging
and clean-instance repeat gates pass.

SDL3 use alone is not a reason to port a program. The SDL3 lane is valuable when
it reduces platform adaptation and exercises OpenGPU/OpenAudio/OpenInput in a
reusable way.
