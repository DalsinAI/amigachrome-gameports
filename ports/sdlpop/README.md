# SDLPoP — m68k AROS

**Status:** COMPILED / RUNTIME TEST PENDING

## Upstream

- repository: https://github.com/NagyD/SDLPoP.git
- pinned commit: `3c5add5fb7f83d4ceb542823ab66d00146c4271b`

## Compile result

- target: m68k AROS / 68040
- executable: `prince`
- observed size: 1,149,368 bytes
- SHA-256: `a332101d35b6486111d2832d87b502fe211c12dd6edee938cee765b5e8fc7961`

## Successful portability bridge

The old code uses `alloca()` through a macro but the AROS/GCC build did not see a prototype and the project promotes the warning to an error.

The successful build added:

    -Dalloca=__builtin_alloca

together with the AROS include path and `-m68040`.

This is a compile-success bridge. The preferred final port is a tiny explicit source portability fix.

## Data

The pinned SDLPoP source includes the engine runtime `data/` tree used by our field-test staging. Original commercial Prince of Persia installs are not included here.

A separate two-level demo can be used as a comparison fixture during local testing.

## Gate

Start game -> level one -> animation timing -> keyboard/controller input -> save behaviour -> audio -> clean exit.
