# Explicit terrain dispatch and coherent indexed ground plates

This child adds a concrete offscreen ground-layer compositor using the verified
mesh, indexed palette preparation, ordinary point lighting and pixel stages.
It does not enable the frontend or infer complete world rendering.

## Source material and UV context

The API requires an explicitly supplied source tile identity, resolved
visibility/farm-render flags, rockiness, moisture, dunes and transition mask.
Unavailable identity rejects transactionally. No serialized pointer is
dereferenced or automatically used as runtime identity; no coordinate hash or
World gameplay RNG fallback exists.

Private unchanged625750/625770 and numeric caller regions463e92..463f91,
4641b4..464353 and46435f..4643a9 executed6,144controlled contexts. Observer
boundaries collect the UV/texture/fog choices before draw calls. These numeric
controls do not pretend a callback returned or claim a full world caller ran.
Instruction-backed load_terrain metadata identifies each selected56x56 crop.

Transition selection is generated as15rotational blob shapes: corner bits apply
only when both adjacent cardinal edges exist, with source symmetry tie order
0/3/2/1. This geometric classifier matches254of256source entries. The original
metadata's two exceptions, masks202and218, explicitly select variant11,
rotation2. They are preserved as observed compatibility cases; whether the
original entries were intentional is not inferred. The original512byte lookup
is not copied into the implementation.

The source's pixel-centred UV seams and crop roles are represented as atlas
format metadata. Farms suppress UV rotation even when farm rendering is off;
rockiness still selects the transition variant. Enabled farms take precedence
over dunes. Moisture overlay crops use the source's interleaved load order,
not a guessed linear order. Visible contexts select fog-1; fogged contexts65.

All6,144owned plans match original numeric results exactly on host ASan/UBSan
and independent big-endian68040. The portable plan output is811,008bytes,
SHA256`ca6b0406f0ebb1b17de479a6d87849c313ada707d8082bccc2d6c7d9817f4bf8`.
Six missing-context/bounds/partial-alias controls and partial input also pass.
An initial alias TEST formed a misaligned reference before the API ran; the
final controls use aligned full/partial overlap and fail sanitizers on errors.
No production arithmetic or expected output changed to hide that failure.

## Coherent ground-layer component

`render_terrain_ground_plate` accepts already qualified caller geometry, point
lights, explicit material context and an already prepared indexed atlas. For
each of four original ground triangles it draws the selected plate and then
its optional keyed moisture layer. It stages the entire small offscreen surface
before commit, rejects destination overlap with inputs/atlas/descriptors, and
preserves the destination on any failure. Scratch storage is bounded (surface
at most256x256, pitch at most256), and allocated privately without SDK overlays.

Private original comparison executes actual unchanged Draw3/Draw2/Trans5/Trans4,
edge setup, division and real RAM Buffer acquire/release calls. The original
numeric material/UV regions execute too. Mesh coordinates and light values are
supplied through the already-qualified numeric adapter boundary, not fabricated
callbacks; source-level full world height/visibility/caller execution is not
claimed. Observers stop after actual draw returns; original stacks and Buffer
locks balance. Physical display/OS calls never run.

Both48owner-atlas patches and48public synthetic numeric patches match all
786,432pixels per corpus, on host and independent68040. Controlled profiles
include two zooms, clipped/sloped ground, farm/dunes/material/fog choices and
signed lights. Owner output SHA256:
`23da222d2e91b50f1e8f88de2a933a3f792ae7edede18c7c47e072cfeebb95ba`.
Synthetic output SHA256:
`4a966c7042e90f1ccdedf05b5ffdd89fcdd1cf7c6b6d271c9fb0c562e2e8e374`.
The synthetic indexed patterns are automated test fixtures, not replacement
game art, and are not presented in the game. Public controls add12transactional
rejections and partial input. Original assets/instructions remain private.

Normal SDK builds both probes. All92parent workflow commands are retained,
with four new commands. Actual native AmigaOS, final CI, frontend integration
and visible game qualification remain unrun; the user-owned watch is untouched.

## Remaining scene boundaries

This renders a ground plate, not the full tile or map. A real caller still must
supply proven rendering identity and resolved visibility/transition context;
historical save pointer bytes do not establish a current allocation. Water,
coast masks, rivers, farms' other facilities, roads, forest/fungus/resource
layers, fog visibility, units/base occlusion and physical screen copy remain
separate. Existing CVR camera/append/projection proofs retain their scopes.
No full original-world scene, restored campaign or native fix is claimed.
