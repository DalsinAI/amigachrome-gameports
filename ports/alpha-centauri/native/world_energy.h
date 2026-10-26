/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_ENERGY_H
#define ALPHA_WORLD_ENERGY_H
#include "world_fresh_minerals.h"
#include "world_citizen_energy.h"
#include "world_psych.h"
#include "world_commerce.h"
namespace alpha {
enum class CommerceEnergyStage : uint32_t { current_first_field_fresh_technology=0 };
struct WorldEnergyStage {
 WorldMineralStage resources;
 PoliceSnapshotProfile snapshot;
 CommerceEnergyStage commerce;
 uint32_t upkeep_stage;
 AlienGridPolicy alien_grid_policy;
};
struct WorldEnergyResult {
 WorldMineralResult resources;
 WorldCommerceResult commerce;
 int32_t hq_distance;
 BaseEnergyResult energy;
};
/* Complete read-only resource-to-energy proposal within the inherited stable
 * directed-human allocation domain. Fresh tiles/satellites/convoy/support/
 * minerals feed energy; never saved intake_2 or mineral surplus. Commerce ranks
 * explicitly use current first energy fields and fresh all-faction technology
 * bonuses, preserving original stage ordering instead of recomputing all ranks.
 * Upkeep projection/snapshot/transient inputs/alien scratch policy are explicit.
 * Inherited fresh resources currently reject specialists, governor allocation,
 * unavailable workers and unsupported stages; no replacement/fallback occurs.
 * All tables must be qualified from one same-edition rules input. Proposal
 * only: no authority, mutation, accounting commit, turn, UI or persistence.
 */
StableIntakeError world_energy(const OriginalWorld&,const Rules&,const BasicRules&,
 const WorldRules&,const ResourceInfo&,const UnitDefinitions&,const CitizenDefinitions&,
 size_t,const WorldEnergyStage&,WorldEnergyResult&);
}
#endif
