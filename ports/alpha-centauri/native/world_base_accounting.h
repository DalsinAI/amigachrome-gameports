/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_BASE_ACCOUNTING_H
#define ALPHA_WORLD_BASE_ACCOUNTING_H
#include "world_base_minerals.h"
#include "world_base_support.h"
#include "world_energy.h"
#include "base_nutrients.h"
namespace alpha {
struct WorldAccountingStage {
 StableIntakeStage intake;
 int32_t commerce,energy_grid; // fresh trade inputs; not saved base totals
};
struct WorldAccountingResult {
 SupportResult support;
 WorldMineralResult mineral;
 BaseNutrientResult nutrient;
 WorldEnergyResult energy;
 EnergyShortfallOrders energy_shortfall;
};
/* One connected read-only accounting proposal for an owned HUMAN base-game
 * base: derive fresh support, then recompute convoys and worked-tile intake
 * once, then reuse exactly those results for minerals, food and energy.
 * Saves no totals and does not advance a turn or apply projected actor flags.
 * Commerce/grid are unresolved fresh trade facts, explicitly supplied by the
 * caller. Existing stable-directed worker inspection guards remain in force.
 * All outputs are atomic together and disjoint from each other and all source
 * storage. Failure preserves the complete summary and actor-state buffer.
 */
StableIntakeError world_base_accounting(const OriginalWorld&,const Rules&,
 const UnitDefinitions&,const BasicRules&,const WorldRules&,const ResourceInfo&,
 size_t base,const WorldAccountingStage&,WorldAccountingResult&,
 uint32_t* projected_support_states,size_t capacity);
}
#endif
