/* SPDX-License-Identifier: GPL-3.0-or-later. Policy attribution in worker_allocation.h. */
#ifndef ALPHA_WORLD_WORKER_UPKEEP_H
#define ALPHA_WORLD_WORKER_UPKEEP_H
#include "worker_allocation.h"
#include "world_convoy_nutrients.h"
#include "world_base_support.h"
namespace alpha {
struct WorkerUpkeepStage {int32_t upkeep_state;bool editor_active,control_upkeep;};
struct WorkerUpkeepCandidate {
 WorkerRadiusResult radius;
 WorldConvoyFlows convoys;
 SupportResult support;
 WorkerAllocationFacts facts;
 WorkerAllocationResult allocation;
 StableIntakeResult intake;
};
/* Read-only base-game human stage1 candidate. Derives candidate yields from
 * real tiles/fresh technology, fresh convoy/support, satellite counts, current
 * directed mask/population, current caps and inefficiency. No saved resource
 * totals are inputs. Human physical radius restrictions reuse the independently
 * qualified inspection radius query; this is an explicit restoration policy,
 * not a claim that the recovered original upkeep branch is complete. Actual
 * stage1 is required, inspection0/editor/AI/governor/SMAX reject.
 * Returned assignment/intake are a proposal. Specialist substitution/technology
 * and riots remain explicit unresolved events; do not commit this as a turn or
 * bypass those events. No actor/world/authority/ledger/persistence mutation.
 * Inputs must be disjoint from output. Every error preserves complete output.
 */
StableIntakeError world_worker_upkeep_candidate(const OriginalWorld&,const Rules&,
 const UnitDefinitions&,const BasicRules&,const ResourceInfo&,size_t base,
 const WorkerUpkeepStage&,WorkerUpkeepCandidate&);
}
#endif
