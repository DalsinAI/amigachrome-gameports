# AmigaChrome Game-Port Test Releases

Snapshot: 9 October 2026

## Rule

**DONE means RELEASE.** These are test-release payloads until the AC090 runtime gates are complete.

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
| AstroMenace | GCC16/0009 AmigaOS build PASS | **BUNDLED** | self-contained package; first launch creates `gamedata.vfs` from bundled licensed `gamedata/` |
| Chocolate Doom | fixed-GCC build lane | **AUTO-FETCH / BUNDLE-OK** Freedoom 0.13.0 | staging fetches verified Freedoom and installs `freedoom1.wad` |
| NXEngine-evo | GCC16/0009 AmigaOS build PASS | **AUTO-FETCH** Cave Story freeware data | staging fetches public NXEngine-compatible data and installs it beside the engine |
| The Ur-Quan Masters | build green in fixed-GCC lane | **AUTO-FETCH** official UQM 0.8 base content | staging downloads verified official content and puts it under `content/packages` |
| OpenOMF | active AmigaOS portability/build lane | **AUTO-FETCH** OMF2097 freeware assets | staging fetches the asset archive linked by OpenOMF upstream and installs it locally |
| OpenJazz | build green in fixed-GCC lane | **AUTO-FETCH** Jazz Jackrabbit 1.1 shareware | staging downloads the shareware episode only; registered/full data is never mirrored |
| SDLPoP | build green in fixed-GCC lane | project runtime data + **AUTO-FETCH** two-level demo reference | staging uses the pinned project data and optionally fetches the public demo |
| C-Dogs SDL | build green; local self-contained test package exists | local test package only for now | upstream data provenance review remains open before public release packaging |
| AssaultCube | client/server build green | upstream package-data review still required | not yet in one-click test-release state |

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

Active GCC16 qualification branch:

`runner/gcc16-gameports-20261009`

### Local build/cache - disposable

Relative to a working checkout:

- `build/os3/` - current AmigaOS build outputs
- `build/game-ports/test-data-cache/` - fetched public runtime datasets
- `build/game-ports/out/field-test/` - staged field-test tree where the legacy aggregate lane is used

These paths are build products and may be cleaned/rebuilt.

### GitHub Actions test artifacts

The GCC16 Kitchen workflow publishes the focused payload as:

`AC090-FinishThree-0009-<run-id>`

Artifact retention is **90 days** on the current qualification workflow.

GitHub Actions artifacts are test evidence, not the permanent public release channel. Once a port clears the runtime release gates it should be promoted to a versioned GitHub Release package.

## Public data currently recorded

- Freedoom 0.13.0 - verified SHA-256
- Cave Story / NXEngine-compatible freeware data - public fetch, observed hash recorded
- UQM 0.8.0 base content - verified SHA-256
- One Must Fall 2097 OpenOMF assets - public upstream fetch, observed hash recorded
- Jazz Jackrabbit 1.1 shareware - public shareware fetch, observed hash recorded
- Prince of Persia two-level demo - public demo fetch, observed hash recorded

No registered/full commercial game dataset is silently substituted for these public test datasets.
