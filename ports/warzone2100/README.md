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

## Pinned-source review, 24 October 2026

The local checkout was checked at the exact catalog pin above; no configure,
compile or runtime pass is claimed. In particular, this revision requires
**SDL3 3.2.12** (`lib/sdl/CMakeLists.txt`), rather than the SDL2 SDK used by
Neverball/AssaultCube. The root build requires C++20 and defaults
`WZ_ENABLE_BACKEND_VULKAN` on for supported platforms.

The checked CMake files also require PhysFS and Sodium >= 1.0.16
(`lib/framework`), curl (`cmake/IncludeFindCurl.cmake`), PNG and FreeType
(`lib/ivis_opengl`), OpenAL/Ogg/Vorbis/Opus (`lib/sound`), and OggVorbis
(`lib/sequence`). Optional Opusfile is separately detected. This is a minimum
observed dependency list, not a complete transitive inventory.

Before a target build, pin and qualify those dependencies and the submodules,
provide the OpenGPU SDL3/GL interface and the Open-family audio path, and route
curl's TLS through OpenTLS. A first OpenGPU GL build should explicitly disable
the upstream Vulkan backend; it must not silently pick host libraries or bundle
an unrelated SDL/Mesa/TLS stack. OpenMulticore work must keep Exec/SDL calls on
the OS CPU and qualify worker jobs/fallbacks separately. No acceleration or
plain-68040 performance result follows from this source review.
