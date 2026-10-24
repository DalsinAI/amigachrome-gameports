# AssaultCube source preparation and open3 rebuild, 24 October 2026

**Compile/link only. No guest runtime or release pass.**

Reviewed current GitHub gameports main `ed4dca0` and its merged #47 UQM/NXEngine
notes, the older restart/pause capsule, and current OpenUp, OpenRTG/OpenGPU,
OpenAmigaGCC, OpenTLS and OpenMulticore repository material before this work.
The old pause/status snapshots do not describe today's queue.

## Fixed

The client previously assumed the bootstrap and client patches had already
been applied manually. The server guessed bootstrap completeness from a single
`__amigaos__` text match. Both now use one offline preparer: check the catalog
HEAD, check/apply each target's complete ordered patch set, accept exact
already-applied patches, and reject mismatches with a filename. The client
also reports an absent ENet archive with the server-build prerequisite.

## Validation

Two independent clean local clones of upstream pin
`13f0d8eea4822dee5c976661218d022be74342e3` were made without copying dirty files,
objects or binaries. No network was used for upstream source preparation.
The first tree was client-prepared before the server/client builds; the second
exercised server preparation twice and client preparation twice, then built
server/client independently. Both outputs match byte for byte across trees.

The existing isolated `team1-open3` stove passed the repository's full
compiler/runtime inventory gate and locked zlib dependency gate. It was read,
not modified. The OpenGPU SDK and codec inputs are reused dependencies;
this is not a claim of fresh dependency builds or their runtime qualification.
All new outputs are in this chat's private checkout. No instance, installation,
ROM, game data or shared source checkout was modified.

| Artifact | Bytes | SHA-256 (both builds) |
|---|---:|---|
| ac_server | 765488 | `cd1705c0f1d76a35365cc60c21da69304a5a190c51a1a79f9b6745762042e3c4` |
| ac_client | 2608004 | `99f28548a43c3967865da23f61e68bba7aa41291d129f29dada73efb37382869` |

`results.json` and the four actual build logs retain identity, flags and output.
Builds use `-m68040 -m68881 -mcrt=nix20`; the client links the selected stove's
OpenGPU SDL2/GL stubs and the existing audio stub. Compiler and autotools warnings
remain in the logs; this is not a warning-free build.

68 host tests pass, including source pin refusal before mutation, fresh/repeated
preparation, partial preparation, patch drift and source-directory validation.
Shell syntax, Python compilation and working-tree whitespace checks pass.
These checks do not execute the new Amiga binaries.

## Outstanding queue

- AssaultCube: actual server listen/stop and client menu/bot-map/input/exit,
  audio integration, clean-instance qualification and release packaging.
- Neverball/Neverputt: existing first-light/name-persistence evidence remains
  bounded; full levels/hole, audio, controller/focus and clean-instance gates
  remain. Neither was rebuilt or run in this pass.
- UQM: retain today's merged native-path/ZIP and muted Sol/exit evidence;
  conversation, persistence, audio and intended-ROM repeat remain. The README
  table now reflects that evidence. UQM was not rebuilt/run here.
- Warzone: exact source pin inspected; SDL3 3.2.12 and dependency requirements
  recorded in its port note. No configure/build/runtime pass. Open-family
  adapters and qualified dependencies are prerequisites.

No port is promoted to RELEASE, and no runtime deployment is performed.
