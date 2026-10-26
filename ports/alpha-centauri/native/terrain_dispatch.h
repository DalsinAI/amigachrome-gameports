/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TERRAIN_DISPATCH_H
#define ALPHA_TERRAIN_DISPATCH_H
#include <stdint.h>
namespace alpha {
struct TerrainDispatchContext {
 // Actual source render-context identity, not a pointer to dereference. The
 // caller must establish provenance; no save pointer/gameplay-RNG fallback.
 uint32_t original_tile_address;
 uint8_t rockiness,moisture,transition_mask;
 bool dunes,farm,farm_rendering,visible;
 bool source_address_supplied=false;
};
struct TerrainUv {int32_t u,v;};
struct TerrainDispatch {
 uint16_t atlas_x,atlas_y,overlay_x,overlay_y;
 uint8_t variant,rotation,variation;
 int16_t fog_colour;
 bool overlay;
 TerrainUv uv[4][3];
};
// Ordinary land plate + optional keyed moisture overlay. Explicit source
// context only (unavailable identity rejects): no world visibility/height/coast inference or scene rendering.
// The original two exceptional transition metadata cases are retained.
bool terrain_dispatch(const TerrainDispatchContext&,TerrainDispatch&);
}
#endif
