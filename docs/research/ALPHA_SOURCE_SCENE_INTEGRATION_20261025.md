# Source-scene frontend integration candidate — host-only

This separate candidate descends from frozen map-anchor0c7b864 and preserves the fitted assembly and all recovered scene dependencies unchanged. It wires CvrSourceUnit into the real original-world viewer and frontend build, rather than another diagnostic window. Normal SDK symbol inspection confirms CvrSourceUnit::draw and CvrSceneRenderer::render are linked; CvrUnitAssembly::draw is absent. This is a read-only frontend candidate, not native/deployed or a completed game.

## Rendering contract

Static frame0 basic infantry is owner VI plus dedicated Viptr00 at the source append vector(-400,200,0). The basic Explore rover uses the original ten-part append order with shared original depth and faction shade tables on panels2/4. Separate LOD sets are selected at the actual source magnifications for current viewer zoom1/2 (source zoom-5/+5). The recovered camera/defaults, mathematical projection, original groups/seeds, fixed conversion, procedural shading and original-smaller-depth raster are reused directly. There is no silhouette centring, guessed32768+depth, tile-foot fitting or arbitrary enlargement.

Each assembly caches at most sixteen256-square indexed snapshots (1MiB), not giant RGB animation atlases. The target copy uses the qualified caller anchor and normal1:1 clip/key contract; game index9 stays transparent. A source global palette is required; rover additionally requires explicit faction metadata. Palette/tint changes invalidate all facing/zoom caches; equal settings retain them. Missing metadata does not silently fall back to raw colours. Custom gamma/system palettes and other faction/model variants remain unsupported. Alias checks protect the assembly, model storage, palettes and cached snapshots.

## Conservative terrain support

The source normal RenderScale25 path is explicit. Sea returns a flat camera/zero height. Land support requires all sixteen relevant corner-neighbours to be valid land in un-clamped elevation bands3..6, no special height-feature lookups, and all four resolved relative-corner codes equal and within1..7. Slopes, coast/edge uncertainty and special-height contexts keep the unsupported-scene label. This derives all four corners; it does not infer flatness from the centre climate.

The owner's Explore rover24,24 is a symmetric hilltop: unchanged original recursive altitude/relative helpers, facet initializer, vector math and camera produce relative(1,1,1,1), normalized normal(0,0,0.9999999404), and a camera BYTE-IDENTICAL to the qualified flat camera. At the two actual viewer zooms its original corner heights are4/8, matching the owned decision. All seven uniform relative-corner profiles were separately executed through these original helpers and yield exactly the same flat camera. No callback, Windows import or full PE/game execution was substituted.

The map remains the explicitly flat terrain presentation. Relative caller anchors are used; original elevation/topology, coast joins, shadows and movement animation are not restored. Basic infantry is wired for supported contexts, but Explore's coastal infantry does NOT pass this conservative context guard and retains a label. No old fitted renderer is used as a fallback. Supported context is independent of design eligibility: other weapons, reactors, armour, abilities, expansion IDs and variants still fail the original bounded profile checks.

## Verification

Host ASan/UBSan compares32 complete scenes (both recipes, two actual viewer/source zooms, eight facings) with unchanged original no-shadow scene helper639e60 and its real dependencies: all6,291,456 indexed/depth bytes match, SHA2561eeed34dbaa128c3575b916956823b485918bfbd1f2ef7c2a50563062f908bb8. Ninety independent destination-coordinate RGB/border/cache composites pass. An alternate faction tint actually changes pixels; restoring the original tint exactly restores pixels and forces fresh caches. Invalid/self-alias input checks pass. Twenty-seven synthetic terrain guard cases and fresh1,000 map/22 dimension controls pass; the frozen6,044 original/viewer map-boundary proof remains applicable to the unchanged anchor helper.

Actual original altitude/camera proof uses only bounded helpers and real dependencies, with private owner/synthetic inputs. The original final17230/USER32 buffer wrapper remains unexecuted: owned numeric clipping/compositing is tested independently. Original code/assets/tables are private and never uploaded.

## Remaining gates

The final normal SDK frontend and unit/context probes build. Native CLI execution, native integrated Explore rendering/zoom/selection/return/audio/normal exit, performance and independent big-endian CI are UNRUN. No lab, browser, instance, original saves, installed SDK or root gameplay branch was touched. No PR publication or merge is authorised by this handoff. Root must reconcile the dependency stack/current main, review the actual final scope/head, then qualify native/CI before publication or deployment. The user watch retains exclusive lab ownership.
