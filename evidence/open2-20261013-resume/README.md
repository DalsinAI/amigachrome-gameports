# Resume after reconnect: review fixes and clean dependency builds

**13 October 2026 local time (12 October UTC).** This pass addresses the review of PR29 at `9ac1321`, recovers the earlier bounded runtime session, and continues qualification. Historical evidence is retained rather than rewritten.

## Review fixes implemented

### Explicit, portable dependency staging

The AssaultCube client/server, UQM, AstroMenace and NXEngine recipes no longer fall back to a workstation WebKit tree. They call `scripts/port_dependencies.py verify` before source edits or configure, and consume only `prefix/m68k-amigaos/port-deps` inside the selected stove.

`toolchain/port-dependencies.lock.json` records **113 exact archives/headers** covering zlib, PNG, JPEG and FreeType. `stage` requires an explicit local source directory, verifies every hash, copies only allow-listed codec files transactionally and writes a manifest. Builds reject missing inputs, changed hashes, unrecorded extra files, header/library symlinks and a manifest differing from the repository lock. Moving the stove away from its original donor is covered by tests.

An arbitrary external `ZLIB_ROOT` or `UQM_DEPS` is rejected. The existing SDK and these codec archives are **still reused dependencies**, not rebuilt with Open2 by this pass. The lock gives an exact byte inventory, not independently established original codec source/compiler provenance. That provenance remains a release requirement.

### Real committed-diff CI check

The workflow now fetches history and checks the PR base-to-head diff, or the push-before-to-HEAD diff, with a main comparison fallback for new branches/manual runs. It no longer checks an empty fresh-checkout working-tree diff and calls that PR whitespace validation. Raw evidence output remains exempt from whitespace rewriting so its hashes remain valid.

### NDK and extraction safety

The Open2 stager now protects **all release files**, including an NDK tree if a release contains one. Conflicting supplied NDK files are refused; identical files are retained. Supplemental NDK files receive their own hashes. A missing safe tar extraction filter gives an explicit Python compatibility error; Python 3.12+ is the recommended baseline.

The updated stager ran against the real release: **2,350 protected release files**, **1,374 fingerprinted supplied NDK files**. The real output is retained. Tests cover the previously omitted release-NDK collision.

### Rendering labels

The current status now separates the software-2D/plain-68040 qualification lane (OpenJazz, SDLPoP, Chocolate Doom) from the OpenGPU-dependent AssaultCube client. The dedicated server has no rendering requirement. A requirement is not automatically a passed physical-hardware or runtime test.

## Fresh AssaultCube rebuilds

Both targets were built from a **new clean checkout** of pin `13f0d8eea4822dee5c976661218d022be74342e3`, with the four existing reviewed Amiga patches and the newly staged dependency directory. Cached objects from previous builds were not used.

| Artifact | Result | SHA-256 |
|---|---|---|
| Server, 765,160 bytes | compile/final link exit 0; HUNK | `2b1880d8893c683c5c9e2c766a7d464ce241a5d32c526ad5696dae581b681a4b` |
| Client, 2,607,676 bytes | compile/final link exit 0; HUNK | `40b25ec3e16c912f5da702cafa768e9cc0a708a54831de2976d87a0c909dffa9` |

The retained logs contain **zero old WebKit path references**, **zero duplicate-section diagnostics** and **zero mismatched-new-delete diagnostics**. This confirms the targeted fixes survive removal of the old include tree. It is not a warning-free build: 13 server and 16 client warning lines remain (including configure/deprecation/other diagnostics). The client audio stub remains. Neither AssaultCube executable was run in a guest during this pass.

## Regression evidence

**35 Python tests passed locally**, including the previous compiler/AssaultCube patch contracts, new dependency gate tests and NDK preservation cases. Missing dependencies were exercised through all five real shell entry points; each refused before attempting a build. `regression-tests.txt` is actual runner output.

`results.json` records the new executable hashes, byte sizes, logs, dependency-lock hash and stager counts. No binary, game data or proprietary NDK headers are committed.

## Recovered earlier runtime results

[The recovered session](../sdltext-20261012-recovered/README.md) proves Neverball's name-entry/edit/persistence on the precise draft SDL2 candidate and an observed SDLPoP rightward movement sequence. It is explicitly labelled as the earlier run, not a new test at the recovery time. Its initial invalid focus precondition and unverified quickload black frame are retained.

## New SDLPoP software-rendering check

A fresh copy of the stopped owned lab was started through `lab_instance.py`,
muted and on its own port block. The guest script sets `SDL_OPENGPU=0`;
`GetEnv` returns `0` before launching the game. The copied candidate SDL2
runtime is the same hashed module described in the recovered evidence.

The new bounded sequence observed Level 1 rendering, then F6 quicksave,
right-arrow movement away from the saved position, and F9 quickload returning
the character to the original location after an eight-second wait. The save
file is 3,467 bytes; its hash/mtime and actual frames are retained. Ctrl+Q
returned **`CPU_SDLPOP_EXIT 0`** and the guest remained usable. The private lab
was stopped at 23:34:06 UTC, with no processes left and its ports released.

This resolves the *earlier short-wait black-frame uncertainty for this specific
SDLPoP save/load scenario*. It does not establish save compatibility across all
levels, physical 68040 hardware performance, gamepad/fullscreen, audio quality
or a clean-install repeat. Entry was the documented `megahit 1` diagnostic,
not a full normal-menu or complete-level playthrough. The RTG desktop remains;
`SDL_OPENGPU=0` is the SDL software-rendering mode, not removal of every host
or display-related graphics component. No network test was performed and the
private copied bsdsocket 4.2 baseline was not relabelled 4.3.

See `cpu-guest-exit.txt`, `cpu-guest-script.txt`, `cpu-save-receipt.json`,
`cpu-lab-identity.json`, `cpu-actions.jsonl` and the actual captured window crops.
