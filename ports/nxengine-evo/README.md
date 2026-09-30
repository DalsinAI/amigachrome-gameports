# NXEngine-evo — m68k AROS

**Status:** COMPILED / RUNTIME TEST PENDING

## Upstream

- repository: https://github.com/nxengine/nxengine-evo.git
- pinned commit: `1f093d1423cc395eb199230cd609b806ef1daa36`

## Compile result

- target: m68k AROS / 68040
- executable: `nxengine-evo`
- observed size: 17,390,596 bytes
- SHA-256: `3f36ae8bcf0efb89db9317e2891cd8f6da3b0ef0aa7acf4f5178b789202e0abe`

## Successful portability bridge

Upstream `ResourceManager.cpp` currently recognises a set of Unix-like platforms but not AROS. The compile-success probe used:

    -D__unix__

CMake then produced the full m68k AROS executable.

This flag is intentionally documented as a bridge, not the final solution. The proper port should recognise `__AROS__` directly and audit any remaining Unix assumptions.

## Data

NXEngine-evo requires Cave Story data. The AmigaChrome field lab uses freeware Cave Story/NXEngine data locally. No game data is committed here.

## Gate

Opening room -> map/sprites -> movement -> input -> audio -> clean exit.
