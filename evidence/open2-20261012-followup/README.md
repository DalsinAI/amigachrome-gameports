# Open2 Follow-Up: Real Guest First Light and Narrow Port Fixes

**12 October 2026.** Continues [the first Open2 batch](../open2-20261012/README.md). Target: AmigaOS 3.x / AC090 / 68040 + FPU. **No port is promoted to RELEASE.**

## Results at the end of this pass

| Port or component | Evidence obtained | Remaining limits |
|---|---|---|
| SDLPoP | Intro and animated title rendered; actual keyboard input advances intro; level 1 rendered using upstream `megahit 1`; pause/menu state changes observed; two guest returns with exit 0 | Movement attempt occurred while paused, so movement is NOT verified. Audio, save/reload, gamepad, fullscreen and a fresh-install repeat remain open. |
| Neverball / AmigaNeverBall | Rebuilt with Open2; package verifier PASS; title, name UI, level sets and Level 01 3D start screen render; mouse/menu input works; native window close returns guest exit 0 | Native typed player name remains empty on the copied SDL2 runtime. On-screen keyboard allowed continuation. Ball control, level completion, audio, persistence and clean-instance repeat remain untested. |
| Neverputt | Rebuilt with Open2 in the same verified package | No runtime attempt in this pass. |
| AssaultCube client and server | Two targeted source fixes; rebuilt HUNK executables, no previous duplicate stream-section diagnostics; allocation-mismatch warning resolved | Other compiler warnings remain. No game/server runtime test; client audio is still the first-light stub. |
| NXEngine-evo | Previous long Open2 build completed: exit 0, 29,649,500-byte HUNK executable | 4,410 duplicate-section warning lines; no runtime test or content dataset in this lab. Not release-ready. |
| Stream-method diagnostic | Exact added methods exercised by 21 checks in a real AmigaOS guest, exit 0 | This proves the focused diagnostic, not AssaultCube gameplay. |

Exact artifact hashes, log hashes, runtime module identities and gate states are in [results.json](results.json). No engine binaries, game-data archives, ROMs or copies of AmigaOS are committed here.

## The AssaultCube fixes

### One Amiga definition for three stream methods

The HUNK linker warned that `stream::getchar`, `putchar` and `putstring` were emitted with different size/content in different object files. Inspecting the target disassembly showed translation-unit-specific devirtualisation: one copy used a normal virtual call, another added a speculative check against the default `read` implementation. The linker warning is not, by itself, proof that the generated methods are semantically wrong.

[Patch 0003](../../ports/assaultcube/patches/0003-amiga-stream-single-definition.patch) gives these three methods a single out-of-line definition on AmigaOS only. Non-Amiga builds retain their original inline definitions. Existing optimisation flags are unchanged and no diagnostics are suppressed. The original binaries and disassembly were retained; [before disassembly](stream-getchar-before.txt) records the differing target code.

Both build entry points apply or reverse-check the patch and reject unexpected source drift. Full rebuilt logs after this change are `assaultcube-*-single-definition-build.txt`; final logs include the subsequent small allocation fix.

### Match array allocation with array deletion

GCC identified a genuine allocation mismatch in `gmode_parse`: `newstring` allocates with `new char[]`, but the buffer was freed with scalar `delete`. [Patch 0004](../../ports/assaultcube/patches/0004-match-mode-buffer-array-delete.patch) changes exactly that statement to `delete[] buf` without altering protocol parsing or gameplay.

The final server and client links exit 0, with zero occurrences of the previous duplicate-section warnings and no `mismatched-new-delete` warning. This does **not** mean every warning in the project has been eliminated: earlier full compile logs retain volatile deprecation, potentially uninitialised data, overlapping-copy and other diagnostics requiring review.

### Tests

[Sixteen Python regression tests passed](regression-tests.txt), including the existing Open2 installation/selection guards plus four focused patch checks. The host behavioural test compiles the exact method bodies extracted from patch 0003, not a separately rewritten equivalent.

The same [diagnostic source](stream-contract.cpp) was compiled with the actual Open2 m68k compiler and executed in the isolated AmigaOS guest. [Guest output](stream-contract-guest.txt): `STREAM_CONTRACT_PASS 21 checks`; [guest exit](stream-contract-guest-exit.txt): `OPEN2_STREAM_CHECK_EXIT 0`. Checks cover read/write success and failure, EOF, unsigned byte values, truncation, empty strings and virtual dispatch.

## The actual runtime environment

The canonical `lab_instance.py` tool copied an existing **stopped** development lab into a new private workspace. No live instance was used for execution. All copied disk paths were independent, the copied source was not running, and a private guest command mailbox was installed in the copy.

