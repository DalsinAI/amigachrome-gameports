/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TERRAIN_OBSERVATION_H
#define ALPHA_TERRAIN_OBSERVATION_H
#include "original_world.h"
namespace alpha {
enum class TerrainSourceEdition {smac,smacx};
enum class TerrainObservationStatus {resolved,held_neighbour_visibility,held_negative_faction_lookup,missing_runtime,invalid_input};
struct TerrainObservationRuntime {
 int32_t viewer,source_zoom,current_faction;uint32_t draw_flags,game_rules,more_preferences,game_state;
 TerrainSourceEdition edition;bool supplied=false;
};
struct TerrainObservation {
 uint32_t displayed_items,landmarks_after;
 uint8_t rainfall,rockiness,source_faction;bool concealed,visible,far_profile;
};
// Closed source observation prelude, both original editions. Runtime flags are
// explicit, not inferred from saved preferences. Complex neighbour visibility
// returns typed-held without guesses/callbacks. No mutation of World cache bit.
// Literal source viewer0 non-revealed snapshot reads landmarks (tileoffset12),
// not visible_items[-1]. Negative-viewer faction lookup is held when reached.
// On anything but resolved, output remains unchanged. Reject all World aliases.
TerrainObservationStatus observe_world_terrain(const OriginalWorld&,int x,int y,const TerrainObservationRuntime&,TerrainObservation&);
}
#endif
