/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TERRAIN_LIGHTING_H
#define ALPHA_TERRAIN_LIGHTING_H
#include "cvr_terrain_context.h"
namespace alpha {
struct TerrainBrightness { int8_t triangle[4]; };
struct TerrainPointLights { int8_t point[5]; };
// Relative corners W,N,E,S in1..7; mathematical original integer table effect.
bool terrain_brightness(const uint8_t relative[4],TerrainBrightness&);
// Ordinary terrain centre/W/N/E/S light. Special height/light features remain
// unsupported; sea/map-edge neighbours contribute zero brightness. Read-only,
// transactional output cannot overlap world-owned storage.
bool world_terrain_lights(const OriginalWorld&,int x,int y,TerrainPointLights&);
}
#endif
