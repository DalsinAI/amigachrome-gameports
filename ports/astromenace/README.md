# AstroMenace — AmigaChrome AC090

**Status:** SOURCE PINNED / FIRST KITCHEN BUILD WIRED

## Upstream

- repository: https://github.com/viewizard/astromenace.git
- pinned commit: `bbdb3ac5af2774c92b85c4d9b2a238f606911e66`
- source licence: GPL-3.0-or-later
- notable upstream change: this pin includes the 6 August 2026 GCC 16 startup-hang fix.

## AmigaChrome treatment

The target is AmigaOS 3.x / AC090 / 68040 + FPU.

- SDL2 comes from the OpenGPU SDK.
- OpenGL comes from the OpenGPU GL compatibility surface.
- audio must resolve through the OpenAudio/OpenAL compatibility surface supplied by the approved AmigaChrome SDK; the port must not bundle a second unrelated audio stack.
- Ogg/Vorbis and FreeType are shared SDK dependencies, not private copies carried by the game.
- target-side VFS generation is disabled during cross-build; runtime data packaging is a separate, reproducible release step.

## First gate

1. clean GCC16/0009 Kitchen build;
2. title/menu;
3. ship configuration;
4. first mission and rendered 3D scene;
5. keyboard/pad input;
6. audio through OpenAudio;
7. clean exit.

**DONE means RELEASE.** Compile success alone does not make this port done.
