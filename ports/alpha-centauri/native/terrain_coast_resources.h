/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TERRAIN_COAST_RESOURCES_H
#define ALPHA_TERRAIN_COAST_RESOURCES_H
#include "terrain_mesh.h"
#include "terrain_dispatch.h"
#include <stddef.h>
namespace alpha {
// Edition-bound source metadata. These private class/rule tables must be
// supplied with verified provenance; no guessed/generated shore colours.
struct TerrainCoastMetadata {
 const uint8_t* classes;size_t class_bytes;
 const uint8_t* rules;size_t rule_bytes;
 bool supplied=false;
};
struct TerrainCoastResources {uint8_t mask[56*56],palette[6];};
// Raw decoded RAINFALL.PCX. The supported first mask crop contains no original
// transparent pixels; transparent crop inputs reject rather than guessing the
// source OS allocation's initial contents. Colours are original indices+10.
bool prepare_terrain_coast_resources(const uint8_t*rain,size_t bytes,unsigned width,unsigned height,const TerrainCoastMetadata&,TerrainCoastResources&);
struct TerrainCoastSector {int32_t xy[4][2];TerrainUv uv[4];uint8_t lookup[53];};
// Actual ordinary-ground mesh-to-coast caller: W,N,E,S sectors, source 27/28
// UV seams, signed midpoint floor. Does not resolve ocean-detail point lights.
bool prepare_terrain_coast_sector(const TerrainMesh&,unsigned sector,uint8_t shore_mask,int32_t map_x,int32_t map_y,const TerrainCoastMetadata&,const TerrainCoastResources&,TerrainCoastSector&);
}
#endif
