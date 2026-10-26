/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TERRAIN_GROUND_PLATE_H
#define ALPHA_TERRAIN_GROUND_PLATE_H
#include "terrain_mesh.h"
#include "terrain_lighting.h"
#include "terrain_dispatch.h"
#include "terrain_texture.h"
namespace alpha {
struct PreparedTerrainAtlas {const uint8_t* pixels;size_t bytes;unsigned width,height,pitch;};
struct TerrainGroundPlateInput {TerrainMeshInput mesh;TerrainPointLights light;TerrainDispatchContext material;};
// One ordinary land ground plate, not an entire tile/world scene. Input context
// must already come from qualified source caller/visibility/identity resolution.
// Atlas indices are already prepared (+10); RGB palette installation is outside
// this indexed stage. Each ground triangle is followed by its keyed moisture layer.
// Small offscreen surface <=256x256, pitch<=256. All failure is transactional;
// destination may not overlap descriptors or atlas. No water/coast/items/units.
bool render_terrain_ground_plate(const TerrainGroundPlateInput&,const PreparedTerrainAtlas&,const TerrainTextureSurface&);
}
#endif
