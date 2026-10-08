# Doom 3 — AmigaChrome source intake

**Status:** SOURCE PINNED / BUILD NOT STARTED  
**Priority:** long-range renderer/engine stress target

## Upstream

- source base: https://github.com/id-Software/DOOM-3.git
- pinned commit: `a9c49da5afb18201d31e3f0a429a037e56ce2b9a`
- licence: GPL-3.0 with id Software's additional terms in `COPYING.txt`
- game data: proprietary; users must supply their own Doom 3 data

## Source-base decision

Use id Software's GPL release as the port source. Modern Doom 3 source ports are useful technical references, but the AmigaChrome port should not be based on a project whose contribution policy rejects AI-assisted code.

The original tree uses SCons and contains the historical renderer/platform code we need to understand. This is a late target: it expects considerably more from the 3D stack, memory system and C++ runtime than AssaultCube or Neverball.

## First-light gate

Engine starts with user-owned data -> console/menu -> load one map -> render first frame -> input -> audio -> clean exit.

No Doom 3 game data is committed or redistributed here.
