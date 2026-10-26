/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TERRAIN_KEYED_QUAD_H
#define ALPHA_TERRAIN_KEYED_QUAD_H
#include "terrain_texture.h"
namespace alpha {
struct TerrainKeyedPoint {int32_t x,y,u,v;};
struct TerrainKeyedQuadInput {TerrainKeyedPoint vertex[4];const uint8_t*texels;size_t texture_bytes;unsigned width,height;};
// Original unlit keyed quad61a760/6239e0. Test the raw indexed key before
// writing. Fog even absolute rows replace only non-key pixels; odd rows keep
// original texels. No light addition or thresholded-light semantics. Clockwise
// polygon, XY+/-4096, UV0..255, transactional sampled-UV/alias rejection.
bool draw_terrain_keyed_quad(const TerrainKeyedQuadInput&,const TerrainTextureSurface&,int key,int fog=-1);
}
#endif
