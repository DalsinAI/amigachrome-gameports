/* SPDX-License-Identifier: GPL-3.0-or-later
 * Initial greedy tile scoring: Thinker base.cpp::mod_base_yield, MIT,
 * pin dabe62f2d1abba03de40f3dd3048494843f6a9af, Copyright induktio.
 * This is the directed-human, pre-specialist worker proposal, not recovered
 * original base_yield or Thinker's complete governor/drone optimisation.
 */
#ifndef ALPHA_WORKER_ALLOCATION_H
#define ALPHA_WORKER_ALLOCATION_H
#include <stdint.h>
namespace alpha {
enum class UpkeepWorkerAllocationError {ok,bad_state,bad_buffer,overflow};
struct WorkerAllocationFacts {
 uint32_t available,worked;
 int32_t population,specialists,nutrient_before,mineral_before,energy_before;
 int32_t efficiency,psych_allocation,nutrient_threshold;
 bool can_grow,can_riot;
 int32_t yields[21][3];
};
struct WorkerAllocationResult {
 uint32_t worked;
 int32_t specialists,unallocated,nutrient_after,mineral_after,energy_after;
 uint32_t added[20],added_count;
 bool reset_overassigned,requires_specialist_resolution,requires_riot_resolution;
};
/* Preserve valid directed assignments. Discard unavailable/high bits; repair an
 * overassigned population by restarting at centre. Fill missing workers using
 * the pinned initial greedy score and exact index tie break. This deliberately
 * returns BEFORE specialist substitution and drone optimisation: caller must
 * resolve both requirements before committing a turn. No saved yield inputs.
 * available must contain centre; centre is free and population excludes it.
 * before values contain satellite/convoy income less food/support consumption,
 * before adding worked tiles. No mutation, output unchanged on every error.
 */
UpkeepWorkerAllocationError worker_allocation_candidate(const WorkerAllocationFacts&,WorkerAllocationResult&);
}
#endif
