/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_HUMAN_WORKER_ALLOCATION_H
#define ALPHA_HUMAN_WORKER_ALLOCATION_H
#include "rules_text.h"
namespace alpha {
enum class WorkerAllocationError {ok,bad_state,overflow,bad_buffer,unsupported};
enum class HumanAllocationProfile:uint32_t {directed_no_governor_priorities=0};
struct AllocationCitizen {uint32_t available;int32_t economy,psych,labs;};
struct HumanAllocationFacts {
 Variant variant;HumanAllocationProfile profile;
 int32_t population,requirement,maintenance,current_item,specialists;
 uint32_t worked,unavailable;
 int32_t satellites[3];uint32_t specialist_types[2];int32_t minimum_specialist_size;
 AllocationCitizen citizens[7];int32_t yields[21][3];
};
struct HumanAllocationResult {
 uint32_t worked;int32_t specialists,adjustment,intake[3];
 uint32_t specialist_types[2];bool exchanged,food_deficit;
};
/* Original directed-human missing-worker path, explicit current facts only.
 * No active governor priority weights/AI policy, world authority, yield calls,
 * scratch/resource side effects, assignments, upkeep/turn or persistence.
 * First21 ordinal tiles, centre retained, actual unavailable mask, precomputed
 * actual yields, satellite caps and mineral maintenance must be supplied.
 * Includes strict first-best ties, original9999 worst sentinel, one attempted
 * worker exchange with the original population+1 food margin, and original
 * prerequisite-only specialist fallback. Supplied existing counts must fit the
 * population; specialist adjustment and damaged/overfull arrangements require
 * a separate original prefix/fixup contract. No silent substitute algorithm.
 * Every source signed32 intermediate is checked; failure/alias preserves output.
 */
WorkerAllocationError human_worker_allocation(const HumanAllocationFacts&,HumanAllocationResult&);
}
#endif
