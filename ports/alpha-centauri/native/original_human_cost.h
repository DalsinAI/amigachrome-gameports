/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independently adapted from Brendan Casey's OpenSMACX base.cpp::cost_factor,
 * C2013-2021, pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c, with original
 * instruction-backed signed arithmetic/default branches. Not the patched
 * minimum-one testbed calculation in colony.cpp.
 */
#ifndef ALPHA_ORIGINAL_HUMAN_COST_H
#define ALPHA_ORIGINAL_HUMAN_COST_H
#include "game_logic.h"
namespace alpha {
struct OriginalHumanCostInput {
 int32_t nutrient_multiplier,mineral_multiplier,map_size;
 int32_t pending_growth,pending_industry;
 bool children_creche,golden_age;
};
struct OriginalHumanCosts { int32_t nutrient,mineral; };
/* Pure human-faction factors only, for both original editions. Explicit original
 * pending social/base facts; no inferred current saved accounting or AI default.
 * Multipliers1..100, map0..6; growth additions reject signed32 overflow. Zero
 * factors and the original Industry default branch are intentionally retained.
 * Output replaced on success only. No owned-world binding, production or turn.
 */
LogicError original_human_costs(const OriginalHumanCostInput&,OriginalHumanCosts&);
}
#endif
