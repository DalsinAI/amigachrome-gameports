/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_CVR_RASTER_H
#define ALPHA_CVR_RASTER_H
#include <stddef.h>
#include <stdint.h>
namespace alpha {
/* Projected coordinates, not CVR object coordinates. xy are signed 24.8;
 * z is a wrapping unsigned 16.16 accumulator, with the upper half as depth.
 * Camera, normal-to-shade selection and palette remapping are caller-owned. */
struct CvrRasterPoint { int32_t x,y;uint32_t z;uint16_t normal;uint8_t color; };
struct CvrRasterClip { int32_t left,top,right,bottom; };
struct CvrRasterSurface {
 uint8_t* pixels;uint16_t* depths;size_t capacity;
 uint32_t width,height,stride;
};
/* Validated 1x1/2x2 pixel stage only. Quad clips include right/bottom and
 * clamp anchors at left-1/top-1 onto the edge; single pixels use exclusive
 * right/bottom. These are the original two raster profiles, not a generic
 * geometric rectangle convention. Surface must encompass the entire clip.
 * Strictly smaller depth wins; ties retain the existing pixel. Any invalid
 * input fails before writes. Buffers/tables/points must not overlap. */
bool raster_cvr_points(const CvrRasterPoint* points,size_t count,
 const uint8_t* normal_shades,size_t shade_count,
 const uint8_t* palette,size_t palette_count,
 const CvrRasterClip& clip,bool quad,const CvrRasterSurface& target);
}
#endif
