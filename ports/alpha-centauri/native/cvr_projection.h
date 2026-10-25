/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_CVR_PROJECTION_H
#define ALPHA_CVR_PROJECTION_H
#include <stdint.h>
namespace alpha {
// Already prepared, finite camera/model coefficients: do not normalise them.
struct CvrProjectionInput {
 float camera[9],model[9],camera_position[3],model_position[3],model_centre[3];
 float model_scale,screen_scale,magnification,perspective;
 float near_z,far_z,depth_min,depth_max;
 float model_offset[3],screen_offset[2];
 uint32_t width,height;bool reverse_depth;
 int32_t clip_left,clip_top,clip_extent_x,clip_extent_y;
};
struct CvrProjection {
 float origin[3],basis[9],perspective_factor,model_pixel_scale,model_depth_scale;
 int32_t clip_left,clip_top,clip_right,clip_bottom,clip_width,clip_height;
};
// No shadow projection, geometry decoding, fixed-point conversion or final blit.
// Checked transactional output; output can alias input storage.
bool prepare_cvr_projection(const CvrProjectionInput&,CvrProjection&);
}
#endif
