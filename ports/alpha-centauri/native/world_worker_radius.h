/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original farm_compute behavior independently reconstructed from bounded
 * private original execution/instruction controls. Radius offsets from Thinker
 * path.h, MIT, pin dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * No proprietary tables or executable bytes are distributed.
 */
#ifndef ALPHA_WORLD_WORKER_RADIUS_H
#define ALPHA_WORLD_WORKER_RADIUS_H
#include "original_world.h"
namespace alpha {
enum class WorkerRadiusError { ok,bad_state,bad_buffer,bad_stack,out_of_memory,unsupported_stage };
struct WorkerRadiusStage { int32_t upkeep_state;bool editor_active; };
struct WorkerRadiusResult { uint32_t unavailable,flags[25]; };
/* Complete normal offline inspection branch of original farm_compute.
 * Requires explicit actual BaseUpkeepState0/editorfalse; other stages rejected,
 * not simulated. Derives visibility/territory/convoy/hostile-stack restrictions
 * and neighboring-base reservations from owned records. Does not repair stacks,
 * emit AI goals, allocate workers, accumulate resources, or advance a turn.
 * Output must be disjoint from all inputs/world storage; unchanged on failure.
 */
WorkerRadiusError world_worker_radius(const OriginalWorld&,size_t base,
 const WorkerRadiusStage&,WorkerRadiusResult&);
}
#endif
