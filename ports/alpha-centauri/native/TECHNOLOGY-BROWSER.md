# Technology browser and discovery media

The browser consumes the existing qualified technology reader. It displays the
original technology names, both prerequisites, direct dependents, original
215×215 technology icon and original long description. Its level list is an
owned topological ordering, not the original game's research cost/ranking.
Disabled technologies are excluded; cycles, invalid references and dependencies
on disabled records fail without replacing the previous tree.

This is a read-only technology dependency/discovery-data browser. It does not
grant technologies, choose research, simulate discovery, invent unlock lists or
run research turns. The navigation/layout is new; exact original discovery-window
layout has not been restored. Original `NewTechWin_exec` builds
`voices/tech<ID>.mp3` from the actual decimal ID. Technology images are
`techs/tech<ID padded to three digits>.pcx`. Original content stays local.

`alpha_technology_browser(rules_path)` is the integration entry for both Alpha
profiles. The caller supplies its already-open Intuition/graphics/CyberGraphX
libraries and SDL audio lifetime. Release the unrelated menu audio device before entering (a paused allocated
menu device prevents another device from opening on the native backend), then
reopen the menu device retaining its previous cursor and enabled/muted preference. Restore the parent window's
focus after return. Both the frontend and original world now expose [T] through this entry; the
normal frontend/map build recipes link both browser source files. The
implementation remains separate from the renderer.

For standalone native qualification, run `build_technology_browser.sh OUTPUT`
with the normal SDK. `alpha-technology alpha.txt` or `alphax.txt` uses the
caller's working directory. It needs the original `ALPHC___.TTF`, `arialn.ttf`,
`techlongs.txt` and `techs/techNNN.pcx`. Optional quote playback uses locally
decoded `voices/tech<ID>.wav`, PCM16LE mono 22050. On big-endian machines samples
are swapped once before opening a native-endian SDL device. Playback is one-shot,
not looping, and stops on selection changes/return. The source MP3s remain the
original recordings; the browser does not include an MP3 decoder or distribute
recordings. Missing media is reported rather than substituted.

Controls: N/P next/previous technology; 1/2 prerequisite; D first direct
dependent; click a left-column row to select; V play its voice; Q return. The
browser lists every active technology. The detail pane shows up to seven direct
dependents; this bounded view does not change graph data.

## Qualification status

Host ASan/UBSan controls pass 17 synthetic graph/text cases, plus actual original
base 77 active / 136 edges (162 checks) and expansion 86 / 152 (178 checks).
Normal SDK browser, integrated frontend and CLI control build cleanly. Native
UI/audio qualification is recorded in the separate evidence receipt; independent
big-endian CI must complete before merge. No original assets or proprietary decompiled code are present
in this change.

## Recovery findings (private receipts retained outside Git)

The owner's Planetary Pack supplies 89 numbered technology MP3 files: 0–86,
88 and 92. ID87 is the disabled User Technology. All 88 files in the normal
0–88 table decode strictly; extra92 has an original bad MPEG header and is
quarantined/unassigned. This does not imply that every enabled technology is
encountered during a played campaign. Original decoder dispatch was examined,
not the full original executable executed.

All 90 technology PCXs and both original description files were recovered.
Eight AMB score definitions contain 220 unique literal sample-path references;
160 exact/case-insensitive matches were recovered from the owner's disc. The
remaining 60 paths are also absent from the owner's GOG installer listing.
These may include unused or alternate score references; they have not been
silently aliased. AMB instrument/keymap/sequencer playback is not implemented,
and recovery is not a claim of complete music playback.
