# Source-backed normal no-shadow scene and final-copy boundary

Host-only,25 October2026. New owned scene core; no existing viewer/frozen build
was replaced. Native execution, game-window integration and the full original
screen-copy wrapper remain UNRUN.

## Complete private-viewport comparison

The actual unchanged original scene helper0x639e60 executes sixteen complete
owner-data recipes: VI+Viptr00 infantry and the ten-part basic rover, each at all
eight facings with frame0 poses. Recipe ordering comes from prior original
append traces; this is not execution of the whole vehicle/game/allocator path.
The original flat-camera/Euler/inverse helpers prepare matrices/light. Real
projection, bounds, LOD selection, culling, object rendering, original per-object
palette callback0x618da0 and original memcpy execute, followed by real original
pixel kernels/patchers. No renderer callback or Windows import is substituted.
Instruction traces identify the actual executed helpers; these small scenes use
the unclipped raster kernel. Earlier group controls separately cover clipping.

Owned `CvrSceneRenderer` matches every indexed pixel and depth value:
16 scenes,3,145,728 bytes, SHA256
40dbf43dff5208607e780317b45966a47b46d8c6cfb57afb9e7826c3ed68c0a0.
This remains exact with independently computed flat-camera/facing/light/default
inputs, not just reference-supplied coefficients. Ninety-six transactional
invalid calls preserve prior valid output. Host ASan/UBSan and normal SDK builds
pass. Actual AmigaOS and independent big-endian execution are pending.

The private viewport is256 square, clear index9/depth65535, with shared depth,
original selected LOD/group seeds/hidden frame flags and per-component shade
indices. Gaian rover panel tables are the separately qualified original-derived
faction tables. RGB display still requires the separately qualified global
palette; per-model RGB palettes are not substituted. No owner assets, generated
original tables or proprietary instructions are committed.

## Correct numerical placement

* Flat camera uses the source1.0466667f tilt and source-facing angle
  ((facing−3)&7)*.785375f. Its position is its last row times−10000.
* Scene/model scale stays1. Camera magnification is.07 at original zoom0,
  multiplied by1.25 for basic infantry. Basic rover remains.07. This distinction
  matters: XY receives magnification, Z does not receive that same multiplier.
* Near/far are−65535/+65535; depth interval is0..65535. No arbitrary fitted
  origin,32768+Z*16 depth or silhouette-based centring is used.
* Both infantry body and its dedicated reactor append(-400,200,0). The previous
  frozen preview's omitted200 term is not silently patched into old evidence.
* Original normal Caviar flags1/3 do not request optional silhouette centring.
  This scope explicitly selects no shadows. Terrain slopes, other flags,
  alternate variants and animated action timing are not silently generalised.

## Final1:1 copy and marker anchor: exact source contract, wrapper UNRUN

Original0x617230 places the256-square private surface at supplied centre−128,
with transparent index9, destination clipping and optional separate downsample
mode. Its complete wrapper depends on actual USER32.dll IntersectRect and
UnionRect plus buffer/device state. Those imports are not replaced to fabricate
an execution claim. Only the normal1:1 numeric/compositing contract is isolated
in `cvr_scene_blit`; original downsampling/full wrapper remains unqualified.

75 independent destination-coordinate controls verify clipping, transparency,
padding and off-screen centres, plus four transactional invalid and two anchor
checks. Original Units.pcx metadata scans independently match infantry(13,12)
and rover(14,14), after the original+10 index remap. Only those two chassis are
qualified. Missing markers fail rather than inventing offsets.

At original zoom0, veh_draw adds those chassis marker offsets, then Caviar's
(50,50) centre offset. Thus caller XY maps to infantry centre XY+(63,62), rover
XY+(64,64). Caller XY is the original veh_draw coordinate, NOT an assumed tile
centre or foot. Binding the current viewer's tile coordinates to that original
caller convention remains required before claiming correct map placement.

## Handoff boundary

`cvr_scene_probe` renders both recipes from owner model directories and a
supplied faction table. Its optional prepared-state file is diagnostic; without
it the owned factory computes source-profile camera/facing/defaults itself.
`build_cvr_scene.sh probe|blit OUTPUT` uses the normal SDK. There is no scene
cache or UI integration yet. Keep native/performance qualification, original
caller/tile mapping, final blit/downsampling, terrain/shadow/animation and other
profiles explicit. All preceding frozen branches/commits remain intact.
