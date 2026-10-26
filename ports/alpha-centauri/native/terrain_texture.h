/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TERRAIN_TEXTURE_H
#define ALPHA_TERRAIN_TEXTURE_H
#include <stddef.h>
#include <stdint.h>
namespace alpha {
struct TerrainTextureVertex { int32_t x,y,u,v,light; };
struct TerrainTextureInput {
 TerrainTextureVertex vertex[3];
 const uint8_t* texels; size_t texture_bytes;
 unsigned texture_width,texture_height;
};
struct TerrainTextureSurface {
 uint8_t* pixels; size_t bytes;
 unsigned width,height,pitch;
};
// Bounded indexed three-vertex scan conversion, not material/visibility dispatch.
// XY +/-4096, UV 0..255, light clamps to -4..4 on an owned copy. Clockwise
// winding; half-open surface clipping. Original PC24/truncate gradient effects
// are emulated with integers without changing the caller's floating state.
// Invalid sampled coordinates/aliases fail before any destination writes.
bool draw_terrain_texture(const TerrainTextureInput&,const TerrainTextureSurface&);
// Original draw_3 opaque fog stripe: even absolute surface rows use colour,
// odd rows repeat the first visible texel before advancing the original texture. -1 selects ordinary drawing.
bool draw_terrain_fog_texture(const TerrainTextureInput&,const TerrainTextureSurface&,int colour);
// Indexed key is tested before signed-light addition, including fog rows.
bool draw_terrain_masked_texture(const TerrainTextureInput&,const TerrainTextureSurface&,int key,int fog_colour=-1);
}
#endif
