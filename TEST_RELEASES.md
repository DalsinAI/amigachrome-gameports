# AmigaChrome Game-Port Test Releases

Snapshot: 10 October 2026

## Rule

**DONE means RELEASE.** These remain test-release payloads until each port clears its own AC090 runtime gates. AstroMenace has now cleared its automated AC090 software gate and is an AC090 GCC16/0010 release candidate pending the listed human field checks.

Runtime data follows one of three policies:

1. **BUNDLED** - redistribution is clearly permitted and the data travels with the test package.
2. **AUTO-FETCH** - the data is publicly available, but AmigaChrome fetches it from the recorded public/upstream source rather than republishing it.
3. **USER DATA** - no safe public acquisition path has been approved; the user supplies their own data.

The machine-readable source of truth is:

- `gameports/catalog.json` - per-port policy
- `gameports/runtime-data.json` - public dataset sources/checksums
- `scripts/fetch_runtime_data.py` - downloader/cache
- `scripts/stage_game_ports_for_test.py` - package/staging installer

## Current data experience

| Port | Engine/build state | Runtime data | Test experience |
|---|---|---|---|
| Neverball / Neverputt | built/package path established | **BUNDLED** | self-contained package |
| AstroMenace | **AC090 GCC16/0010 automated runtime gate PASS** | **BUNDLED** | self-contained release candidate with verified canonical `gamedata.vfs`; qualified main menu, mission selection, configured-ship workshop, mission-one rendering, live mixer channel, keyboard event path and clean guest return code `0` |
| Chocolate Doom | fixed-GCC build lane | **AUTO-FETCH / BUNDLE-OK** Freedoom 0.13.0 | staging fetches verified Freedoom and installs `freedoom1.wad` |
| NXEngine-evo | GCC16/0009 AmigaOS build PASS | **AUTO-FETCH** Cave Story freeware data | staging fetches public NXEngine-compatible data and installs it beside the engine |
| The Ur-Quan Masters | build green in fixed-GCC lane | **AUTO-FETCH** official UQM 0.8 base content | staging downloads verified official content and puts it under `content/packages` |
| OpenOMF | active AmigaOS portability/build lane | **AUTO-FETCH** OMF2097 freeware assets | staging fetches the asset archive linked by OpenOMF upstream and installs it locally |
| OpenJazz | build green in fixed-GCC lane | **AUTO-FETCH** Jazz Jackrabbit 1.1 shareware | staging downloads the shareware episode only; registered/full data is never mirrored |
| SDLPoP | build green in fixed-GCC lane | project runtime data + **AUTO-FETCH** two-level demo reference | staging uses the pinned project data and optionally fetches the public demo |
| C-Dogs SDL | build green; local self-contained test package exists | local test package only for now | upstream data provenance review remains open before public release packaging |
| AssaultCube | client/server build green | upstream package-data review still required | not yet in one-click test-release state |

## AstroMenace field checks still open

The automated AC090 gate proves the executable, data, graphics, audio channel, keyboard event and shutdown paths. It does not replace human observation of:

- audible music/SFX quality and latency;
- physical gamepad mapping and feel;
- mouse control and pointer capture/release;
- a longer gameplay/performance soak;
- additional missions, settings, profiles and save persistence.

Qualification evidence is documented in `ports/astromenace/README.md`.

## One-command field-test staging

From a checkout of this repository:

```sh
python3 scripts/stage_game_ports_for_test.py \
  --root . \
  --fetch-public-data \
  --instance /path/to/AmigaChrome/instance \
  --volume DH1
```

The fetcher:

- downloads only datasets listed in `gameports/runtime-data.json`;
- verifies SHA-256 where a trusted checksum is pinned;
- records the observed SHA-256 for public sources without a pinned publisher checksum;
- caches downloads below `build/game-ports/test-data-cache/`;
- does not commit downloaded game data to Git.

The staging tool installs the resulting field-test tree to:

```text
<instance>/devices/harddisks/DH1/AmigaChrome/GamePorts/
```

Inside the guest this is:

```text
DH1:AmigaChrome/GamePorts/
```

## Where build products live

### Source and recipes - persistent

GitHub repository:

`DalsinAI/amigachrome-gameports`

Active AstroMenace qualification branch:

`fix/astromenace-runtime-diagnostics-20261009`

### Local build/cache - disposable

Relative to a working checkout:

- `build/os3/` - current AmigaOS build outputs
- `build/game-ports/test-data-cache/` - fetched public runtime datasets
- `build/game-ports/out/field-test/` - staged field-test tree where the legacy aggregate lane is used
- `build/releases/astromenace-ac090-0010/` - reproducible AstroMenace production package and manifest

These paths are build products and may be cleaned/rebuilt.

### GitHub Actions test artifacts

The older aggregate GCC16 Kitchen workflow publishes focused test payloads as:

`AC090-FinishThree-0009-<run-id>`

AstroMenace now has focused workflows that publish:

- `AstroMenace-AC090-RUNTIME-DIAGNOSTIC-<run-id>` - disposable qualification binary and runtime evidence;
- `AstroMenace-AC090-GCC16-0010` - lean production release candidate, manifest and SHA-256 file.

Artifact retention is 30 days for runtime diagnostics and 90 days for the production candidate.

GitHub Actions artifacts are qualification evidence, not the permanent public release channel. Once the human field checks are complete, the candidate should be promoted to a versioned GitHub Release package.

## Public data currently recorded

- Freedoom 0.13.0 - verified SHA-256
- Cave Story / NXEngine-compatible freeware data - public fetch, observed hash recorded
- UQM 0.8.0 base content - verified SHA-256
- One Must Fall 2097 OpenOMF assets - public upstream fetch, observed hash recorded
- Jazz Jackrabbit 1.1 shareware - public shareware fetch, observed hash recorded
- Prince of Persia two-level demo - public demo fetch, observed hash recorded

No registered/full commercial game dataset is silently substituted for these public test datasets.
