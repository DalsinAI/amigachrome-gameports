# Merged builds and next port checks - 13 October 2026

## Merged and rebuilt

Gameports PR29 merged as `883efd5bc85797bf347afba9450e68290414148d`.
SDL2 text-input PR67 merged as `905d9f938bfa0ac793e8698aea5822ba3b79e684`.
The latter was integrated with current graphics main and qualified before merge.

A clean SDL build from the exact merged revision produced shared SDL2.module,
link stubs, static SDL, both diagnostics and satellites. The target checks and
final build exit were 0. Premerge and postmerge binary hashes differ for some
artifacts, so earlier runtime results are NOT silently attributed to new bytes.
Both final shared/static text diagnostics were run again, and passed the exact
UTF-8 a/A/1/pound/e-acute/A-diaeresis/z sequence, Alt/Amiga identities, command
suppression and Start/Stop checks with exit 0. See final-main-text-* receipts.
Existing opengpu.library, GL and OpenInput remain reused dependencies.

Both OpenJazz and Chocolate Doom rebuilt from merged gameports sources using
the matching new SDL SDK. The OpenJazz directory fix below was then applied and
rebuilt. Exact hashes, logs and the final SDK comparison are retained.

## OpenJazz: actual configuration bug fixed

The first guest run rendered its menu and a diagnostic first level but printed
that its program directory was not writable, and failed to create openjazz.cfg.
A small target diagnostic showed `access(path,W_OK)` returned -1/errno26 while
`stat` reported mode40700 and an exclusive new file was successfully created
and deleted. The same failure occurred for the directory without a trailing
slash, PROGDIR: and ENVARC:.

Inspection of the release's libnix access.c showed that W_OK uses an exclusive
DOS lock. An in-use directory can therefore fail this test despite permitting
file creation. Patch 0002 makes the Amiga-only advisory directory check use
stat/S_ISDIR/S_IWUSR instead. The actual file-open still handles read-only media
and other write errors. Other platforms retain access(W_OK). Both portability
patches are applied or reverse-checked; unexpected source drift refuses.

The fixed build wrote a 487-byte configuration file; the next launch read it
without the earlier missing-configuration/write failure. Actual level and
rightward-motion frames plus exit-0 results are retained for that integration
runtime run. Two new host tests compile the exact helper and test writable,
read-only, missing and non-directory paths. Total local suite: **39 passed**.

**Important final-repeat limitation:** after the exact merged SDL binaries were
installed, a later OpenJazz start showed an initially blank window. A fresh
owned-lab restart and longer continuous run reached the difficulty screen, but
the complete final-repeat gameplay/normal exit was not established before the
lab was stopped. The initial short readiness window and focus state are not a
proven engine regression, but neither is this a final-main runtime pass. The
previous successful config/level checks remain tied to their original hashes.
Do not call OpenJazz RELEASE or erase this remaining startup/focus/exit gate.

## Chocolate Doom: software graphics, save/load and exit

The audio-enabled attempt did not reach a game frame during the observed sound
precaching stage. That attempt was stopped with the owned lab and does not have
a normal game-exit result. There is no audio success or permanent-hang claim.

A separate `-nosound` run with `SDL_OPENGPU=0` and official Freedoom0.13.0 data
reached E1M1 (`-warp 1 1` diagnostic entry), rendered the level and HUD, responded
to forward/turn input, saved slot DALE and loaded it after turning. The restored
view and 61,342-byte save receipt are recorded. Quit confirmation was followed
by an additional end-screen dismissal key; **DOOM_NOSOUND_EXIT 0** then appeared.
The earlier empty exit snapshot is retained, not relabelled as a completed exit.

This proves that bounded graphics/input/save/load/exit scenario with sound
explicitly disabled. It does not qualify audio, gamepads, all levels, fullscreen,
networking, physical 68040 performance or a pristine-OS repeat.

## Data and safety

The official Freedoom ZIP matches its pinned SHA-256. Jazz's shareware ZIP was
fetched through the existing approved runtime-data manifest and fingerprinted;
its archive has an observed hash, not a previously published pin. No game data,
ROM, NDK, save binary or executable is committed here.

All tests used copies of owned stopped labs, muted on reserved ports. The final
private lab was stopped and its ports released; see final-lab-stop.txt. Live
instances, other engineers' work and the public website were not changed.

## Next exact checks

Resolve the final OpenJazz startup/focus/exit repeat using an explicitly ready
and active window, then isolate Doom's audio-precache path without conflating
mute, disabled audio and successful audio playback. Continue full runtime gates
before release; the compiler, dependency and SDL text work is now on main.
