/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_CVR_MAP_ANCHOR_H
#define ALPHA_CVR_MAP_ANCHOR_H
#include <stdint.h>
namespace alpha {
struct CvrMapScale {int32_t zoom,numerator,denominator;};
struct CvrMapMetrics {int32_t scaled50,tile_x,half_x,tile_y,half_y;};
// Numeric dimension contract; positive numerator selects the original ratio override.
bool cvr_map_metrics(const CvrMapScale&,CvrMapMetrics&);
struct CvrMapPlacement {
 int32_t width,height;bool flat;
 int32_t tile_left,tile_top,pixel_left,pixel_top;
 int32_t half_x,half_y,odd_x,even_x,odd_y,even_y;
};
struct CvrMapPoint {int32_t x,y;};
// Four resolved corner heights, not a raw cache containing unresolved -1 sentinels.
// Reproduces original visible-interval wrapping/fallback, not world-space nearest-only.
// Output must not overlap either input. No terrain height generation occurs here.
bool cvr_map_tile_origin(const CvrMapPlacement&,int32_t x,int32_t y,const int32_t corners[4],CvrMapPoint&);
struct CvrViewerUnitAnchor {
 CvrMapPoint caller,scene_centre;int32_t original_zoom;float magnification;
};
// Current flat viewer's two explicit 64x32 / 128x64 terrain diamond sizes.
// Map viewport coordinates only: the caller adds its own window/frame origin.
// Basic infantry/rover only; no slope camera, shadows, status markers or animation.
bool cvr_viewer_flat_anchor(unsigned viewer_zoom,int32_t tile_centre_x,int32_t tile_centre_y,bool infantry,CvrViewerUnitAnchor&);
}
#endif
