/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_PREBATTLE_TERRITORY_H
#define AC_ALPHA_PREBATTLE_TERRITORY_H
#include "battle_entry.h"
namespace alpha {
enum class PrebattleTerritoryError {ok,bad_state,missing_runtime,unsupported};
struct PrebattleTerritoryFacts {
 BattleEntryObservation observation;Variant edition;
 uint32_t attacker,defender,current_faction;int32_t owner;
 uint32_t attacker_owner_status,defender_owner_status,owner_defender_status;
 int32_t owner_strength,defender_strength,defender_owner_friction;
 uint32_t negotiation_faction,negotiation_other,negotiation_accumulator;
};
struct PrebattleTerritoryProposal {
 uint32_t owner_defender_status,defender_owner_status;
 int32_t defender_owner_friction;
 uint32_t negotiation_accumulator,treaty_applied,friction_applied;
};
/* Original PRE-round third-party loop after aggression; distinct from the
 * post-combat territorial gift/friction prefix. The signed tile owner selects
 * at most one of original candidates1..7. Unmatched owners are pure no-ops.
 * Actual current diplomacy/military and active negotiation facts are explicit;
 * this proposal establishes no earlier event completion or legal attack.
 * No World mutation, callbacks, rounds or session authority.
 */
PrebattleTerritoryError inspect_prebattle_territory(const PrebattleTerritoryFacts&,PrebattleTerritoryProposal&);
}
#endif
