# Terrain index preparation, fog and keyed composition

This child of the bounded visible triangle stage recovers actual opaque fog,
transparent and transparent-fog sampling effects. It also prepares decoded
terrain image indices/palette as observed in the original loader. It does not
select world materials or enable a full map renderer.

## Original RAM paths and observed differences

Private unchanged Texture_draw_3 (6233c0), Texture_draw_trans_4 (61c090) and
Texture_draw_trans_5 (622db0) executed their genuine RAM-backed Buffer acquire
and release paths. Each mode matches113controlled triangles /462,848pixels,
including absolute-row clipping, nonuniform light, key pixels and winding.
The Buffer locks balance to zero. Physical surfaces and callbacks were not
substituted or executed. Original instructions/assets are not included here.

Opaque fog writes the supplied colour (terrain caller uses65) to even absolute
surface rows. Odd rows use the original opaque-fog prefetch sequence: the first
visible texel repeats, subsequent texture samples lag one pixel, and light still
advances at every pixel. This behavior is distinct from ordinary opaque drawing.
Initially applying the ordinary texture sequence produced11,096different pixels
in the113case corpus. The retained first failing image had233differences.
No tolerance or altered expected colour hides this difference.

Transparent drawing tests the raw prepared texel against the key before adding
light. Transparent fog tests the same key on both row types; even rows write
fog only for non-key texels. These two paths do not have opaque-fog's repeated
first-sample behavior. The existing indexed draw fixture retains its exact
SHA256 on both architectures after this refactor.

## Actual terrain preparation

Original load_terrain (451c50) calls Buffer_load_pcx with palette pointer0,
index offset10 and palette count236. The bounded private continuation of
Buffer_load_pcx_2 starts at5e27fd, after header/dimension/allocation checks.
It executes original RAM acquisition, RLE decode, unchanged CRTmemset6465f0,
palette copy, release and normal function return. Header parsing, allocation,
file IO, virtual surface setup and physical palette installation are not claimed.

Every786,432pixel of the owner's1024x768 TEXTURE.PCX matches rawIndex+10 modulo
256. Source RGB entries0..235 become buffer palette entries10..245, stored BGR0
in the original DIB. Consequently raw255 maps to prepared transparent key9.
There is no nearest-palette remap in this bounded loader profile; destination
entries outside10..245 retain previous contents.

The owned preparation API accepts decoded indexed bytes and RGB triplets. It
performs the exact offset and palette placement, preserving other destination
palette entries. Public tests cover all256indices, all768palette bytes and nine
transactional boundary/partial-alias rejections. This is an RGB data adapter,
not installation of the original global/OS palette or a PCX decoder replacement.

## Qualification and remaining dependencies

Host ASan/UBSan and independently compiled big-endian Linux68040 tests execute
339composition cases (1,388,544pixels),12alias/boundary/invalid-mode controls,
partial input and the full visible-stage regression. Material preparation has
an independent integer oracle. Both normal SDK probes build. Actual AmigaOS,
frontend/UI, physical surface and final required CI execution remain unrun.
All86parent workflow commands are retained, with four new commands.

Original-world material selection, triangle UV rotation, point-light averaging,
fog visibility, coast masks and layered water/terrain overlays remain unresolved.
MapWin_get_point_light (470790) additionally depends on original brighting and
terrain-feature lookup data; its numeric closure is not inferred from the pixel
stage. No world dispatch callbacks, synthetic replacement imagery or full-scene
claims are introduced. The user-owned running preview is untouched.
