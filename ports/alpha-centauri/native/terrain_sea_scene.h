// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ALPHA_TERRAIN_SEA_SCENE_H
#define ALPHA_TERRAIN_SEA_SCENE_H
#include "terrain_scene.h"
#include "terrain_sea_platform.h"
namespace alpha {
struct TerrainSeaSceneResource {
 SpritePixelSource pixels;
 SpriteRect clip;
 std::int32_t minimum_width,minimum_height;
 bool coherent_source_cache_supplied,prepared_owner_pixels_supplied;
};
// Bounded ordinary scene + explicit acquired-RAM sea-platform layer. All normal
// parent cache/identity/height/view gates remain. World observation and distinct
// late exploration gate are resolved from owned World with the supplied runtime.
// For WATER ONLY bit1 solely dispatches the late sea sprite: the unchanged parent
// renders that prefix with bit1 disabled, then this routine appends the sprite
// after river/base-work grid and before roads. Land is delegated unchanged.
// Sea+bit20 remains held for later crop_yield callbacks; missing sprite/minimum/
// coherent scale-cache authority is missing_runtime. Only source zoom -5/5.
// Original USER32/full Sprite_draw3 return is NOT executed/qualified here.
// Transactional, no frontend enabling or native/physical palette claim.
TerrainSceneStatus render_world_scene_with_sea_platform(const OriginalWorld&,
 const Rules&,const UnitDefinitions&,int,int,const TerrainSceneRuntime&,
 const TerrainCoastMetadata&,const TerrainCoastResources&,const TerrainOceanLightTable&,
 const PreparedTerrainAtlas&,const TerrainSeaSceneResource&,const TerrainTextureSurface&);
}
#endif
