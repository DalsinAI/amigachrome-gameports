/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_HUMAN_ALLOCATION_H
#define ALPHA_WORLD_HUMAN_ALLOCATION_H
#include "human_worker_allocation.h"
#include "world_stable_intake.h"
#include "world_unit_support.h"
#include "world_citizen_energy.h"
namespace alpha {
struct WorldHumanAllocationResult {
 HumanAllocationFacts facts;
 HumanAllocationResult allocation;
 HumanAllocationTrace trace;
 StableIntakeResult intake;
};
/* Original manual-human allocation proposal from actual owned world facts.
 * Uses explicit worker/upkeep and transient scratch inputs; governor bit31,
 * specialist adjustment and overfull/damaged arrangements are unsupported.
 * Unit/citizen/technology/basic/resource tables must share the same rules input.
 * Coherent retained arrangements may preserve real governor bit31 because the
 * original exits before scoring; missing workers with bit31 are rejected.
 * Preflights all available first21 tiles, then replays actual fresh yield leaves
 * in the recorded call order, including rejected swaps and channel3 resets.
 * Preflight may reject an overflowing available tile that the selector would
 * never visit. No silent integer wrap or invented runtime authority/defaults.
 * No assignment/accounting/world/turn/AI/UI/persistence mutation. Every input
 * and owned world allocation must be disjoint from output. Failure preserves it.
 */
StableIntakeError world_human_allocation(const OriginalWorld&,const Rules&,
 const BasicRules&,const ResourceInfo&,const UnitDefinitions&,const CitizenDefinitions&,
 size_t,const StableIntakeStage&,WorldHumanAllocationResult&);
}
#endif
