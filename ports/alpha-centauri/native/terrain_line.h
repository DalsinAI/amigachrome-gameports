/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TERRAIN_LINE_H
#define ALPHA_TERRAIN_LINE_H
#include "terrain_texture.h"
namespace alpha {
struct TerrainLineInput {int32_t x0,y0,x1,y1;unsigned colour;};
// Source RAM Buffer_line2 integer raster with strict positive-error diagonal
// steps, inclusive endpoints, degenerate point no-op, full-surface clipping.
// Source DIB dimensions equal surface dimensions. Bounded XY +/-4096; output
// cannot overlap either descriptor, and malformed input never writes.
bool draw_terrain_line(const TerrainLineInput&,const TerrainTextureSurface&);
}
#endif
