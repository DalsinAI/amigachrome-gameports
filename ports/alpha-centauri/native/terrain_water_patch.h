/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TERRAIN_WATER_PATCH_H
#define ALPHA_TERRAIN_WATER_PATCH_H
#include "terrain_ocean_detail.h"
#include "terrain_ground_plate.h"
namespace alpha {
struct TerrainWaterPatchContext {
 int32_t caller_x,caller_y;unsigned viewer_zoom;int fog_colour;
 int minimum_light,maximum_light;uint32_t source_draw_flags;
 bool source_context_supplied=false,source_bounds_supplied=false;
};
struct TerrainWaterPlan {TerrainTextureVertex vertex[4][4];bool deep[4];int fog_colour;};
// Closed ordinary ocean-pass context from46452c..464720: source normal-scale25,
// zoom1/2, ocean tile, neither source draw flag82000000 enabled, fog-1/65.
// Original runtime primary-light bounds must be supplied explicitly: source
// initial/reset is[-256,256], land-only changes[-4,4]; neither is inferred.
// Explicit actual edition80-entry source light table must remain in0..37.
// Transactional outputs reject overlap with every owned World/input allocation.
bool build_world_water_plan(const OriginalWorld&,int x,int y,const TerrainWaterPatchContext&,const TerrainOceanLightTable&,TerrainWaterPlan&);
// Actual source sector order, deep base iff any pointlight>=16, then threshold16
// primary. Prepared atlas crops280,136 and280,79. Surface<=256x256,pitch<=256.
// This bounded water pass does not resolve human visibility or execute the full
// MapWin/OS renderer, palette installation, item layers or frontend actions.
bool render_world_water_patch(const OriginalWorld&,int x,int y,const TerrainWaterPatchContext&,const TerrainOceanLightTable&,const PreparedTerrainAtlas&,const TerrainTextureSurface&);
}
#endif
