# OpenLoco

Target: AmigaOS 3.x / 68040 + FPU / AC090, GCC 16.2.0-open3 and OpenAmiga SDL3.
Status: initial port recipe and targeted probes; full-game configure BLOCKED.

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

The open3 full-game configure passed source/codec checks and stopped at the
required `openal` dependency. No full-game executable or compile pass is claimed.

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
  -DFMT_SOURCE="$PWD/build/game-ports/dependencies/fmt"
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
not implemented here, so the missing `openal` dependency remains explicit.

Use a locally generated versioned native asset cache where beneficial, per
[the preprocessing standard](../../docs/ASSET_PREPROCESSING.md). The full
schema audit and converter are not implemented. Object records, CSS/WAV
metadata, PCM and packed S5/save structures cannot be converted by blindly
swapping a container's words. Preserve original data and compatible saves.

Also outstanding: native Amiga paths/user directories, time and single-instance
handling, filesystem/packed-data audit, complete C++20 application build and
linker-warning investigation, OpenGPU presentation and INDEX8 qualification,
input, persistence, clean exit and measured memory/CPU costs.

OpenLoco's engine is MIT; preserve dependency notices. Original Locomotion
assets are user-supplied. The disabled OpenGraphics v0.1.12 package is pinned
upstream by SHA-256 `9e6596aaa52b2296cfc779bbe3aab01d0fb37ef61fcb23cd0e5ff4d2b9e846ca`;
it is not fetched or bundled here and still needs a separate packaging/rights
review. No assets, SDK headers or binaries are committed. All
[release gates](../../RELEASE_POLICY.md) remain open.
