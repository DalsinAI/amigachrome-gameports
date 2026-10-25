/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original stable-human base_yield composition independently reconstructed
 * from bounded complete-original execution and component controls.
 * Radius offsets: Thinker path.h, MIT pin dabe62f2d1abba03de40f3dd3048494843f6a9af.
 */
#ifndef ALPHA_WORLD_STABLE_INTAKE_H
#define ALPHA_WORLD_STABLE_INTAKE_H
#include "world_worker_radius.h"
#include "world_tile_yields.h"
namespace alpha {
enum class StableIntakeError { ok,bad_state,bad_buffer,unsupported_stage,needs_allocation,bad_stack,out_of_memory,overflow };
struct StableIntakeStage {
 Variant variant;
 bool expansion_enabled,control_upkeep;
 WorkerRadiusStage worker;
 int32_t terraform_reduction_before,base_energy_before;
};
struct StableIntakeResult {
 int32_t satellites[3],tiles[3],intake[4];
 uint32_t worked_tiles,state_flags;
 int32_t raw_eco_damage,terraform_reduction_after,base_energy_after;
 int32_t tile_yields[21][3];
};
/* Fresh intake proposal for an already valid directed-human worked mask:
 * exactly population+1 selected first21 tiles including centre, no specialists,
 * no automatic governor and no selected unavailable non-centre tile.
 * Fungus uses complete fresh technology effects, never saved cache values.
 * This queries actual satellite counts/projects, tile yields and raw ecology;
 * never recycles saved intake/damage. Explicit transient scratch/branch facts
 * are supplied by the caller, not inferred from the saved container.
 * No world/authority/worker reassignment/upkeep/turn/persistence/UI mutation.
 * All inputs/output must be disjoint; failure leaves output unchanged.
 */
StableIntakeError world_stable_intake(const OriginalWorld&,const Rules&,
 const BasicRules&,const ResourceInfo&,size_t base,const StableIntakeStage&,
 StableIntakeResult&);
}
#endif
