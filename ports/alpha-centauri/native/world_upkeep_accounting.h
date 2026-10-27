/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_UPKEEP_ACCOUNTING_H
#define ALPHA_WORLD_UPKEEP_ACCOUNTING_H
#include "world_worker_resolution.h"
namespace alpha {
struct WorldUpkeepAccountingResult {
 WorkerResolutionResult resolved;
 EnergyShortfallOrders energy_shortfall;
};
/* One internally derived stage1 accounting proposal for the CURRENT owned
 * human base-game world. Calls the worker/specialist/resource/energy/psych
 * resolver itself; it accepts no caller worker result, intake, surplus, support
 * total or saved accounting display as freshness authority. Growth must stage
 * its real new population in this current world before recomputing this API.
 * Commerce/grid remain explicit unresolved upstream trade facts.
 * Returns coherent final food/minerals/energy/allocation/psych plus original
 * energy-shortfall cancellation proposals and projected support actor states.
 * Faction nutrient/hypothetical-efficiency aggregates are proposals derived
 * once from the CURRENT faction state; no intermediate trial is accumulated.
 * requires_riot_event is preserved: this calculation does not resolve riots,
 * mutate the world, complete production, grant turn authority or advance time.
 * Inputs, both outputs and full actor-state buffer must be disjoint. Error
 * leaves both outputs untouched. Only world.vehicle_count states are written;
 * any caller-provided buffer tail remains unchanged. Engine is single-threaded.
 */
StableIntakeError world_upkeep_accounting(const OriginalWorld&,const Rules&,
 const UnitDefinitions&,const BasicRules&,const WorldRules&,const ResourceInfo&,
 const CitizenDefinitions&,size_t base,const WorkerResolutionStage&,
 WorldUpkeepAccountingResult&,uint32_t* projected_support_states,size_t capacity);
}
#endif
