/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_TERRITORIAL_COMBAT_H
#define AC_ALPHA_TERRITORIAL_COMBAT_H
#include "rules_text.h"
namespace alpha {
enum class TerritoryObservation {missing,supplied};
enum class TerritoryError {ok,bad_state,missing_runtime,unsupported};
struct TerritorialCombatFacts {
 TerritoryObservation observation;Variant edition;
 uint32_t climate;int32_t owner,attacker_faction,defender_faction;
 uint32_t owner_defender_status,owner_attacker_status;
 int32_t attacker_gift,owner_friction;
 uint32_t negotiation_faction,negotiation_other,negotiation_accumulator;
};
struct TerritorialCombatResult {
 uint32_t applied;int32_t gift_before,gift_after,friction_before,friction_after;
 uint32_t negotiation_before,negotiation_after;
};
/* Original pre-promotion territorial gift/friction prefix. Caller supplies actual
 * ordered faction relations and active negotiation state, never save/default
 * inferred authority. Unowned/out-of-domain land is held before indexing. Pure
 * checked transition only: no World, legal attack, session, event or turn commit.
 */
TerritoryError inspect_territorial_combat(const TerritorialCombatFacts&,TerritorialCombatResult&);
}
#endif
