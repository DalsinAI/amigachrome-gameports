/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_FACILITY_UPKEEP_H
#define ALPHA_WORLD_FACILITY_UPKEEP_H
#include "facility_upkeep.h"
#include "original_world.h"
#include "rules_text.h"
namespace alpha {
struct FacilityUpkeepStage {int32_t credits_before,maintenance_before,mineral_cost_factor;uint32_t player_flags_before;};
/* Derives physical facilities,queue,owner difficulty,highest reactor technology,
 * owned SelfAwareColony project25,and original FACILITY/FREEFAC bonuses. Ledger
 * inputs and mineral cost factor are explicit actual post-income stage facts.
 * Saved economy/energy totals are never substituted for fresh income. No
 * mutation/authority/turn advance; unsupported earlier income remains caller's
 * prerequisite. Failures preserve output; all world/rule/stage storage disjoint.
 */
EnergyError world_facility_upkeep(const OriginalWorld&,const Rules&,const UnitDefinitions&,const FacilityDefinitions&,size_t,const FacilityUpkeepStage&,FacilityUpkeepResult&);
}
#endif
