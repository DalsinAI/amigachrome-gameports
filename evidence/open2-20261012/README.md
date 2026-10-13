# Open2 Porting Pass — 12 October 2026

**Work performed:** restart the existing game-port queue using OpenAmigaGCC **16.2.0-open2**, not the old GCC16 stove. Target is AmigaOS 3.x / AC090 / 68040 + FPU / libnix.

**Status:** five new compile/link artifacts; no new AmigaChrome game runtime pass. NXEngine-evo is running at this evidence snapshot; it is not marked successful. Exact times, source pins, hashes, compiler identity and dependencies are in [results.json](results.json).

## Completed builds

| Executable | Result from runner | Artifact SHA-256 | Runtime |
|---|---|---|---|
| OpenJazz | compile + final link exit 0; Amiga HUNK executable | `4e228015fd63616001ddf676769ec9ceb354a695ae662c96c41e68c305cad05e` | Not tested in this session |
| Chocolate Doom | compile + final link exit 0; Amiga HUNK executable | `570e47ba3def6f9c68849d7dfb30ac9a40f1d6fb77b753033180e4c9529d0b86` | Not tested in this session |
| SDLPoP / prince | compile + final link exit 0; Amiga HUNK executable | `dbba5fe804951eb5620bc4a77e16d0ac9b0e9f1ffe6fb8127d95915f151626c8` | Not tested in this session |
| AssaultCube server | compile + final link exit 0; Amiga HUNK executable | `72af11244b5ba98c39e77612c2792bb6f1724cb0813912633a358c67bf3adcee` | Not tested; linker warnings below |
| AssaultCube client | compile + final link exit 0; Amiga HUNK executable | `e64205a515df77c2d6f5f471b7a3f4a2b14de7898e865478fdaa33b7ef9d3158` | Not tested; existing audio stub and linker warnings |
| NXEngine-evo | build running at snapshot; source from exact catalogue pin | No final artifact hash claimed | Not tested |

Raw completed build logs are alongside this file as `*-build.txt`. HUNK magic was checked as `000003f3`. Binary files and game data are **not committed**.

## Compiler and build-system work

1. Verified the cached published Open2 binary archive (`5ece099a6fb8923a35eacf420bfa5fced06546ac1822714c76a2ae52466c978c`) and source archive before use.
2. Built an isolated stove; installed NDK headers generated for this release build. Did not replace the shared installed stove or alter any user instance.
3. Imported the existing OpenGPU SDL2/GL SDK and fingerprinted its inputs. Imported **only selected SDK archives/configuration and absent headers**, not old libc, libpthread, libgcc or libstdc++.
4. Fixed a genuine selection bug: existing scripts pass `STOVE`, while the shared CMake file ignored it and could choose a different compiler from HOME. CMake now respects `STOVE`, propagates explicit selection into probes, reports the chosen stove and rejects contradictory environment selections.
5. Verified actual generated CMake compiler paths for OpenJazz and NXEngine point into this Open2 stove.
6. Added a reusable [safe staging script](../../scripts/stage_open2_stove.py) that checks the release digest, refuses existing destinations, creates the installation transactionally, and proves the SDK overlay did not change compiler/runtime files.
7. Ran that staging script against the real release; **2,012 compiler/runtime files** in the build environment match the archive-backed reference. See [runtime match](runtime-archive-match.txt).
8. Added **12 passing fixture/selection tests**; [retained output](toolchain-tests.txt). CI exercises these tests, not full AmigaOS game runtime.

The C++23 `std::vector`/NDK smoke program also linked to an Amiga HUNK executable locally. That is a compile/link probe, not a runtime result.

## Compiler checks actually run

- [GCC reproducers](compiler-proof.txt): 34 successful output checks from the exact Open2 source release's `tests/repro/prove.sh`, covering shift/memmove/copy-backward, nested A6 library calls, FPCR, 64-bit stack arguments, softfloat, calloc, self-memory loops and null reads. The script combines focused qemu-m68k execution and generated-code checks; it is not a game test.
- [C++ EH checks](eh-check.txt): base-relative, resident32 and baserel32 throw/catch source links; 14 base-relative libstdc++/libsupc++ archives checked for the forbidden PC-relative typeinfo sections. This is static/link validation, **not** runtime exceptions or threading qualification.
- The EH check was initially called with a compiler executable path instead of its required prefix directory. That invocation failed to locate the compiler; its output is retained as [invocation error](eh-check-initial-invocation-error.txt). Corrected-prefix execution passed. Do not report the initial harness mistake as a compiler regression.

## Important outstanding risks

**Reused dependencies:** the existing OpenGPU client/satellite archives and the existing zlib/PNG/JPEG codec archives were not rebuilt with Open2 here. Their exact hashes are recorded. This is a clean application rebuild, **not** an assertion that the entire dependency graph has been rebuilt or qualified. The existing SDK's GCC16 loop-distribution workaround remains unchanged.

**AssaultCube:** final link reports duplicate COMDAT stream sections with differing size/content. Both executables were produced, but these warnings must be investigated and runtime-tested, not suppressed or described as a warning-free build. The client still links the deliberate first-light audio stub; it has no new audio integration or audio test result.

**NXEngine-evo:** a longer C++ build remains in progress at snapshot. Compiler warnings include ignored visibility attributes. The process records its final exit code and raw log under the runner workspace when it completes. There is no final link or usable game claim yet.

**Existing recipe feature limits:** OpenJazz is built with networking and its optional scale feature disabled. Chocolate Doom is built without SDL2_net; configure also reports optional SampleRate, zlib and PNG discovery missing. These are not compile failures, but release feature coverage must remain explicit.

**Runtime and data:** no clean-instance graphics, input, audio, save/configuration, focus, networking or clean-exit test was performed in this pass. Original game assets remain external. No port has been promoted to RELEASE.

## Exact next steps

1. Complete NXEngine's current Open2 link and record the result, rather than reusing its old binary.
2. Inspect AssaultCube's duplicate-section warnings; keep the result provisional until resolved or demonstrated harmless through a justified test.
3. Rebuild/qualify the relevant OpenGPU SDK/satellite dependencies with Open2, including the current SDL text-input backend, before calling the entire stack a fixed-toolchain build.
4. In an isolated AC090/AmigaOS 3.x instance, run OpenJazz and SDLPoP first; then Chocolate Doom with an authorised IWAD; then AssaultCube's offline bot map/server. Retain logs, screenshots, exact SDK/runtime module hashes, controls/audio/save and clean-exit results. Do not disturb active user instances.
5. Continue Neverball/Neverputt and the existing C-Dogs/UQM lanes; larger source-only targets such as Serious Sam and Warzone stay separate until their real build/runtime gates are reached.

## Local runner artifacts

Workspace on daletop:

`/run/media/da1ek/WorkingSpace/AmigaChrome-Ports-Open2-20261012`

- `build/open2-stove/`: compiler/SDK environment used for these builds.
- `build/open2-stove-checked/provenance.json`: repeatable staging reference and compiler/runtime fingerprints.
- `build/open2-results/`: full logs, exit records and local test payloads.
- `build/open2-results/test-payload/`: five engine-only artifacts with source pins and NOT RELEASE notices; no game assets.

Build instructions: [OPEN2_BUILDING.md](../../docs/OPEN2_BUILDING.md).
