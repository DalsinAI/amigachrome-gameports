# OpenJazz — AmigaOS 3.x / AmigaChrome

Target: **AC090 / AmigaOS 3.x / 68040 + FPU**

Pinned upstream commit: `a1626f4edd4a7af72c54103021790c56d8ecfced`.

## Status

**COMPILED — provisional old-stove artifact.**

A pristine local archive of the exact pin rebuilt successfully with no network
access using only the checked Amiga patch and build recipe.

Clean provisional SHA-256:

`c6f48d76e5c42fc264e134cef0d41d2b6769fc2dbb0448429bff1aad311a934e`

The only source portability patch currently required adds the standard C++
`<ctime>` declaration needed by the pinned source.

## Platform route

- SDL2 → Open-family SDL/OpenGPU/OpenInput stack;
- network disabled for the first gate;
- scaling disabled in the port build;
- portable file layout;
- target flags: 68040 + FPU.

## Runtime data

OpenJazz requires Jazz Jackrabbit game data. That data is external and is not
committed to this repository.

## Release gate

Title/menu → enter an authorised test level → scrolling/rendering →
keyboard/controller → audio → save/config → clean exit, followed by the
fixed-stove clean-instance repeat.

**DONE means RELEASE.**
