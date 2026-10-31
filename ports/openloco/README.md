# OpenLoco

Target: AmigaOS 3.x / 68040 + FPU / AC090, GCC 16.2.0-open3 and OpenAmiga SDL3.
Status (31 October): playable on a 68040 lab copy of Instance-32 (OS 3.2.3,
OpenRTG, AHI). Title screen with its live demo map, scenario list and previews,
new game (Boulder Breakers), Save Game, autosave, Load Game from the title
screen and a clean exit all work with the original Locomotion data. Sound is
built in: OpenAL calls go to OpenRTG's `LIBS:OpenGPU/OpenAL.module` (OpenAL 1.1
over AHI); the lab reports "OpenAL ... Renderer: AHI Voices, initialized" with
992 mono and 32 stereo sources (labs are muted, so it has not been heard yet).
See [Lab results](#lab-results-31-october), [S5 boundaries](S5_ENDIAN.md) and
[audio](AUDIO_ASSESSMENT.md). Earlier evidence below is historical.

The exact upstream source is `OpenLoco/OpenLoco`
`6d7f5b1bfb4abd5ae2f1a2ade628a2b0d46f861e` (version 26.09).
`source.json` independently pins yaml-cpp 0.9.0, fmt 11.1.4 and sfl 2.2.0
by full commits. It does not change the catalog or existing port lanes.

## Offline recipe

```sh
python3 scripts/game_port_prepare.py prepare --catalog ports/openloco/source.json --game openloco
python3 scripts/port_dependencies.py stage --stove "$STOVE" --source /path/to/locked-codec-inputs
SDL3_SDK=/path/to/openrtg-sdl3-output \
  sh ports/openloco/build-amigaos3.sh --out "$PWD/build/openloco-clean"
```

`STOVE` is an isolated stove root produced by `scripts/stage_open2_stove.py
--release 16.2.0-open3`. Source preparation is offline by default; absent
sources need explicitly approved `--allow-network`. The build itself never
fetches. It verifies pristine pins, compiler/runtime hashes and the repository's
codec inventory, exports source and applies checked patches without editing
caches, and records logs and a receipt. Existing SDK/codec binaries are imported,
not newly source-rebuilt or runtime-qualified.

The platform baseline assessed here is OpenRTG
`0c98fe780e6531946be44bebbb3a9db0c7bb7ab2`, SDL 3.4.14 and SDL3_68k
`c82dea042718200eea401e7ed62d88b997bc9fd9`. New platform work is a separate
qualification dependency. The game links the platform SDL3 client.

## Changes and actual results

- Add truthful m68k/AmigaOS identification.
- Add an opt-in sequential viewport path, selected for this port. Desktop
  parallel rendering remains the default. No TBB substitution or invented
  OpenMulticore support; actual multicore rendering needs a later adapter.
- Disable runtime asset fetching/copying for this engine build. This is not a
  ready-to-run package. Upstream defaults remain unchanged.
- Read/write Sawyer container length and checksum fields explicitly as
  little-endian bytes. Payload conversion is a separate outstanding task.

The initial full-game configure stopped at OpenAL. The current engineering
recipe explicitly disables audio, supplies native Amiga platform/socket bindings
and builds successfully. Two clean builds produce SHA-256
`96da9f61b1c2845c268f20930fe2901503fa73803abc46d399fbe4dc701e5f72`. [Engineering notes](ENGINEERING.md) describe the
AC090 probe results, the scoped open3 compiler workaround and remaining gates.
Current full-application evidence is recorded separately from those API probes.

Targeted tests build the real patched Sawyer loader and MemoryStream with
synthetic data. The host probe passed exact length/checksum bytes, payload
round-trip, position restoration, corrupt checksums and truncated input. The
same probe and the Core Numerics/Prng sources compile/link with open3 to HUNK
and a target archive. The link emits duplicate libstdc++ section warnings.
The target probe has not been run; big-endian runtime correctness is unqualified.

After a game configure attempt has exported the patched source:

```sh
cmake -S ports/openloco/probes -B build/openloco-probes \
  -DOPENLOCO_SOURCE="$PWD/build/openloco-clean/source" \
  -DFMT_SOURCE="$PWD/build/game-ports/dependencies/fmt" \
  -DSFL_SOURCE="$PWD/build/game-ports/dependencies/sfl"
cmake --build build/openloco-probes --target port-probes -j2
ctest --test-dir build/openloco-probes --output-on-failure
```

Add the existing Amiga toolchain and `-DAMIGA_STOVE="$STOVE"` for cross-builds;
run that executable in an Amiga guest, not with host CTest.

## Remaining game work

The [audio reuse assessment](AUDIO_ASSESSMENT.md) recommends a reusable
OpenAL-compatible module over native OpenAHI/ACAHI. OpenAHI already supplies
mixing, voices, pitch and pan; ACAHI Host mix can offload the mixing. The common
layer needs resource arbitration, context/source lifetime, playback semantics,
spatialization and truthful capabilities. OpenLoco can keep its OpenAL calls.
A game-specific SDL3 audio replacement is not selected. The shared adapter is
not implemented here, and the engineering recipe explicitly disables it until that shared backend is ready.

Use a locally generated versioned native asset cache where beneficial, per
[the preprocessing standard](../../docs/ASSET_PREPROCESSING.md). The full
schema audit and converter are not implemented. Object records, CSS/WAV
metadata, PCM and packed S5/save structures cannot be converted by blindly
swapping a container's words. Preserve original data and compatible saves.

Implemented but still requiring full-application qualification: native Amiga
paths/user directories, time and single-instance handling. Still outstanding:
filesystem/packed-data audit, complete C++20 application qualification and
linker-warning investigation, OpenGPU presentation and INDEX8 qualification,
input, persistence, clean exit and measured memory/CPU costs.

OpenLoco's engine is MIT; preserve dependency notices. Original Locomotion
assets are user-supplied. The disabled OpenGraphics v0.1.12 package is pinned
upstream by SHA-256 `9e6596aaa52b2296cfc779bbe3aab01d0fb37ef61fcb23cd0e5ff4d2b9e846ca`;
it is not fetched or bundled here and still needs a separate packaging/rights
review. No assets, SDK headers or binaries are committed. All
[release gates](../../RELEASE_POLICY.md) remain open.

The final binary was launched from RAM: on AC090. Native DOS paths correctly
created program-relative User/save, landscape and objects folders. Version/help
print the pinned version and return 1, matching the upstream dispatcher. A full
startup reaches the retail assets but cannot interpret packed little-endian G1
metadata (4122 elements becomes 437256192); CSS loading also reports an invalid
vector size. No menu/gameplay pass is claimed. See
[evidence](evidence/noaudio-native-results.json).

## Lab results (31 October)

Built on Daletop through Cradle (`stove_build`, target `ports/openloco`,
stove `os32-gcc16`); `build-amiga.sh` keeps an incremental work folder,
builds OpenRTG's OpenAL.module and link stub at the commit in `source.json`
(`openrtg`), and installs into lab `t3-openloco` only (never an instance).

| Output | Bytes |
| --- | --- |
| OpenLoco (HUNK) | 9,835,448 (text 8,253,880) |
| OpenAL.module | 58,528 |

- New game, date advances, Save Game (763,801 byte .SV5 in under 0.5 s),
  autosave, Load Game of that save, quit from the window's close box: all OK.
- Patch 0017 lets the engine build against an OpenAL without EFX (OpenRTG's
  has none): reverb stays off because `ALC_EXT_EFX` is absent.
- Patch 0018 fixes the Save Game hang. The engine shows a busy pointer during
  a save and then puts the system pointer back. SDL3.modules built before
  OpenRTG 164dd9b (24 October; Instance-32's is one) link a system cursor
  into SDL's list twice, so `SDL_SetCursor` looped forever. On the Amiga the
  port now uses SDL's default cursor for the system pointers.
- Not yet checked: hearing the sound and music on an unmuted instance,
  building track and running trains for a long session.
