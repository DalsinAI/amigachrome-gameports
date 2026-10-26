/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TERRAIN_FEATURE_LAYERS_H
#define ALPHA_TERRAIN_FEATURE_LAYERS_H
#include "terrain_ground_plate.h"
#include "terrain_world_masks.h"
#include "terrain_lighting.h"
namespace alpha {
struct TerrainFeatureInput {
 TerrainMeshInput mesh;TerrainWorldMasks masks;
 unsigned altitude;bool jungle,fungus,concealed,visible,source_context_supplied=false;
 bool forest=false;TerrainPointLights lights{};
};
// Ordinary source layer order: unlit keyed jungle, then source-enabled lit forest, then keyed/fog fungus;
// coast follows at the caller. Genuine owner prepared atlas only. Concealment
// suppresses both; ocean fungus starts at altitude2. No special height tables,
// farms/other improvements, actors, RGB palette or OS-loader authority is inferred.
// Surface<=256x256. Failure and all descriptor/atlas aliases are transactional.
bool render_terrain_feature_layers(const TerrainFeatureInput&,const PreparedTerrainAtlas&,const TerrainTextureSurface&);
}
#endif
