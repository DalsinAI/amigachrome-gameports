/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_CVR_TERRAIN_NORMAL_H
#define ALPHA_CVR_TERRAIN_NORMAL_H
#include <stdint.h>
namespace alpha {
// Normal RenderScale25 facet-table coordinates, ordered west/south/east/north.
// Codes must already be resolved by the original map-height rules: this helper
// does not infer slopes, coast state, visibility or neighbouring terrain.
// Output commits only on success and may overlap the input codes.
bool cvr_terrain_normal(const uint8_t relative[4],float out[3]);
// Camera derived from an explicit finite nonzero terrain normal. No map inference.
// Camera and heading outputs must be disjoint and commit together.
bool cvr_terrain_camera(const float normal[3],float camera[9],float* heading);
}
#endif
