# The Ur-Quan Masters -> AGA/040

Pinned build baseline: official UQM `0.8.0` source archive.

- source SHA-256: `24f2f7db9cf7faf53b95f9e2580e6f596205a98ed0c335cfe834c64785ad4f5a`
- required base content SHA-256: `77d75ac25e6fb755a33c4ba3b38a7b7bc41fcbc02896891b0cc9ac9214b72eef`

The live SourceForge tree is newer; the stable archive is used for the first reproducible port because it has an independently published checksum. We can rebase to current `main` after the platform layer works.

## First port cut

1. Keep the game/core code unchanged.
2. Replace the SDL graphics/input boundary under `libs/graphics` / `libs/input` with ACGame/Amiga implementations.
3. Target the game's natural 320x240 presentation.
4. Start without speech and optional 3DO music; ship no content in the AmigaChrome source repository.
5. Use indexed AGA for navigation, combat and planet gameplay.
6. Add an offline/pre-display HAM8 path for conversation portraits, title/loading art and other mostly-static full-screen imagery.

## Licensing boundary

Program code is GPL. UQM content has separate Creative Commons/custom terms including non-commercial material. The port recipe therefore treats source and content as distinct inputs and does not silently relicense or bundle the content.

## First visible gate

Reach the main menu, start a new game, display the solar-system view and open one alien conversation screen; HAM8 is optional at the first gate but is the intended second visual milestone.

