# Scene output alias correction — host-only

Separate correction from frozen scene candidate `9b2649d38543f8057263e2c5725d865363e64ff5`. No camera, projection, geometry, shading or raster equations change; no viewer/lab changes.

The flat-camera factory rejects overlapping projection/light output ranges and wrapping addresses before either write; exactly adjacent ranges remain valid.

Either rendered output-plane range must be disjoint from the other plane, renderer descriptor/lattice, projection input, world light, part descriptors, shade tables and all loaded-model allocations. Model ranges include descriptor, object/voxel/frame arrays and complete power-of-two group allocation slack. The new ownership-range query changes no parser or object layout. Forged/corrupted model descriptors and concurrent input mutation remain outside the API contract.

RGB copy rejects destination overlap with indexed source, palette and referenced clip before any writes. In-place copying is unsupported. Read-only inputs may overlap each other. The contiguous destination range includes row padding.

Host ASan/UBSan: 14,421 rejected alias calls per complete scene suite, partial first/last-byte overlaps on both planes, pointer wrap, renderer/lattice and every model allocation. Protected allocations, renderer state and ordinary outputs are checked unchanged. Two adjacent factory arrangements pass. Both independent source-default and supplied-original-state suites retain every original output byte: 16 scenes / 3,145,728 bytes, SHA256 `40dbf43dff5208607e780317b45966a47b46d8c6cfb57afb9e7826c3ed68c0a0`. Previous 96 invalid scene checks still pass. RGB: 75 independent copy controls plus ten new alias/wrap rejections; reader32, LOD40, group4 semantic/3 malformed regressions pass.

Normal SDK builds pass. Native and independent big-endian execution are UNRUN; no CI/PR publication or map integration. The final Windows copy wrapper remains unexecuted. Prior faction, anchor, shadow, animation and campaign limitations remain. Original data/reference bytes stay private.
