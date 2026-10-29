/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_BASE_BUDGET_H
#define ALPHA_WORLD_BASE_BUDGET_H
#include "world_base_accounting.h"
#include "world_energy_allocation.h"
#include "world_facility_upkeep.h"
namespace alpha {
enum class BudgetError {ok,bad_state,bad_buffer,unsupported,needs_allocation,overflow,out_of_memory};
struct BaseBudgetStage {
 WorldAccountingStage resources;
 int32_t credits_before,maintenance_before;
 uint32_t player_flags_before;
};
struct BaseBudgetResult {
 WorldAccountingResult resources;
 WorldAllocationResult allocation;
 FacilityUpkeepResult maintenance;
 int32_t credits_after_income,mineral_cost_factor;
 bool maintenance_applied;
};
/* Atomic read-only financial projection: fresh support/tiles/convoys/energy ->
 * current sliders and facility/project multipliers -> actual economy income ->
 * original facility maintenance. Never adds raw energy surplus to credits.
 * Explicit credits/maintenance/flag inputs are the running pre-base ledger;
 * commerce remains an explicit fresh prerequisite. The caller must resolve
 * production, growth, workers and riots before treating this as upkeep income.
 * This inspection forecast neither establishes those prerequisites nor applies
 * credits, scrapping, specialists, support flags, research or convoy changes.
 * Stockpile production is excluded and no stockpile credits are inferred.
 * Turns0/1 skip maintenance like the pinned base-upkeep policy. All outputs
 * and actor-state buffers are committed together after the last stage succeeds.
 */
BudgetError world_base_budget(const OriginalWorld&,const Rules&,const UnitDefinitions&,
 const BasicRules&,const WorldRules&,const ResourceInfo&,const UpkeepCitizenDefinitions&,
 const FacilityDefinitions&,size_t base,const BaseBudgetStage&,BaseBudgetResult&,
 uint32_t* projected_support_states,size_t capacity);
}
#endif
