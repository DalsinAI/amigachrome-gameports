# AstroMenace — AmigaChrome AC090

**Status:** RUNTIME QUALIFIED / AC090 GCC16 0010 RELEASE CANDIDATE

## Upstream

- repository: https://github.com/viewizard/astromenace.git
- pinned commit: `bbdb3ac5af2774c92b85c4d9b2a238f606911e66`
- source licence: GPL-3.0-or-later
- notable upstream change: this pin includes the 6 August 2026 GCC 16 startup-hang fix.

## AmigaChrome treatment

The target is AmigaOS 3.x / AC090 / 68040 + FPU.

- SDL2 comes from the OpenGPU SDK.
- OpenGL comes from the OpenGPU GL compatibility surface and VirGL host path.
- Audio uses the AmigaChrome SDL2_mixer compatibility layer and OpenAudio/AHI path; the port does not bundle a second unrelated audio stack.
- Ogg/Vorbis, FreeType, PNG and zlib are shared SDK dependencies, not private copies carried by the game.
- A verified canonical `gamedata.vfs` is produced during release packaging. The duplicate unpacked `gamedata/` tree is deliberately omitted from the user package.
- Existing AstroMenace VFS, VW3D, VW2D and TGA data are read with explicit little-endian conversion on the 68k target.
- The AmigaOS build uses a target-safe random seed before SDL startup and reseeds after the SDL timer becomes available.
- Joystick startup is optional and non-fatal; keyboard and mouse remain available when no controller is present.

## Automated AC090 qualification

Runtime workflow `38091891845` passed on a disposable copy of AmigaChrome Instance-32. The qualification route used the real AC090 runtime and ordinary game transitions:

1. Open and validate the canonical VFS.
2. Initialise SDL2, OpenAudio and the VirGL/OpenGL context.
3. Load fonts, shaders, models, textures and other game assets.
4. Render and capture a fully faded main-menu frame.
5. Prepare a valid default pilot and enter mission selection.
6. Create, render and capture the shipyard/workshop with the configured ship.
7. Follow the workshop's normal fade-to-game route into mission one.
8. Render and capture a playable 3D mission frame.
9. Start a real SDL2_mixer channel and confirm that it reports active.
10. Pass synthetic key-down and key-up events through AstroMenace's normal SDL event loop.
11. Leave the game through its normal loop and resource-cleanup path.
12. Shut down audio and return guest code `0`.

The three captured frames passed structural and spatial-coherence checks and had distinct SHA-256 values:

| Frame | SHA-256 | Mean channel standard deviation | Sampled colours |
|---|---|---:|---:|
| Main menu | `d30cbdaf7e2eb23947070b885ce0e5ea59047c386d486e5aacdafdedfd02a319` | 36.8074 | 2,604 |
| Shipyard/workshop | `c796ccf359355abf9dc422da4b16bb612f884ca20fad1fae89fe813af633d27b` | 48.6444 | 6,861 |
| Mission one | `f8a1b89a56fc838349ac300463e0228fc4dcd2f4669f2f2d870b045a14d91323` | 42.6346 | 10,345 |

The qualified diagnostic binary was:

`c6fce537297dc7bb4c5f57f4309918b75715b61512a17bc3c384c3f6a23a5e4`

The canonical VFS was:

`0e457c97cc3ad4e41a30933f660064d2bc7b6d9ece0f372c0419e6fd38d86852`

## Release package

The production workflow builds:

`AstroMenace-AC090-GCC16-0010.zip`

The release package excludes all qualification sentinel strings, stage files and frame probes. It contains:

- the AmigaOS executable;
- `gamedata.vfs`;
- upstream and asset licences;
- AmigaChrome release notes and build provenance;
- checksums.

## Manual field checks still required

The automated gate proves that the software paths execute correctly, but these remain human-observed field checks rather than CI claims:

- audible music/SFX quality, latency and volume balance on real speakers;
- physical controller mapping, dead-zone and feel;
- mouse control feel and pointer capture/release;
- a longer gameplay and performance soak;
- additional missions, options changes, profiles, saves and persistence across restart.

**DONE means RELEASE.** Compile success alone does not make this port done; the automated gate is green, and the remaining items above are the final field-test campaign.
