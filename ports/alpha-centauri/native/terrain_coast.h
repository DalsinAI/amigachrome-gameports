/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TERRAIN_COAST_H
#define ALPHA_TERRAIN_COAST_H
#include "terrain_texture.h"
namespace alpha {
struct TerrainCoastInput {
 TerrainTextureVertex vertex[4];
 const uint8_t*texels;size_t texture_bytes;
 const uint8_t*mask;size_t mask_bytes;
 const uint8_t*lookup;size_t lookup_bytes;
 unsigned width,height;
 int fog_colour;
};
// Original four-vertex coast pixel effect: lookup[mask]0 preserves destination,
// 1 selects textured/signed-light pixel, other values select constant palette
// indices. Fog stripes preserve zero-mask pixels. Lookup/palette/UV/light source
// preparation is deliberately caller-supplied, not invented shore colours.
// XY +/-4096, UV0..255, light +/-127 (not clamped), texture <=256x256.
// All validation is transactional; destination cannot overlap any source data.
bool draw_terrain_coast(const TerrainCoastInput&,const TerrainTextureSurface&);
}
#endif
