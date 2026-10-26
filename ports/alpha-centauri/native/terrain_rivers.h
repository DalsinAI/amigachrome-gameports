/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TERRAIN_RIVERS_H
#define ALPHA_TERRAIN_RIVERS_H
#include "terrain_ground_plate.h"
namespace alpha {
struct TerrainRiverInput {TerrainMeshInput mesh;TerrainPointLights lights;unsigned neighbour_mask;int fog_colour;bool river=false,source=false,source_context_supplied=false;};
// Source raw river bit80, four diagonal neighbour bits; source bit200 adds
// mask0 after the main patch. Runs after coast and before late exploration gate.
// Fog preserves raw key9; prepared owner atlas and clamped land lights[-4,4].
// Transactional indexed RAM surface<=256x256, no OS/import/scale callbacks.
bool render_terrain_rivers(const TerrainRiverInput&,const PreparedTerrainAtlas&,const TerrainTextureSurface&);
}
#endif