This was **not a fresh AmigaOS installation** and does not meet the final clean-install repetition gate. The copied machine runs Kickstart 47.115 / Workbench 47.5, 512 MiB fast RAM and 8 MiB chip RAM. Its existing `opengpu.library` identifies as 0.9; OpenInput is 1.2. Exact hashes of the existing SDL2, GL and satellite modules are recorded in [lab-identity.json](lab-identity.json).

The application's compiler/runtime is Open2, but the OpenGPU SDK, runtime modules and codec archives are still reused dependencies, not newly rebuilt/qualified Open2 versions. Do not infer that the current upstream SDL text-input implementation was exercised simply because these games linked with the new compiler.

The lab was muted throughout. A configured audio device, a running mixer or an audio-capable executable does not establish an audio pass.

## SDLPoP observations

- [Animated intro after a real Space key](sdlpop-keyboard-continue-window.png).
- [Level 1 dungeon, character and torches](sdlpop-level-one-window.png), reached via upstream's documented `megahit 1` direct-level diagnostic mode, not a verified normal menu journey.
- [Paused menu](sdlpop-before-move-window.png) and [resumed level after Escape](sdlpop-settings-menu-window.png). The filenames are event labels, not assertions: the attempted right-arrow movement was made while paused, so it cannot be called a movement pass.
- [First exit](sdlpop-intro-exit.txt) and [second exit](sdlpop-level-exit.txt) both show guest return code 0. Ctrl+Q was sent as actual Amiga raw key input. The guest remained usable afterwards and launched the next program.

The full frames remain in the local evidence workspace. The committed window crops are exact crops of those captured guest frames, not generated artwork.

## Neverball observations and a concrete text-input gap

The exact cached upstream PK3 matches its pinned SHA-256. Open2 rebuilt both Neverball and Neverputt. [Package verification](neverball-package-verification.json) checks 27 files / 79,088,722 bytes and reports `packageGate: PASS` but explicitly `release: false`.

[The actual title screen](neverball-first-window.png) rendered in the lab. Return advanced to the player-name UI. Raw Shift+D/A/L/E input left [the name field empty](neverball-typed-name-window.png). That is a failed typed-name check on this recorded runtime, not proof that a newer OpenGPU implementation still has the same defect.

The game's own on-screen keyboard produced [the visible name `DAe`](neverball-onscreen-name-window.png), rather than the intended `DALE`; that path allowed [level-set selection](neverball-after-name-window.png). It is explicitly a workaround for further graphics testing, **not** an SDL_TEXTINPUT pass or a precise name-entry test.

[Level 01](neverball-level-activate-window.png) then displayed the actual 3D board, coins, goal and Boing Ball beneath its start instructions. This is a rendered level-start scene, not proof of ball physics/input, a completed level or a frame-rate benchmark. [Native telemetry](neverball-renderer-telemetry.txt) records virgl submissions/presents and zero reported virgl errors during this run; it does not establish that every GL path or earlier black-frame issue is fixed.

Closing the native window produced [guest exit 0](neverball-guest-exit.txt). The private mailbox finished normally.

## Harness corrections, not hidden game failures

The initial frame reader mishandled an unchanged AGA frame and could read an unlit RTG buffer. It was corrected to request full timed ACF4 pictures and to ignore inactive displays. Only subsequent live RTG captures are used for reported game rendering.

An initial input helper call used native record kind 3 (mouse buttons) instead of kind 1 (keyboard). The mouse state was released; keyboard events were corrected against the runtime's actual input switch before reporting observed keyboard effects. This is not recorded as an SDL/game defect.

No guest-memory dumps or speculative GUI assertions are used as test proof.

## Lab cleanup and next steps

[The private lab was stopped](lab-stop.txt) using `lab_instance.py`; no processes remained and its four ports were released. The copy and raw logs remain available for resumption. Live instances and other engineers' lab ports were not stopped or changed.

**Next exact gate:** in this preserved isolated lab, qualify the current intended OpenGPU SDL2 runtime and its matching SDK against Open2, then repeat actual keyboard text entry in Neverball: type a name, edit it, accept it, exit and relaunch to verify persistence. Keep the previous module hashes and this failing baseline for comparison.

Then exercise SDLPoP movement while definitely unpaused, audio/save/reload/controller behaviour; Neverball ball control and audio; and the rebuilt AssaultCube client/server with authorised game data. NXEngine's duplicate-section warning volume needs its own compiler/linker assessment before release. Other ports' original game data remains external.

**Local workspace:** `$WORKSPACE/AmigaChrome-Ports-Open2-20261012/build/followup-20261012/`.

**Summary:** additional builds, two narrow code fixes, actual 2D/3D first light, a real target diagnostic pass and clean guest exits. No full game release or unmeasured performance claim.
