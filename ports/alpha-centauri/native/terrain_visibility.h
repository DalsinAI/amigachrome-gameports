/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TERRAIN_VISIBILITY_H
#define ALPHA_TERRAIN_VISIBILITY_H
#include "terrain_observation.h"
#include "rules_text.h"
namespace alpha {
enum class TerrainVisibilityStatus {resolved,missing_runtime,invalid_input,bad_world,bad_rules,held_negative_viewer,held_negative_owner_runtime,out_of_memory,held_geometry};
struct TerrainVisibilityRuntime {
 TerrainObservationRuntime observation;
 int32_t moving_x,moving_y,moving_vehicle;
 uint32_t source_map_flags; // Actual runtime MapFlags; low bit must match owned geometry.
 bool expansion_enabled;
 // Source reverse diplomacy can index faction[-1] for an unoccupied tile.
 // These eight low pact bits are explicit observations of that runtime memory,
 // never manufactured from a save pointer, inferred as zero or read out of bounds.
 uint8_t negative_owner_reverse_pacts;
 bool negative_owner_reverse_supplied=false,supplied=false;
};
// Complete neighbour path for validated owned stacks/base bits. Only resolved
// output writes. Base and SMAX radar/sensor mechanisms remain edition-specific.
// No repairs/logging/OS callbacks and no moving-actor defaults. Original
// neighbour topology, one-step wrap and source short-circuit ordering retained.
TerrainVisibilityStatus observe_world_terrain_visibility(const OriginalWorld&,const Rules&,const UnitDefinitions&,int x,int y,const TerrainVisibilityRuntime&,TerrainObservation&);
}
#endif
