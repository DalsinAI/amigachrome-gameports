# Owned original CVR groups and raw-object fidelity

Host-only,25 October2026. This extends the owned reader on a separate branch;
frozen reader/rover/palette/projection/fixed candidates are preserved. No lab or
live preview was accessed and nothing was published as a PR.

## Preserved representation

Each selected geometry level now owns its original dimensions, position,
rotation centre, geometry flags and bounded group array. A group retains its
first/count range in the decoded voxel array, original stream entry count,
all twelve signed jump-header values and widened seed. Single streams retain
the header rotation centre; jump streams retain position plus offset. STOP
entries remain excluded from drawn voxels but included in stream entry counts.

The seed must be signed32, because it adds two signed16 values. Valid synthetic
files already accepted by the old parser demonstrate32768 and−32769 seeds whose
first plotted voxels fit signed16. A new explicit widened fixed-seed entry point
preserves these values; the earlier signed16 entry point remains available.
Two identical absolute voxels with different original seeds can have different
fixed positions after rounding. A control demonstrates this instead of silently
regrouping flattened voxels.

Group capacity grows geometrically to the existing4096-per-object limit. All
error paths, LOD replacement/discard and model destruction release owned groups.
The same bounded stream/STOP/bounds/unsupported-linked-header rules remain;
adding metadata does not loosen rejected profiles. Loaded arrays survive input
poison/free and failed reload. No original pointers are imported.

## Actual verification

* Existing32 reader and40 LOD host sanitizer controls pass unchanged.
* Four new metadata fixtures and three malformed fixtures pass; eight fixed
  group controls cover widened seeds, distinct seeds at one absolute point,
  and transactional invalid inputs.
* All2,373 group/header fields across12 owner models/69 objects/89 groups match
  a separate raw-file parser, including infantry, its reactor and all ten basic
  rover components. All earlier decoded voxel/animation values remain intact.
* Actual unchanged original object renderer0x63e950, its shade/direction helpers,
  and real original clipped/unclipped pixel kernels and patchers execute those
  geometries. The ordinary controlled viewport uses the unclipped kernel;
  a second clipped viewport actually executes both kernels (instruction traces
  confirm this). No renderer callbacks are substituted.
* Owned group/fixed-seed/lighting/raster output matches every pixel/depth byte:
  24 model/viewport cases,4,718,592 bytes. Ordinary SHA256:
  9904d47e96124155ba002e69e2f0634dee68b8cd6ebc224892ef048285cfaf21.
  Clipped SHA256:
  237987b81076a6b52239ea8d5152bf4726db447731cbe2837cfcccc9f497ae33.

These are controlled raw-object draws: identity camera/object projection,
explicit scale.15 and origin(128,128,32000), common depth, synthetic shade-index
palette and selected original geometry. They deliberately do not claim original
animation, complete component assembly, faction colours, original camera or
final game-screen placement. Original geometry/instructions/palettes stay
private. No full PE startup, imports or game was run.

## Integration boundary

Normal SDK metadata/raster/fixed probes build; actual AmigaOS and independent
big-endian execution remain UNRUN. The viewer is not changed to consume these
new groups. Its old previews and hashes are not silently replaced.

The recovered source facts remain explicit: infantry/reactor append offset
(-400,200,0), normal camera position=last camera row*−10000, inspected constructor
near/far−65535/+65535 and depth0..65535. These are inspection facts, not an
executed full constructor/default-state or final blit proof. Original terrain,
shadow, alternate flags/default changes, and final256-square clipping/blit
still need qualified integration. Do not infer full in-world fidelity from
this controlled raw-object equivalence.
