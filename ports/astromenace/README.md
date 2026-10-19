# AstroMenace — AmigaChrome AC090

**Status:** SOURCE PINNED / FIRST KITCHEN BUILD WIRED

## Upstream

- repository: https://github.com/viewizard/astromenace.git
- pinned commit: `bbdb3ac5af2774c92b85c4d9b2a238f606911e66`
- source licence: GPL-3.0-or-later
- notable upstream change: this pin includes the 6 August 2026 GCC 16 startup-hang fix.

## AmigaChrome treatment

The target is AmigaOS 3.x / AC090 / 68040 + FPU.

- SDL2 comes from the OpenGPU SDK (`sdl2-config --libs --gl`).
- OpenGL comes from OpenGPU's GL.module (Mesa): virgl on the host GPU where there is one, softpipe on the 68k otherwise.
- audio goes through the SDK's SDL2_mixer to AHI (`audio_amigachrome.cpp` replaces the OpenAL backend); the port bundles no second audio stack.
- mipmaps: the GLU shim calls `glGenerateMipmap` once after each upload, not the legacy flag that rebuilds every level on each sub-update.
- built with `-fno-tree-loop-distribute-patterns`, as the OpenGPU SDK asks for GCC 16.

## Running it fast

- A truecolour RTG screen and openrtg.library 0.15 or later, so each frame goes straight to the screen: `GetEnv GLAPresent` says `direct`. Otherwise every frame is read back and copied, about a quarter of the 68k's time.
- SDL2.module 3 with the text-input fix (openamigartg #67) for the pilot name. The game needs no relink for it.
- `SDL_OPENGPU_STATS=1` shows what is drawn where. VSync in the game's options does nothing: OpenGPU stores the swap interval but does not wait.
- Known cost: each texture or buffer transfer is a synchronous round trip to the host, and client-side vertex arrays (AstroMenace's 2D and text) are copied every draw. Screens with many strings (the pilot profile) pay that per string.
- Ogg/Vorbis and FreeType are shared SDK dependencies, not private copies carried by the game.
- target-side VFS generation is disabled during cross-build; runtime data packaging is a separate, reproducible release step.

## First gate

1. clean GCC16/0009 Kitchen build;
2. title/menu;
3. ship configuration;
4. first mission and rendered 3D scene;
5. keyboard/pad input, including typing the pilot name;
6. audio through OpenAudio;
7. clean exit;
8. `GLAPresent` is `direct`, and the menu and first level hold a measured frame rate.

**DONE means RELEASE.** Compile success alone does not make this port done.
