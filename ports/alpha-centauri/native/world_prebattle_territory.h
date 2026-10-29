/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_WORLD_PREBATTLE_TERRITORY_H
#define AC_ALPHA_WORLD_PREBATTLE_TERRITORY_H
#include "original_world.h"
#include "prebattle_territory.h"
namespace alpha {
struct WorldPrebattleTerritoryRuntime {
 BattleEntryObservation observation;
 uint32_t current_faction,negotiation_faction,negotiation_other,negotiation_accumulator;
};
struct WorldPrebattleTerritoryProposal {
 uint32_t tile_index;int32_t owner;uint32_t attacker,defender;
 PrebattleTerritoryProposal effect;
};
/* Supplied current PRE-movement frame: attacker coordinates are the original
 * starting coordinates captured by this caller. Do not use this query on an
 * already relocated frame to reconstruct the origin. Resolves origin tile,
 * directional relations, military and friction from the owned World. Negative
 * or out-of-domain owners are held before indexing; owner0/participants are
 * observed identity proposals using their real records. The original outer
 * attacker-plan11 and defender-plan12 exclusions are held. Explicit current and
 * negotiation state only, no saved authority/event completion or stage advance.
 */
PrebattleTerritoryError inspect_world_prebattle_territory(const OriginalWorld&,size_t,size_t,
 const WorldPrebattleTerritoryRuntime&,WorldPrebattleTerritoryProposal&);
}
#endif
