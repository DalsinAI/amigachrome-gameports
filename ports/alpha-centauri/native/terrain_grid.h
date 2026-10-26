/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TERRAIN_GRID_H
#define ALPHA_TERRAIN_GRID_H
#include "original_world.h"
#include "terrain_mesh.h"
#include "terrain_texture.h"
namespace alpha {
struct TerrainGridView {uint32_t more_preferences;bool enabled=false,supplied=false;};
struct TerrainGridPlan {uint8_t colour[2];bool supplied=false;};
// Original shared NW/NE edges; neighbour raw altitude, single source X wrap.
// MorePreferences800000 explicitly enables sea edges. No inferred viewer authority.
// Transactional output cannot alias context or any World-owned allocation.
bool world_terrain_grid(const OriginalWorld&,int,int,const TerrainGridView&,TerrainGridPlan&);
struct TerrainGridInput {TerrainMeshInput mesh;TerrainGridPlan plan;};
// Source order: ground, grid, plants, coast, rivers, late layers. RAM-only
// full-surface clip/DIB dimensions agree. Transactional indexed<=256 surface.
bool render_terrain_grid(const TerrainGridInput&,const TerrainTextureSurface&);
}
#endif
