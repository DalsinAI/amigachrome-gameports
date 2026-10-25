# Original model-to-viewport projection boundary

Host-only recovery on 25 October 2026. Native execution and integrated map
placement are UNRUN. No running watch/lab was accessed. Palette integration
adcbd7efadc5345b2abb020d27e4bb05543fb66f and animation research06c0a8f are preserved.

## Exact observation boundary

Privately executed unchanged original helper0x6396a0 together with its actual
matrix/vector, floor and float-to-integer helpers. There are no substitute
renderer callbacks, Windows imports, PE startup or full-game execution.
Synthetic caller-owned structures provide all inputs. Shadows are explicitly
disabled; the additional shadow projection branch is excluded.

256 contexts cover facing-like and dense non-normalised model/camera matrices,
model/camera translations, model-centre offsets, three screen scales,
magnification, positive/zero perspective coefficients, both depth directions,
odd/even buffers and negative clip origins. Independently written C++ and Python
mathematics both match every output exactly:3,840 float fields and1,536 integer
fields. Output SHA256 is329536ce0e3e9dfc3d5de912724048bcdf7fb9bdd0237a8b74ab7af63e2c7471.
Original instructions/data and owner executables remain private.

## Owned contract

`cvr_projection.h` accepts already prepared camera/model matrices and positions.
It deliberately does not normalise matrix, light or animation coefficients.
The output is the floating basis/origin before fixed-point voxel preparation.

* The composed matrix is camera times model, scaled by the supplied model scale.
* Translation is camera times(model position minus camera position).
* Perspective, when transformed model-centre Z and perspective coefficient are
  positive, is1/(centreZ*coefficient+1); otherwise it is1. This does not choose
  an arbitrary camera distance or replace the supplied zoom.
* XY basis scales by magnification*screenScale*perspective. Translation is
  multiplied by that factor and rounded to float BEFORE adding screen offsets.
  Omitting this store caused three one-ULP mismatches in dense-matrix tests;
  the final owned implementation preserves it and matches all values exactly.
* Buffer origin is(floor(width/2)+0.5,floor(height/2)+0.5), plus supplied offsets
  and transformed translation. It is not centred on visible voxel bounds.
* Z maps the supplied near/far interval onto the supplied depth interval, with
  the caller's reverse-depth flag and model Z offset. No arbitrary32768+Z*16
  replacement is introduced.
* Clip fields are an origin and source extents, not guessed right/bottom
  coordinates. Original inclusive margins and floor conversion are preserved.

The bounded owned API checks finite inputs, valid scales/intervals/dimensions,
integer conversion ranges and output finiteness before committing output.
Twenty-five invalid cases verify transactional failure, alongside one exact
identity/buffer-centre control. The CLI probe uses the owned `ACPJ` wire format:
magic,count,forty little-endian floats, width/height/depthFlip and four signed
clip-origin/extent values; output is fifteen floats and six signed integers.

## Placement implications and remaining work

Instruction inspection of original Caviar blit0x617230 shows a256-square private
viewport placed around the caller's supplied centre. The normal map caller's
zoom0 centre offset is50px per axis. Its ordinary draw flags are1/3, whereas
silhouette-based centring is an explicit optional bit4 path. These final blit
facts have not been executed here. The current viewer's silhouette centring
and fitted foot anchor therefore remain diagnostic conventions until a scoped
integration binds the true original scene/camera state and final clipping.

Before claiming restored in-world placement, qualify the real Caviar defaults,
all scene offsets, object geometry/fixed-basis preparation, integer rounding and
final1x/2x blit against bounded original controls. Preserve source zoom/model
scale and supplied non-normalised animation/light coefficients. The API does
not implement shadows, terrain-slope camera selection, full component assembly,
unit actions or final whole-game fidelity. It is not wired into the map viewer.
Normal SDK probe/control builds are available privately; their native execution
and independent big-endian CI remain pending. No PR is published for this scope.
