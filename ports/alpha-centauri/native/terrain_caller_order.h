/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TERRAIN_CALLER_ORDER_H
#define ALPHA_TERRAIN_CALLER_ORDER_H
#include "terrain_observation.h"
namespace alpha {
enum class TerrainCallerStatus {resolved,missing_runtime,invalid_input,held_draw_mode};
enum class TerrainCallerPath {outline_only,terrain_hidden,land_then_coast,water};
struct TerrainCallerRuntime {
 int32_t outline_parameter,minimum_light,maximum_light;
 bool supplied=false;
};
struct TerrainCallerOrder {
 TerrainCallerPath path;
 int32_t pass_minimum_light,pass_maximum_light,exit_minimum_light,exit_maximum_light;
 bool land_clamp_before_observation;
};
// Original numeric caller gates, both editions. outline_parameter is the actual
// source argument a5, not a guessed named option. Incoming global light bounds
// must be supplied: ocean inherits them, land first changes them to [-4,4].
// Source eventual normal exits reset [-256,256]. No global/World mutation.
// Resolved ordering is NOT pixel rendering or visibility resolution: land
// ground precedes coast, ocean deep/threshold water precedes later item layers.
// Only ordinary flags0/1/0x20/0x21 are admitted to this numeric gate; other modes remain held unless the
// source outline/terrain-hidden gate already excludes terrain. Output transactionally
// rejects input/World aliases. No source helper, actor, cache or OS callback.
TerrainCallerStatus plan_world_terrain_order(const OriginalWorld&,int x,int y,const TerrainObservationRuntime&,const TerrainCallerRuntime&,TerrainCallerOrder&);
}
#endif
