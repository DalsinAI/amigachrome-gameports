/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TERRAIN_BASE_GRID_H
#define ALPHA_TERRAIN_BASE_GRID_H
#include "original_world.h"
#include "terrain_mesh.h"
#include "terrain_texture.h"
namespace alpha {
struct TerrainBaseGridContext {int32_t viewer,cursor_x,cursor_y;bool supplied=false;};
struct TerrainBaseGridPlan {uint8_t edges;bool late_visible=false,supplied=false;};
// Source preference200: raw val3 low3 using-faction filtered by viewer diplo8,
// unioned with cursor map-range<3. Raw diagonal neighbours NE,SE,SW,NW.
// The late raw exploration gate is distinct from terrain reveal. Actual cursor
// must be supplied and on-map/parity-valid; no save/default cursor is inferred.
// Output cannot overlap context or any World allocation; failures never write.
bool world_terrain_base_grid(const OriginalWorld&,int,int,const TerrainBaseGridContext&,TerrainBaseGridPlan&);
struct TerrainBaseGridInput {TerrainMeshInput mesh;TerrainBaseGridPlan plan;};
// Source249 lines after rivers and before roads. Indexed RAM<=256, no OS calls.
// Inclusive line endpoints and alias/late-failure rejection are transactional.
bool render_terrain_base_grid(const TerrainBaseGridInput&,const TerrainTextureSurface&);
}
#endif
