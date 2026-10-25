# Original map-to-unit placement contract — host-only

This separate candidate descends from frozen alias correction a368655. It changes no previous scene renderer, palette, model, projection or viewer source.

## Important correction to the earlier marker interpretation

The original veh_draw normal-model path preserves original caller X/Y in EBX/ESI, then adjusts stack X/Y by Units.pcx marker offsets for status/flag overlays. Its call to vehdraw_draw passes the preserved EBX/ESI, not those adjusted stack values. The original instructions at 559865/55986a/55986c and 559881/55988e/559890 distinguish these values; 55995b/55995c push the preserved Y/X into the original model draw. The original model draw then adds scaled50 to both original caller coordinates before final copy.

Therefore do NOT use the frozen marker-plus50 arithmetic helper as the model anchor. The observed infantry13,12 / rover14,14 markers remain real metadata, but belong to overlays. Previous candidates stay frozen; this contract corrects their interpretation explicitly. Whole veh_draw/model assembly/final Windows copy paths were inspected, not executed here.

## Original numeric path actually executed

Unchanged MapWin_tile_to_pixel462f00 calls unchanged MapWin_get_alt46fe70 with four actual cached signed corner heights. 1,000 explicit land-cache/visible-interval/wrapped/flat contexts match both the independent Python oracle and owned C++ exactly. The original height generator recursion is not invoked: all four selected cache heights are already resolved. The mean uses signed floor division by4, including negative values.

The complete unchanged MapWin_calculate_dimensions462980 also executes22 normal zoom/explicit-ratio contexts. Caller-owned object layout and metadata provide a bounded viewport; last font/scale/zoom fields select real branches requiring no font/window callback. No callback or import was replaced. Original tile-dimension bytes match the owned metrics helper. No full PE startup or game runs.

Original dimensions derive unit=max(1,trunc((scaled50+1)/4)), with halfX=4unit and halfY=2unit. At original zoom0 scaled50=50, dimensions96x48 and half sizes48x24. The coordinate helper returns pixelLeft+(relativeX-1)*halfX, pixelTop+(tileY-top-1)*halfY-floor(sum(cornerHeights)/4). Its visible-interval wrapping takes precedence over nearest-wrap fallback; the fallback even occurs for flat maps outside the interval. This differs from blindly applying nearest-X everywhere.

## Binding the current flat viewer without silhouette fitting

Original terrain generation shifts its central mesh origin by(+halfX,+halfY) from tile_to_pixel. MapWin_gen_tile/MapWin_draw_vehicles_2 subtract halfY before the unit caller. Normal model center is caller+(scaled50,scaled50). Relative to a flat terrain diamond's central origin the scene center is therefore (scaled50-halfX, scaled50-2halfY). At original zoom0 this is(+2,+2).

Current viewer terrain diamonds64x32 at viewer zoom1 and128x64 at zoom2 match original zoom-5 and+5 respectively. Those profiles give:

| Viewer zoom | Original zoom | scaled50 | halfX / halfY | Caller from tile centre | Scene centre from tile centre |
|---|---:|---:|---|---|---|
|1|-5|34|32 /16|(-32,-32)|(+2,+2)|
|2|5|65|64 /32|(-64,-64)|(+1,+1)|

The new flat-viewer anchor computes source camera magnification .07*(originalZoom+16)/16, then the source basic-infantry1.25 multiplier (basic rover remains1). Window/frame12,70 offsets remain caller presentation coordinates and are not baked into this mathematical contract. No image bounds, voxel silhouette or guessed feet are used.

6,044 actual original coordinate-helper cases verify this binding over every currently visible flat tile for the two viewer sizes, twelve camera/seam locations and explicit viewport intervals covering the viewer's draw margin. The first local fixture mistakenly put the source interval's origin one half-tile out; a second insufficient interval clipped marginal seam tiles. Corrected explicit viewport state passes; no production formula was changed to fit a model image. This establishes flat-geometry correspondence, not the complete original MapWindow camera/window state.

## Limits

The current viewer draws flat texture diamonds; original slopes/coasts/terrain corner geometry are not restored. A rover on a non-flat land plane needs original relative-altitude terrain normals/camera, still unsupported by this flat-only binding. Basic infantry's original flat-camera exception is separate. Height generation, viewport selection policy, optional source ratio/downsampling flags, shadows, movement animation, faction variants and final Windows buffer-copy wrappers remain outside this proof. Full scenes at the newly related zooms are not requalified by these coordinate controls. No native/independent big-endian execution, map wiring, lab access or publication occurred. Normal SDK build only; original bytes/decompile/assets stay private.
