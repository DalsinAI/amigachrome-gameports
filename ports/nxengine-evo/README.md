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

## AROS portability

Upstream `ResourceManager.cpp` recognises several Unix-like targets but not AROS. The preserved compile-success probe previously forced `-D__unix__`.

The AmigaChrome lane now carries `patches/0001-aros-resource-manager.patch`, which recognises `__AROS__` explicitly for the existing stat-based resource lookup path. The build script applies and verifies that patch and no longer impersonates a Unix target.

The remaining gate is a clean rebuild with the current AROS GCC16 toolchain followed by AC090 runtime qualification.

## Data

NXEngine-evo requires Cave Story data. The AmigaChrome field lab uses freeware Cave Story/NXEngine data locally. No game data is committed here.

## Gate

Opening room -> map/sprites -> movement -> input -> audio -> clean exit.
