/* SPDX-License-Identifier: GPL-3.0-or-later
 * See game_logic.h for source attribution and exact scope.
 */
#include "game_logic.h"
namespace alpha {
static bool signed_word(int64_t n) { return n >= (-2147483647LL-1) && n <= 2147483647LL; }
static bool add_word(int64_t& target, int64_t delta) {
  const int64_t value = target + delta;
  if (!signed_word(value)) return false;
  target = value;
  return true;
}
LogicError calculate_social(const SocialRules& rules, const Faction& faction,
                            const SocialContext& context, SocialResult& output) {
  if (faction.bonus_count > 8) return LogicError::bad_state;
  for (uint32_t choice : context.selected) if (choice >= social_model_count) return LogicError::bad_state;
  for (size_t i = 0; i < faction.bonus_count; ++i) {
    const auto& b = faction.bonuses[i];
    if ((int32_t)b.kind < 0 || (int32_t)b.kind > 18) return LogicError::bad_state;
    if (b.kind == FactionBonusKind::social || b.kind == FactionBonusKind::immunity || b.kind == FactionBonusKind::robust) {
      if (b.value < 0 || b.value >= (int32_t)social_effect_count) return LogicError::bad_state;
    } else if (b.kind == FactionBonusKind::impunity || b.kind == FactionBonusKind::penalty) {
      if (b.value < 0 || b.value >= (int32_t)social_category_count ||
          b.modifier < 0 || b.modifier >= (int32_t)social_model_count) return LogicError::bad_state;
    }
  }
  int64_t values[social_effect_count] = {};
  for (size_t cat = 0; cat < social_category_count; ++cat) {
    const uint32_t model = context.selected[cat];
    for (size_t eff = 0; eff < social_effect_count; ++eff) {
      int64_t value = rules.models[cat][model].effects[eff];
      if (value < 0) {
        if ((cat == 3 && ((model == 1 && context.network_backbone) || (model == 3 && context.cloning_vats))) ||
            (cat == 2 && model == 1 && context.cloning_vats)) value = 0;
        if (value < 0) {
          for (size_t i = 0; i < faction.bonus_count; ++i) {
            const auto& b = faction.bonuses[i];
            if (b.value != (int32_t)cat || b.modifier != (int32_t)model) continue;
            if (b.kind == FactionBonusKind::impunity) {
              if (!add_word(values[eff], -value)) return LogicError::overflow;
            } else if (b.kind == FactionBonusKind::penalty) {
              if (!add_word(values[eff], value)) return LogicError::overflow;
            }
          }
        }
      }
      if (!add_word(values[eff], value)) return LogicError::overflow;
    }
  }
  if (!context.quick) {
    if (context.ascetic_virtues && !add_word(values[5], 1)) return LogicError::overflow;
    if (context.living_refinery && !add_word(values[2], 2)) return LogicError::overflow;
    if (context.temple) {
      if (!add_word(values[7], 1)) return LogicError::overflow;
      if (context.expansion && (faction.rule_flags & 0x80u) && !add_word(values[10], 1)) return LogicError::overflow;
    }
    // compute_faction_modifiers adds all SOCIAL entries into base effects.
    int64_t base[social_effect_count] = {};
    for (size_t i = 0; i < faction.bonus_count; ++i) {
      const auto& b = faction.bonuses[i];
      if (b.kind == FactionBonusKind::social && !add_word(base[b.value], b.modifier)) return LogicError::overflow;
    }
    for (size_t eff = 0; eff < social_effect_count; ++eff)
      if (!add_word(values[eff], base[eff])) return LogicError::overflow;
    // Preserve order: overlapping IMMUNITY/ROBUST entries are not commutative.
    for (size_t i = 0; i < faction.bonus_count; ++i) {
      const auto& b = faction.bonuses[i];
      if (b.kind == FactionBonusKind::immunity) {
        int64_t& value = values[b.value];
        value = value < 0 ? 0 : value > 999 ? 999 : value;
      } else if (b.kind == FactionBonusKind::robust && values[b.value] < 0) {
        values[b.value] /= 2; // signed truncation toward zero, including odd negatives
      }
    }
  }
  for (int64_t value : values) if (!signed_word(value)) return LogicError::overflow;
  for (size_t i = 0; i < social_effect_count; ++i) output.effects[i] = (int32_t)values[i];
  return LogicError::ok;
}
LogicError calculate_nutrients(const NutrientInput& in, NutrientResult& out) {
  if (in.population < 0 || in.population > 127 || in.nutrient_per_citizen < 0 ||
      in.nutrient_per_citizen > 100 || in.convoy_to < 0 || in.convoy_from < 0 || in.upkeep_stage < 0)
    return LogicError::bad_state;
  const int64_t growth = (int64_t)in.pending_growth + (in.children_creche ? 2 : 0) + (in.golden_age ? 2 : 0);
  const int64_t intake = (int64_t)in.intake + in.convoy_to;
  const int64_t consumption = (int64_t)in.convoy_from + (int64_t)in.population * in.nutrient_per_citizen;
  const int64_t surplus = intake - consumption;
  int32_t accumulated = in.accumulated;
  if (surplus >= 0) { if (accumulated < 0) accumulated = 0; }
  else if (!accumulated) accumulated = -1;
  const int64_t total = (int64_t)in.faction_surplus_total +
    (in.upkeep_stage == 1 ? (surplus < 0 ? 0 : surplus > 99 ? 99 : surplus) : 0);
  if (!signed_word(growth) || !signed_word(intake) || !signed_word(consumption) ||
      !signed_word(surplus) || !signed_word(total)) return LogicError::overflow;
  out = {(int32_t)growth,(int32_t)intake,(int32_t)consumption,(int32_t)surplus,accumulated,(int32_t)total};
  return LogicError::ok;
}
}
