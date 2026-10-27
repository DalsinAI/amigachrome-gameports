/* SPDX-License-Identifier: GPL-3.0-or-later. See energy_allocation.h. */
#ifndef ALPHA_WORLD_ENERGY_ALLOCATION_H
#define ALPHA_WORLD_ENERGY_ALLOCATION_H
#include "energy_allocation.h"
#include "original_world.h"
namespace alpha {
struct WorldAllocationResult {AllocationResult allocation;AllocationFacts facts;};
struct ProposedCitizenAssignment {
 int32_t population,specialist_count;
 uint32_t worked_tiles,specialist_types[2];
};
/* Base-game human allocation proposal. Derives actual sliders, current physical
 * facilities, project locations, specialist nibbles/technology availability and
 * global physical Network Node count. Fresh surplus and commerce must come
 * from the current energy/commerce stage; saved accounting totals are ignored.
 * Economy excludes stockpile production. No credits/research/psych-drone/world
 * mutation. Input aliases and all failures preserve output. */
EnergyError world_energy_allocation(const OriginalWorld&,const Rules&,
 const BasicRules&,const CitizenDefinitions&,size_t base,int32_t fresh_surplus,
 int32_t commerce,WorldAllocationResult&);
/* Same current population, with a freshly qualified worker-stage proposal.
 * Requires centre, first21 mask, every citizen assigned, and valid active
 * specialist nibble IDs. Effective types after obsolete upgrades must be
 * available, nonobsolete and meet the minimum-population/psych qualification.
 * This verifies population/count/encoding and specialist consistency;
 * physical tile availability and allocation priority remain the worker stage's
 * prerequisite. It never edits a const world or reuses its obsolete specialist
 * count. Growth must commit/recompute its real population before this call.
 * Proposed upgrade slots are returned through allocation.specialist_types. */
EnergyError world_energy_allocation_for_assignment(const OriginalWorld&,
 const Rules&,const BasicRules&,const CitizenDefinitions&,size_t base,
 const ProposedCitizenAssignment&,int32_t fresh_surplus,int32_t commerce,
 WorldAllocationResult&);
}
#endif
