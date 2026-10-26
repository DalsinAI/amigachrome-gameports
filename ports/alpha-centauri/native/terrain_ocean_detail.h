/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TERRAIN_OCEAN_DETAIL_H
#define ALPHA_TERRAIN_OCEAN_DETAIL_H
#include "original_world.h"
namespace alpha {
struct TerrainOceanDetails {uint8_t value[4][4];};
struct TerrainOceanLightTable {const int32_t*by_detail;size_t count;bool supplied=false;};
struct TerrainOceanLights {int32_t point[4][4];};
// Original alt_get_ocean_detail effect, all4sectors/4vertices. Uses raw contour
// bytes, neighbour climate class and source wrap/edge rules. No fog/view/RNG.
// Output rejects overlap with every owned World allocation.
bool world_ocean_details(const OriginalWorld&,int x,int y,TerrainOceanDetails&);
// Requires actual edition-bound6861f0 table supplied explicitly; no guessed
// depth-to-colour gradient. Supported signed-light range is -127..127.
bool light_ocean_details(const TerrainOceanDetails&,const TerrainOceanLightTable&,TerrainOceanLights&);
}
#endif
