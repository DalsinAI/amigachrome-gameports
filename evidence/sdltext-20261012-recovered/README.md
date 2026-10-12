# Recovered autonomous SDL2 / game evidence

**Execution:** 12 October 2026, 20:39-21:39 UTC. **Recovered and inspected after reconnect:** 13 October local time. These are preserved results from the earlier authorised run, not newly executed tests. No full game release is asserted.

## What is now supported by the saved output

- Baseline SDLTextCheck emitted physical key events but **zero text events**. The baseline and candidate logs are retained separately.
- The candidate shared SDL2 module emits `a`, `A`, `1` and `z` as the expected one-byte UTF-8 text events. Ctrl/Amiga combinations and editing/navigation keys do not add printable text. F2 disabling/enabling text input suppresses/restores text events. The shared diagnostic returned exit 0.
- The static diagnostic emitted exactly the expected byte sequence `61 41 31 7A` and returned exit 0. See `text-static-checks.json`.
- A separate native Intuition focus window established **observed focus loss**, no text while unfocused, then restored delivery. See `text-focus-checks.json` and `text-focus-guest.txt`.
- Neverball accepted actual injected raw keyboard input `DALEX`; Backspace changed it to `DALE`; Return accepted it. This was not on-screen keyboard clicking. The input journal, actual guest frames and saved configuration agree.
- Both the first-exit and after-relaunch configurations contain `player DALE`. The first run and relaunch returned exit 0; the relaunched Play path reached level selection. This closes the narrow name-entry/edit/persistence gate on **this candidate runtime**, not all save or game functionality.
- SDLPoP's right-arrow test changed the character's horizontal position after an idle control frame showed it stationary. Actual before/control/after crops are retained. This is an observed rightward movement result on this lab, not complete controls/gamepad qualification.
- SDLPoP displayed QUICKSAVE and a 3,467-byte save file exists in its private guest directory. The last quickload capture is black: **successful restoration is not established**. A proper save/move/load/restore comparison remains required.

## Source, binary identity and limits

Source base: openamigartg `4d22f1bc2878bf9d7d17998e9cc15309d82de4da`, with the text-input patch from draft PR67 `787ac2f5ef78898174cf38e2b575a7c9d5504444` and the saved Makefile diagnostic targets.

The exact patch is `candidate-text-input.patch`, SHA-256 `6c9c48c5012b1cd3c89d0d6ee2476fdfa64185bee576659b28378f7e256845da`. `candidate-makefile.diff`, `candidate-diagnostic.c` and `candidate-helper-tests.py` preserve the actual local qualification changes. Restore these to their original openamigartg paths before running the helper tests; this evidence directory is not a separate SDL source tree.

Candidate SDL2.module: `b0594314367c47574fc5c701bb373b819eed2bad82aa563122601407c0a62400`.

Neverball executable tested: `ccff279192d9a63d762b56c98c8ba819dce5f6e152df392266a24d0f261721aa`.

SDL2 shared/static libraries and SDL image/mixer/ttf/net build artifacts were built with Open2. Only SDL2, image and mixer runtime replacements are asserted installed by `qualified-sdl-runtime.json`. The existing `opengpu.library`, `GL.module`, OpenInput and host were reused. This is not a complete OpenGPU/GL stack rebuild or a production deployment.

The lab's bsdsocket copy was still 4.2. Host deployment of 4.3 did not retroactively change the test baseline. The lab was muted: audio is **not passed**.

## Test problems retained, not erased

`text-candidate-checks.json` contains an initial failed unfocused-text check. The preceding click did not actually cause a focus-loss event; the test's prerequisite failed. The later second-window test explicitly observed loss/recovery and is the valid focus result. Both records remain available.

The physical Amiga modifier is logged as `Left Alt`. That pre-existing physical-key/scancode mapping discrepancy remains a qualification issue; successful text suppression alone does not validate all modifier mapping. International layouts, composed/dead-key text, held-key focus reset and every command combination remain unqualified here.

The quickload black frame is not renamed as a restore pass. No level completion, frame-rate benchmark, sound quality, controller or clean-install repeat is asserted.

## Captures and integrity

Neverball image files are exact `(0, 0, 695, 535)` crops of the saved guest frames; no generated graphics. SDLPoP crops are copied unchanged from the run. `results.json` records each evidence file's hash, original frame hash/crop where applicable, game/runtime identity and individual gate states.

The original baseline evidence directories remain historical and unmodified. The separate lab stopped at 21:39:11 UTC with no processes left and its ports released; `cutoff-lab-stop.txt` and exit code 0 are retained. No live user instance was used.

## Next gates

1. Keep PR67 draft until its own owner reviews the candidate, modifier/focus details and applicable AstroMenace profile test. Neverball evidence is not a substitute for AstroMenace's explicit acceptance criterion.
2. Verify SDLPoP and the other 2D ports on the software path with `SDL_OPENGPU=0`; OpenGPU acceleration is not a plain-68040 requirement.
3. Complete SDLPoP restore behaviour, Neverball controls/audio and gamepad/fullscreen/clean-install repeats.
4. Continue the separate OpenGPU-dependent 3D lane for AssaultCube; the dedicated server itself has no rendering requirement.
