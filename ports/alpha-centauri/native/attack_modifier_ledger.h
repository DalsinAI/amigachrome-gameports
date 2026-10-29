/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_ATTACK_MODIFIER_LEDGER_H
#define AC_ALPHA_ATTACK_MODIFIER_LEDGER_H
#include "battle_entry.h"
namespace alpha {
struct AttackModifierSlot {uint32_t label;int32_t percent;};
struct AttackModifierLedger {uint32_t count;AttackModifierSlot slots[4];};
struct AttackModifierEvent {uint32_t label;int32_t percent;uint32_t appended;};
struct AttackModifierFacts {
 BattleEntryObservation observation;Variant edition;
 uint32_t fungus_applied,fanatic_flag;int32_t fanatic_percent;
 uint32_t offense_factor_count;int32_t offense_factors[8];
 uint32_t ground_applied;int32_t mobile_percent,road_percent,downhill_percent;
};
struct AttackModifierResult {
 AttackModifierLedger ledger;
 uint32_t lookup_count;AttackModifierEvent lookups[13];
};
/* Source order for normal flags0, conventional nonbase/non-native ground,
 * no attacker abilities/grants: Fungus, Fanatic, expansion OFFENSE rows,
 * Mobile/Open, Road, Downhill. Facts must come from actual current stages.
 * Original resets count only; unused slots retain their previous contents.
 * Every entered source lookup is retained, including factor100 and saturated
 * ledgers. Returned label ordinals are unresolved lookup obligations, not text,
 * string-copy completion or GUI acknowledgement. No strength or World writes.
 */
BattleEntryError attack_modifier_ledger(const AttackModifierFacts&,
 const AttackModifierLedger&prior,AttackModifierResult&);
}
#endif
