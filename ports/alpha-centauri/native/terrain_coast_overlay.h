/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TERRAIN_COAST_OVERLAY_H
#define ALPHA_TERRAIN_COAST_OVERLAY_H
#include "terrain_coast_resources.h"
#include "terrain_ocean_detail.h"
#include "terrain_ground_plate.h"
namespace alpha {
struct TerrainCoastOverlayInput {
 TerrainMeshInput mesh;TerrainOceanLights light;
 uint8_t shore_mask[4];int32_t map_x,map_y;int fog_colour;
 bool source_coast_context_supplied=false;
};
// The source ordinary-land coast pass after existing ground pixels. Actual
// caller-enabled context and fog(-1/65) required. Prepared primary comes from
// original atlas crop280,79; secondary/lookup use explicit owner resources.
// Small surface<=256x256, pitch<=256. Reject aliases transactionally. Not full
// World visibility, water plates, OS allocation/palette or frontend rendering.
bool render_terrain_coast_overlay(const TerrainCoastOverlayInput&,const TerrainCoastMetadata&,const TerrainCoastResources&,const PreparedTerrainAtlas&,const TerrainTextureSurface&);
}
#endif
