/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original battle_compute airdrop/fanatic/faction/Planet phases; OpenSMACX
 * pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c, Brendan Casey2013-2021.
 * Actual original instruction arithmetic takes precedence over rewrite errors.
 */
#ifndef AC_ALPHA_COMBAT_FACTIONS_H
#define AC_ALPHA_COMBAT_FACTIONS_H
#include "psi_abilities.h"
namespace alpha {
struct CombatFactionContext {
 Variant variant;int32_t offense,defense;uint32_t psi_mask,attacker_plan;
 bool made_airdrop,drop_pods,fanatic;uint32_t attacker_faction;
 int32_t active_planet,airdrop_penalty,fanatic_bonus,planet_percent;
 uint32_t offense_count,defense_count;int32_t offense_factors[8],defense_factors[8];
};
struct CombatFactionStrengths {int32_t offense,defense;uint32_t applied;};
/* Both actors present, normal flags0/non-Probe attacker. Offense AFTER early
 * fungus/Resonance/Empath; defense AFTER basic defense BEFORE terrain/base.
 * Separate phase inputs, no full battle, mode/attack/session or World authority.
 * SMAX only matching OFFENSE/DEFENSE factors supplied in original record order.
 * Applied bits1/2/4/8/16: drop/fanatic/offense factors/Planet/defense factors.
 * Checked signed multiply BEFORE division toward zero. Errors preserve output.
 */
LogicError combat_faction_modifiers(const CombatFactionContext&,CombatFactionStrengths&);
}
#endif
