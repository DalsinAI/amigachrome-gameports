/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_CVR_SCENE_BLIT_H
#define ALPHA_CVR_SCENE_BLIT_H
#include <stddef.h>
#include <stdint.h>
namespace alpha {
class PcxImage;
struct CvrSceneRect {int32_t left,top,right,bottom;}; // Right/bottom exclusive.
struct CvrSceneRgbTarget {uint8_t* pixels;size_t capacity;uint32_t width,height,row_bytes;};
// Source-derived normal 1:1 contract only; full original OS/buffer wrapper UNRUN.
// Destination must not overlap source, palette or clip; rejection leaves it unchanged.
bool composite_cvr_scene_1x(const uint8_t* indices,size_t,const uint8_t* rgb,size_t,int32_t centre_x,int32_t centre_y,const CvrSceneRect&,CvrSceneRgbTarget);
bool cvr_basic_unit_marker(const PcxImage&,unsigned chassis,int32_t& x,int32_t& y);
// Input is veh_draw caller XY, not an assumed tile centre/foot. No zoom scaling.
bool cvr_unit_anchor_zero_zoom(int32_t x,int32_t y,int32_t marker_x,int32_t marker_y,int32_t& centre_x,int32_t& centre_y);
}
#endif
