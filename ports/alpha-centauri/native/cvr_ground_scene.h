/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_CVR_GROUND_SCENE_H
#define ALPHA_CVR_GROUND_SCENE_H
#include "cvr_projection.h"
namespace alpha {
enum class CvrGroundProfile { basicInfantry, basicRover };
// Source NormalRenderScale25 caller numeric contract only. Resolved corner
// codes are not a terrain mesh, visibility decision or permission to draw.
// Basic infantry retains the original flat-normal override. Outputs must be
// disjoint; both commit together. No world coordinates or OS copy performed.
bool prepare_cvr_ground_scene(const uint8_t relative[4],CvrGroundProfile,unsigned facing,float magnification,CvrProjectionInput&,float world_light[3]);
}
#endif
