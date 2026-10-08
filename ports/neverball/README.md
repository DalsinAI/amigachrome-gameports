# Neverball — AmigaChrome source intake

**Status:** SOURCE PINNED / BUILD NOT STARTED  
**Priority:** optional early 3D validation target

## Upstream

- repository: https://github.com/Neverball/neverball.git
- pinned commit: `a1ed09911dca262d80049c12a2824d683af494d6`
- licence: GPL-2.0-or-later; see upstream `LICENSE.md` for documented third-party exceptions

## Porting shape

Neverball is primarily C99 with SDL2 and OpenGL, plus SDL2_ttf, Vorbis, JPEG, PNG and optional curl/NLS/HMD integrations. The build already exposes switches for optional fetch, NLS, tilt and HMD support, so first light can deliberately turn those off.

This is attractive as a geometry, camera, texture and input test because the game workload is easy to understand visually and does not need the networking/gameplay complexity of an FPS.

## First-light gate

Title -> one built-in level -> camera/tilt input -> textured ball and level geometry -> timer/UI -> audio -> clean exit.

No upstream source is committed here.
