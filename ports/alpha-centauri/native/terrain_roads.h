/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TERRAIN_ROADS_H
#define ALPHA_TERRAIN_ROADS_H
#include "original_world.h"
#include "terrain_ground_plate.h"
namespace alpha {
struct TerrainRoadView {int32_t viewer;uint32_t displayed_items;bool supplied=false;};
struct TerrainRoadPlan {uint8_t direction[8],isolated;bool source_context_supplied=false,source_late_layers_visible=false;};
enum class TerrainRoadStatus {resolved,invalid_input,held_base_repair};
// Original late-layer visibility gate first; then actual source base_at bits/first matching record, viewer-visible bases and
// raw neighbour snapshots. 0=no draw,1=road,2=tube; direction NE,E,SE,S,SW,W,NW,N.
// A stale base bit is held: no repair/log/OS callback is fabricated. This query
// does not qualify the other effects sharing source draw bit0x20.
TerrainRoadStatus world_terrain_roads(const OriginalWorld&,int,int,const TerrainRoadView&,TerrainRoadPlan&);
struct TerrainRoadLayerInput {TerrainMeshInput mesh;TerrainPointLights lights;TerrainRoadPlan plan;};
// Source-keyed shaded quads in source neighbour order, then isolated fallback.
// Already source-clamped land lights[-4,4], genuine prepared owner atlas.
// Indexed surface<=256x256. Failure/descriptor/atlas aliases are transactional.
bool render_terrain_roads(const TerrainRoadLayerInput&,const PreparedTerrainAtlas&,const TerrainTextureSurface&);
}
#endif
