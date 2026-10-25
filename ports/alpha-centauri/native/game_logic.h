/* SPDX-License-Identifier: GPL-3.0-or-later
 * Owned-state adaptation of Brendan Casey's OpenSMACX (C) 2013-2021,
 * faction.cpp compute_faction_modifiers/social_calc and base.cpp base_nutrient,
 * pinned 2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 */
#ifndef AC_ALPHA_GAME_LOGIC_H
#define AC_ALPHA_GAME_LOGIC_H
#include "rules_text.h"
namespace alpha {
enum class LogicError { ok, bad_state, overflow };
struct SocialContext {
  uint32_t selected[social_category_count];
  bool network_backbone, cloning_vats, ascetic_virtues, living_refinery, temple;
  bool expansion, quick;
};
struct SocialResult { int32_t effects[social_effect_count]; };
/* Definition inputs must already be qualified. Project/facility facts are
 * supplied explicitly; no original-executable callbacks or global addresses.
 * Output is committed only on success. Quick mode excludes base faction,
 * project additions, immunity and robust; project penalty removal still runs.
 */
LogicError calculate_social(const SocialRules& rules, const Faction& faction,
                            const SocialContext& context, SocialResult& output);
struct NutrientInput {
  int32_t population, nutrient_per_citizen, intake, accumulated;
  int32_t convoy_to, convoy_from, pending_growth, upkeep_stage;
  int32_t faction_surplus_total;
  bool children_creche, golden_age;
};
struct NutrientResult {
  int32_t growth, intake, consumption, surplus, accumulated, faction_surplus_total;
};
/* One base_nutrient calculation, not growth/population advancement or a turn.
 * No repeated-call reset of intake/totals is implied: caller supplies the
 * current stage's pre-convoy intake and current faction aggregate each time.
 */
LogicError calculate_nutrients(const NutrientInput& input, NutrientResult& output);
}
#endif
