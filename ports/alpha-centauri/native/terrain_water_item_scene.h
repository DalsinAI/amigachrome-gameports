// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ALPHA_TERRAIN_WATER_ITEM_SCENE_H
#define ALPHA_TERRAIN_WATER_ITEM_SCENE_H
#include "terrain_sea_scene.h"
#include "terrain_water_sprites.h"
namespace alpha {
struct TerrainWaterItemResources {TerrainSeaSceneResource sea;WaterItemSprites items;WaterItemDrawRuntime draw;};
// Bounded owned-World normal scene with sea-platform and NO-CROP later water
// Draw3 items. Actual observation/late gate/using-faction are resolved from World;
// source CurrentFaction and ExpansionEnabled come from supplied scene runtime.
// Normal source-5/5/cache/height/identity gates remain, as do all required crop,
// highbit DrawDest, stale base repair and unsupported feature/import holds.
// Sea platform -> source water road/base-query boundary -> later item sprites.
// Water bit20 is removed ONLY for the qualified terrain prefix AFTER inspecting
// every late operation and actual base-query/repair gate; water has no road pixels.
// All resource/cache/clip/minimum authority is explicit. Nothing enables a
// frontend/native view. Original joined RAM proof is NOT whole USER32 execution.
// Fully transactional; original destination cannot overlap any input allocation.
TerrainSceneStatus render_world_water_item_scene(const OriginalWorld&,const Rules&,
 const UnitDefinitions&,int,int,const TerrainSceneRuntime&,const TerrainCoastMetadata&,
 const TerrainCoastResources&,const TerrainOceanLightTable&,const PreparedTerrainAtlas&,
 const TerrainWaterItemResources&,const TerrainTextureSurface&);
}
#endif
