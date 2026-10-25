/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_FRESH_NUTRIENTS_H
#define ALPHA_WORLD_FRESH_NUTRIENTS_H
#include "world_stable_intake.h"
#include "base_nutrients.h"
namespace alpha {
struct FreshNutrientStage {StableIntakeStage intake;int32_t convoy_to,convoy_from;};
struct FreshNutrientResult {StableIntakeResult intake;NutrientResult nutrients;};
// Derives owner/population/pending growth/physical Creche/GoldenAge/stored food
// and faction total from the owned snapshot; requirement from actual BasicRules.
// Intake is freshly computed, never the saved intake. Convoys remain explicit
// transient facts, not claimed as a restored convoy engine. Worker inspection0
// is the same actual upkeep stage passed to nutrient accounting, so the faction
// aggregate is unchanged here. No record/authority/assignment/upkeep mutation.
// All world/rule/stage storage must be disjoint from output; failure preserves it.
StableIntakeError world_fresh_nutrients(const OriginalWorld&,const Rules&,
 const BasicRules&,const ResourceInfo&,size_t,const FreshNutrientStage&,
 FreshNutrientResult&);
}
#endif
