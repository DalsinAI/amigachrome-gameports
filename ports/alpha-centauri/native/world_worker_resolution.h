/* SPDX-License-Identifier: GPL-3.0-or-later
 * Directed-human Thinker default worker/specialist/psych restoration policy,
 * MIT pin dabe62f2d1abba03de40f3dd3048494843f6a9af. See WORKER-UPKEEP.md.
 */
#ifndef ALPHA_WORLD_WORKER_RESOLUTION_H
#define ALPHA_WORLD_WORKER_RESOLUTION_H
#include "world_worker_upkeep.h"
#include "world_energy_allocation.h"
#include "world_base_psych.h"
#include "world_base_minerals.h"
#include "upkeep_world_energy.h"
namespace alpha {
struct WorkerResolutionStage {WorkerUpkeepStage worker;int32_t commerce,energy_grid;};
struct WorkerResolutionResult {
 WorkerUpkeepCandidate candidate;
 ProposedCitizenAssignment assignment;
 StableIntakeResult intake;
 WorldMineralAccounting mineral;
 BaseNutrientResult nutrient;
 UpkeepWorldEnergyResult energy;
 WorldAllocationResult allocation;
 WorldPsychResult psych;
 uint32_t specialist_adjust,trimmed_workers;
 bool requires_riot_event;
};
/* Derive candidate internally; complete eligible specialist assignment and
 * fresh iterative resource/energy/psych evaluation, trimming newly assigned
 * workers under the pinned default policy. Never accepts a caller's candidate
 * or saved resource/drone totals as authority. Commerce/grid are explicit
 * authoritative current-stage inputs, not computed here. All current world
 * inputs remain unchanged. Output is complete and atomic; no world/turn/event
 * mutation. True requires_riot_event needs actual event handling, not a bypass.
 * Final nutrient and energy calculations use stage1 once against original
 * current faction aggregates; intermediate trials never compound aggregates.
 */
StableIntakeError world_worker_resolution(const OriginalWorld&,const Rules&,
 const UnitDefinitions&,const BasicRules&,const WorldRules&,const ResourceInfo&,
 const UpkeepCitizenDefinitions&,size_t base,const WorkerResolutionStage&,
 WorkerResolutionResult&);
}
#endif
