/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TERRAIN_WATER_TEXTURE_H
#define ALPHA_TERRAIN_WATER_TEXTURE_H
#include "terrain_texture.h"
namespace alpha {
struct TerrainWaterTextureInput {TerrainTextureVertex vertex[4];const uint8_t*texels;size_t texture_bytes;unsigned width,height;};
// Source quad draw2/3: caller supplies original runtime clamp bounds; fog
// retains original odd-row prefetch
// lag. Threshold draw624670/61ac60 does NOT clamp light; signed shade<threshold
// writes, with all-vertices>=threshold early skip. Fog even rows fill the span
// only after this early test; odd threshold rows do not have prefetch lag.
// XY+/-4096, UV0..255, threshold/light+/-127, texture<=256x256. Transactional
// sampled-coordinate/alias validation, source PC24/truncate integer emulation.
bool draw_terrain_water_primary(const TerrainWaterTextureInput&,const TerrainTextureSurface&,int minimum_light,int maximum_light,int fog=-1);
bool draw_terrain_water_threshold(const TerrainWaterTextureInput&,const TerrainTextureSurface&,int threshold,int fog=-1);
}
#endif
