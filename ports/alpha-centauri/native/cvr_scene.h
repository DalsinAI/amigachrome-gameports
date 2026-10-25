/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_CVR_SCENE_H
#define ALPHA_CVR_SCENE_H
#include "cvr_model.h"
#include "cvr_projection.h"
#include "cvr_raster.h"
namespace alpha {
// Source flat-terrain camera/facing/default numeric profile; no shadow/OS state.
bool prepare_cvr_flat_scene(unsigned facing,float magnification,CvrProjectionInput&,float world_light[3]);
struct CvrScenePart {const CvrModel* model;const uint8_t* shade_table;size_t shade_bytes;float offset[3];uint32_t frame;};
class CvrSceneRenderer {
public:
 CvrSceneRenderer();~CvrSceneRenderer();
 // Explicit prepared coefficients, 256-square no-shadow private viewport only.
 // Clears to transparent index9/depth65535, then commits transactionally.
 bool render(const CvrScenePart*,unsigned,const CvrProjectionInput&,const float world_light[3],CvrRasterSurface);
private:
 CvrSceneRenderer(const CvrSceneRenderer&)=delete;CvrSceneRenderer&operator=(const CvrSceneRenderer&)=delete;
 uint8_t* lattice_;
};
}
#endif
