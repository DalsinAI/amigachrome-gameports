# Serious Sam Classic — AmigaChrome source intake

**Status:** SOURCE PINNED / BUILD NOT STARTED  
**Priority:** after the first OpenGL/OpenGPU game is stable

## Upstream

- selected port base: https://github.com/tx00100xt/SeriousSamClassic.git
- pinned commit: `80b9893e5b74e5a2160eaf63e6d6b3f3981dfbbd`
- licence: GPL-2.0
- provenance: maintained cross-platform work derived from Croteam's open Serious Engine and the earlier Linux port
- game data: original Serious Sam Classic data is not open; users must supply their own data

## Why this base

This tree already carries Linux/BSD/macOS/Raspberry Pi work, CMake builds, SDL2 integration and repaired OpenGL paths for both The First Encounter and The Second Encounter. That is a substantially better portability starting point than redoing the original Windows-specific platform layer.

## First porting scan

Target **The First Encounter first**. Disable editors and authoring tools. Separate platform bring-up (filesystem, timer, input, audio, networking) from renderer bring-up, and use original user-supplied game data only.

## First-light gate

Engine starts with user data -> menu -> one TFE level -> player movement -> enemies -> sound -> clean exit.

No commercial game data is committed or redistributed here.
