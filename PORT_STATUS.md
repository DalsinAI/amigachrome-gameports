# Port Status

Snapshot date: **9 October 2026**

Primary target: **AmigaChrome AC090 / AmigaOS 3.x / 68040 + FPU**

**DONE means RELEASE.** A successful compile is intermediate evidence, not a release result.

## Compiler qualification hold

The currently installed GCC 16.2.0b stove has reproduced compiler defects on the
AC090/68040 target. All binaries produced with that stove are therefore
**provisional engineering artifacts**.

The fixed GCC 16.2 stove is being qualified separately. Every port below must be
cleanly rebuilt and its relevant runtime gates repeated with the approved fixed
stove before it can be promoted to RELEASE.

Source patches, package layouts, provenance work and runtime findings remain
valid engineering work unless a fixed-stove A/B test shows otherwise.

## Current revival lane

| Port | Build / package | Runtime | Current evidence / next gate |
|---|---|---|---|
| Neverball / Neverputt 1.6.0 | COMPILED; package verifier PASS | TESTING | Mesa softpipe renders the Neverball title/menu. Virgl opens a valid GL context but the old-stove binary produces a black frame; retest with the fixed stove before assigning the defect to OpenGPU/virgl. |
| C-Dogs SDL | COMPILED; 103 MB self-contained package staged | TESTING | AC090 launch reaches the executable, SDL2/OpenGPU module and bundled data. The old-stove binary then raises dead-end CPU alert **0x80000005 (divide by zero)** before a usable game frame. Preserve unchanged for fixed-stove A/B. |
| Chocolate Doom | COMPILED; staged | PENDING | Current provisional binary SHA-256: `5ecb67c242a002378bd79ea70760b6561285d622923bc32bca0435afb26eacfc`. Runtime requires an approved external/test IWAD. |
| OpenJazz | COMPILED; staged | PENDING | Current provisional binary SHA-256: `1ac53c090503687002559eaafcc5653674ea153e9374474834b299ff730d7177`. Runtime data remains external. |
| SDLPoP | COMPILED; staged | PENDING | Current provisional binary SHA-256: `51097f931538140895659af40f021d58ff20c348751c174bd093881c23600f6a`. Runtime data remains external. |
| UQM 0.8.0 | COMPILED | PENDING | Full AmigaOS/OpenGPU executable final-links using UQM's bundled regex and Amiga portability fixes. Provisional old-stove SHA-256 `4173391559e65d7cfb6c330cd2820bc33cb97a6402175f895c11c3f057f386b8`; fixed-stove rebuild and user-supplied content runtime qualification remain. |
| NXEngine-evo | Previous compile exists; current rebuild deferred | PENDING | The latest revival rebuild was stopped because unrelated toolchain work was saturating the host, not because of a port failure. Rebuild directly with the approved fixed stove. |
| AssaultCube | Client + server COMPILED | PENDING | OpenGPU client bring-up work is preserved. Clean fixed-stove rebuild, audio integration and AC090 runtime qualification remain. |
| Serious Sam Classic | SOURCE PINNED | NOT STARTED | TFE first. |
| Warzone 2100 | SOURCE PINNED | NOT STARTED | Large 3D/application stress target. |
| Doom 3 GPL source | SOURCE PINNED | NOT STARTED | Long-range renderer/engine stress target. |

C-Dogs provisional old-stove binary SHA-256:
`acf1b1cd8d42e8c9ccd93b246313481acbba79c71913bbf52551becb28573bfc`.

## Release gates

A port reaches **RELEASE** only when all applicable gates are green:

1. Clean build from the exact pinned source using the approved compiler stove.
2. Required Open-family services are used instead of private replacement stacks.
3. Package layout, notices, source provenance and SHA-256 inventory are complete.
4. Launch succeeds on a clean AC090 instance.
5. Graphics reaches a usable game/title frame.
6. Keyboard, mouse and controller paths are exercised as applicable.
7. Audio is exercised through the Open/SDL/AHI path as applicable.
8. Save/configuration behaviour is verified.
9. Window/fullscreen/focus behaviour is verified where applicable.
10. Clean exit is verified with no stuck task, screen or input state.
11. A final repeat is run on a clean instance.

## Data policy

Game data, ROMs and proprietary assets are not committed merely to make a test
convenient. Runtime datasets are either redistributable with recorded provenance
or remain user/test-supplied.

External network access is an explicit approval point. Source preparation is
offline by default and may only fetch after an approved `--allow-network` run.
