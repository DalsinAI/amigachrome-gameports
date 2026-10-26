/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TERRAIN_SCENE_H
#define ALPHA_TERRAIN_SCENE_H
#include "terrain_visibility.h"
#include "terrain_caller_order.h"
#include "terrain_coast_overlay.h"
#include "terrain_water_patch.h"
namespace alpha {
enum class TerrainSceneStatus {rendered,missing_runtime,invalid_input,held_view,held_features,held_height,held_observation,out_of_memory,failed_stage};
struct TerrainSceneRuntime {
 TerrainVisibilityRuntime visibility;
 TerrainCallerRuntime caller;
 int32_t caller_x,caller_y,render_height,cache_source_zoom;
 int32_t source_scale_numerator,source_scale_denominator;
 uint32_t source_map_tiles_address,game_preferences;
 // The supported source receiver is distinct from main MapWin and has a fresh
 // active height/light cache. No stale/cache-null/main-window bypass is inferred.
 bool private_view_fresh_active_cache=false,source_identity_supplied=false,source_scale_supplied=false,supplied=false;
};
// Ordinary source flags0/1/0x20/0x21 plus independent river bit0x400 terrain path (sea-platform contexts held): observation/caller gate, genuine
// heights/lights/material dispatch, ground then coast OR deep/primary water.
// Source GameRules100/unexplored concealment overrides rainfall/rockiness and
// suppresses fungus/jungle/forest; retained ordinary height and coast topology are real.
// Fresh normal-scale25 zoom-5/5 cache profile only. No outline/hidden draw,
// special heights, grid/territory/sea-platform/actors/OS effects. Revealed
// jungle and land/ocean fungus use source unlit keyed layers; flag1 forest
// is lit and inserted between jungle and fungus using actual neighbour bits.
// Land farm textures use the source identity-selected variant, rainfall
// overlay and unrotated farm UVs; sea farms remain a typed hold.
// Roads/magtubes follow coast, using the separate late exploration gate,
// source base visibility and neighbour snapshots. Other effects sharing bit20
// (crop-yield and improvement/landmark sprites) remain explicit holds.
// Rivers use RAW centre/neighbour item bits, even in concealed observation,
// after coast and before the separate late visibility gate; source200 adds
// a second mask0 texture. Fog uses absolute even rows and preserves key9.
// Draw flag01000000 is a separate flat-colour polygon mode and remains held.
// Actual source tile allocation identity, edition-bound metadata/light table,
// and prepared owner textures are explicit inputs, never guessed authority.
// No original landmark-cache mutation, global side effects or OS palette load.
// Indexed RAM surface <=256x256. All failure is transactional; destination
// cannot alias any descriptor, World/rules/unit allocation or source resource.
TerrainSceneStatus render_world_ordinary_scene(const OriginalWorld&,const Rules&,const UnitDefinitions&,int x,int y,const TerrainSceneRuntime&,const TerrainCoastMetadata&,const TerrainCoastResources&,const TerrainOceanLightTable&,const PreparedTerrainAtlas&,const TerrainTextureSurface&);
}
#endif
