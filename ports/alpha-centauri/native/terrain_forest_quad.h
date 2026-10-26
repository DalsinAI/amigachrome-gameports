/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TERRAIN_FOREST_QUAD_H
#define ALPHA_TERRAIN_FOREST_QUAD_H
#include "terrain_texture.h"
namespace alpha {
struct TerrainForestVertex {int32_t x,y,u,v,light;};
struct TerrainForestQuadInput {TerrainForestVertex vertex[4];const uint8_t*texels;size_t texture_bytes;unsigned texture_width,texture_height;};
// Source shaded keyed quad: four edge-interpolated lights; raw key comparison
// precedes indexed shade addition. Original Gouraud edge half-texel bias.
// Already source-clamped light values -256..256; no invented fog or threshold.
// XY +/-4096, UV0..255; sampled UV and aliases reject transactionally.
bool draw_terrain_forest_quad(const TerrainForestQuadInput&,const TerrainTextureSurface&,int key=9);
}
#endif
