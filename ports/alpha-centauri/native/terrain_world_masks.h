/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TERRAIN_WORLD_MASKS_H
#define ALPHA_TERRAIN_WORLD_MASKS_H
#include "original_world.h"
namespace alpha {
struct TerrainMaskView {uint32_t displayed_items;bool supplied=false;};
struct TerrainWorldMasks {uint8_t coast[4],roughness,fungus,jungle;};
// Real neighbour topology for the original ordinary terrain branch. The caller
// must resolve centre displayed-items/visibility; this query does not invent a
// viewer/fog policy. No height, material, RNG, palette or pixel effects occur.
// Transactional output cannot overlap context or any world-owned allocation.
bool world_terrain_masks(const OriginalWorld&,int x,int y,const TerrainMaskView&,TerrainWorldMasks&);
}
#endif
