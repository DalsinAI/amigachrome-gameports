/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TERRAIN_MESH_H
#define ALPHA_TERRAIN_MESH_H
#include <stdint.h>
namespace alpha {
struct TerrainMeshInput {
 int32_t caller_x,caller_y;
 // Already resolved ordinary land height-cache values: centre, W,N,E,S.
 // Screen-space corner ordering is distinct from CVR model-space axes.
 int32_t height[5];
 unsigned viewer_zoom;
};
struct TerrainMeshPoint {int32_t x,y;uint8_t corner;};
struct TerrainMesh {TerrainMeshPoint triangles[4][3];};
// The four source ground triangles, before texture/visibility/light dispatch.
// NormalRenderScale25 at viewer zoom1/2, ordinary heights0..96 only. No world
// inference, rasterisation, clipping, UV/material selection or OS calls.
// Output commits transactionally and may overlap input storage.
bool build_terrain_mesh(const TerrainMeshInput&,TerrainMesh&);
}
#endif
