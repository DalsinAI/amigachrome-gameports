# OpenUp SDL2 Demos

Eight small, asset-free SDL2 applications for exercising OpenGPU, OpenInput and
OpenBench on AmigaOS 3.2.x. They are deliberately compact and independent: a
failure in one demo does not stop the others from being built or tested.

Target: **AC090 / 68040 + FPU / OpenGPU SDL2 2.32.10**.

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

All demos use `F` or `F11` for fullscreen and `Esc` for a clean exit. Their
initial windows are resizable and use a small logical surface so the OpenGPU
renderer performs scaling while the 68040 only generates the intended retro
workload.

## Build and deployment

`build.sh` uses the packaged GCC 16 OpenGPU SDK (`sdl2-config`) and builds phase
one first. As soon as each binary links, it is atomically installed on the
folder-backed DH1 volume. Phase two starts immediately after phase one.

Default destination:

```text
/home/da1ek/AmigaChromeInstances/Instance-32/devices/harddisks/DH1/SDL2 Demos
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

The complete eight-binary set cross-compiles successfully with GCC 16.2.0b and
has been deployed to Instance-32 DH1. It remains an engineering test set until
each program has been launched, interacted with and exited on the guest.

## Licence

MIT, Copyright (c) 2026 Dalsin Limited.
