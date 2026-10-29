/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original social_upkeep/social_calc adaptation from OpenSMACX faction.cpp,
 * Brendan Casey (C)2013-2021, pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c,
 * and Thinker faction.cpp, MIT THINKER-LICENSE.txt,
 * pindabe62f2d1abba03de40f3dd3048494843f6a9af. Edition differences retained.
 */
#ifndef ALPHA_SOCIAL_UPKEEP_H
#define ALPHA_SOCIAL_UPKEEP_H
#include "game_logic.h"
namespace alpha {
struct SocialUpkeepFacts {
 Variant variant;uint32_t selected[4];int32_t base_effects[11];
 int32_t bonus_count,bonus_kind[8],bonus_first[8],bonus_second[8];
 uint32_t rule_flags;
 bool network_backbone,cloning_vats,ascetic_virtues,living_refinery,temple;
};
struct SocialUpkeepResult {
 uint32_t current[4];int32_t pending[11],active[11],temporary[11];
 int32_t upheaval_cost_paid;
};
/* Pure original social-upkeep proposal, not compute_faction_modifiers. Uses
 * explicit CURRENT base effects; SOCIAL bonus entries do not reconstruct or
 * replace them. Original three calculations use the same selected choices and
 * full calculation (the differing fourth argument is unused), so their outputs
 * agree. SMAX alone adds temple alien research. No pending/active mutation,
 * rules parser, World binding, phase authority, repair, production or turn.
 * Signed intermediate arithmetic is checked; malformed/overflow/overlapping
 * complete input/output ranges reject before writes. Errors preserve output.
 */
LogicError social_upkeep(const SocialRules&,const SocialUpkeepFacts&,SocialUpkeepResult&);
}
#endif
