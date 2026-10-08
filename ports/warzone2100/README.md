# Warzone 2100 — AmigaChrome source intake

**Status:** SOURCE PINNED / BUILD NOT STARTED  
**Priority:** later 3D/application stress target

## Upstream

- repository: https://github.com/Warzone2100/warzone2100.git
- pinned commit: `d7ce18df8d998c968915a5e6410a68626927813e`
- licence: GPL-2.0 project; preserve upstream `COPYING`, `COPYING.NONGPL` and `COPYING.README`
- submodules: enabled and pinned by the upstream commit

## Porting shape

Current upstream is a modern CMake/C++ codebase. The root build defaults to C++20, while some components support C++17. It also has a large dependency/submodule surface. This is therefore not the first compiler bring-up target; it is a later test of the toolchain, SDL/platform layer, renderer, networking, audio and large-application memory behaviour together.

## First-light gate

Main menu -> start skirmish -> terrain and units render -> camera movement -> unit selection/pathing -> audio -> clean exit.

The exact dependency set will be reduced for AmigaChrome before a first compile is attempted.
