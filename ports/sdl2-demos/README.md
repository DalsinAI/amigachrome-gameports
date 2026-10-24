# OpenUp SDL2 Demos

Thirteen small, asset-free SDL2 applications for exercising OpenGPU, OpenInput,
OpenAudio and OpenBench on AmigaOS 3.2.x. They are deliberately compact and
independent: a failure in one demo does not stop the others from being built or
tested.

Target: **AC090 / 68040 + FPU / OpenGPU SDL2 2.32.10**.

## Renderer proof in every window

Every demo queries `SDL_GetRendererInfo()` after creating its renderer and keeps
the result visible in the window title. A healthy accelerated instance reads,
for example:

```text
OpenFire | opengpu [ACCELERATED+VSYNC+TARGET] | heat 8 | 60.0 fps
```

A fallback is deliberately obvious:

```text
OpenFire | software [SOFTWARE/CPU+TARGET] | heat 8 | 12.4 fps
```

The same information is printed to the launching Shell. The applications still
allow SDL's software renderer so the fallback can be diagnosed rather than
turning into an unexplained launch failure. The title reports the selected SDL
renderer; individual OpenGPU commands may still use OpenGPU's documented
per-operation CPU fallback when a backend does not advertise that operation.

## Phase one

| Demo | Main workload | Controls |
|---|---|---|
| `OpenFire` | Streaming ARGB texture, palette animation | `C` palette, Up/Down heat, Space pause |
| `OpenTunnel` | Procedural polar texture, dithering, alpha texture and `RenderCopyEx` | Arrows speed/direction, Space pause |
| `OpenTriangles` | Accelerated `SDL_RenderGeometry` / OpenGPU TRIANGLES | `T` texture, `B` blend mode, mouse drag, Space pause |
| `OpenEffects` | Plasma, metaballs, rotozoom and deformation | Left/Right or Space effect, `P` pause |
| `OpenRay` | Interactive raycaster, continuous keyboard state and minimap | WASD/arrows, Shift run, mouse drag, `M` map |

## Phase two

| Demo | Main workload | Controls |
|---|---|---|
| `OpenHypno` | Line tunnel, star trails, perspective floor and wireframe cube | Arrows reverse, Space pause |
| `OpenIntro` | Star trails, Doom fire, sine scroller, alpha blending and scanlines | Space pause |
| `OpenFloor` | Perspective grid, copper bars, shaded/rotated Boing Ball | Arrows speed, `C` palette, Space pause |

## Phase three

| Demo | Main workload | Controls |
|---|---|---|
| `OpenSprites` | Hundreds of alpha/colour-modulated rotating texture copies | Arrows count, `B` blend, `C` colour, Space pause |
| `OpenTarget` | Render-to-texture, target composition, rotation and flips | `B` blend, `T` trails, `R` reset, Space pause |
| `OpenParticles` | Large additive or alpha particle batch with gravity | Arrows count, `B` blend, `G` gravity, Space pause |
| `OpenBoids` | Flocking simulation rendered in one `SDL_RenderGeometry` call | Mouse attract/repel, arrows count, `W` edges, `C` colour, `R` reset |
| `OpenScope` | Generated SDL audio through the Amiga audio backend plus oscilloscope drawing | Arrows pitch, `W` waveform, `M` mute, Space pause |

All demos use `F` or `F11` for fullscreen and `Esc` for a clean exit. Their
initial windows are resizable and use a small logical surface so the OpenGPU
renderer performs scaling while the 68040 only generates the intended retro
workload.

## Build and deployment

`build.sh` uses the packaged GCC 16 OpenGPU SDK (`sdl2-config`) and builds the
phases in order. As soon as each binary links, it is atomically installed on the
folder-backed DH1 volume; the next demo starts without waiting for the rest of
its phase.

Default destination:

```text
$AC_INSTANCE/devices/harddisks/DH1/SDL2 Demos   (AC_INSTANCE defaults to ~/AmigaChromeInstances/Instance-32)
```

Amiga-side launch:

```text
CD "DH1:SDL2 Demos"
OpenFire
```

Each executable has a matching `.readme` and `.SHA256SUMS`; the directory also
contains phase and complete checksum inventories. CI retains the full build tree
and compiler logs as a workflow artifact.

## Status

**24 October 2026:** all thirteen demos rebuilt successfully with the verified
GCC 16 open3 stove and its frozen SDL2 2.32.10 SDK. The rebuilt binaries,
readmes and checksum inventories are installed on Instance-32 at
`DH1:SDL2 Demos`. Every installed binary matches its new SHA-256 and retains
the FPS title string. The old directory was backed up before replacement.
See [the deployment receipt](evidence/open3-instance32-20261024.json).
This records compilation and installation only: this pass did not launch the
programs, replace any libraries or change autostart. Runtime and release gates
below remain open. The selected SDL renderer label does not measure individual
commands executed on the host GPU.

The complete thirteen-binary set is expected to cross-compile with GCC 16.2.0b
and deploy to Instance-32 DH1 through the self-hosted daletop runner. It remains
an engineering test set until each program has been launched, interacted with,
resized/fullscreened and exited on the guest. `OpenScope` additionally needs a
human audio check.

## Licence

MIT, Copyright (c) 2026 Dalsin Limited.
