/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_FRESH_MINERALS_H
#define ALPHA_WORLD_FRESH_MINERALS_H
#include "world_convoy_nutrients.h"
#include "world_unit_support.h"
#include "base_minerals.h"
namespace alpha {
struct WorldMineralStage {
 StableIntakeStage intake;
 // SMAX v12 contains eight LE tectonic counters; earlier versions need an
 // explicit runtime counter. Base SMAC has no tectonic contribution.
 bool runtime_tectonic;
 int32_t tectonic_count;
};
struct WorldMineralResult {
 ConvoyNutrientResult food;
 UnitSupportTotals support;
 MineralInput facts;
 MineralResult minerals;
};
/* Read-only fresh food/mineral proposal. Derives mineral pre-intake/raw ecology
 * from fresh tiles, actual convoy actors, pending support and Clean/native
 * exemptions, physical facilities/project base owners, map and faction facts.
 * Ascending deliberately retains original global Voice != -1 semantics.
 * No source global is fabricated; missing SMAX tectonic state requires explicit
 * runtime facts or rejects. Matching wire/component editions only; expansion
 * legacy-save fixups are unqualified. Does not execute full support side effects, commit
 * accounting, complete production, advance upkeep/turns, or grant authority.
 * Failure preserves output; all input/output storage must be disjoint.
 */
StableIntakeError world_fresh_minerals(const OriginalWorld&,const Rules&,
 const BasicRules&,const WorldRules&,const ResourceInfo&,const UnitDefinitions&,
 size_t,const WorldMineralStage&,WorldMineralResult&);
}
#endif
