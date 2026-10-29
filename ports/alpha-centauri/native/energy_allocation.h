/* SPDX-License-Identifier: GPL-3.0-or-later. Original algorithm provenance:
 * OpenSMACX GPL3+, Brendan Casey,2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 */
#ifndef ALPHA_ENERGY_ALLOCATION_H
#define ALPHA_ENERGY_ALLOCATION_H
#include "energy_inefficiency.h"
namespace alpha {
struct EnergyAllocationFacts {
 int32_t intake,convoy_to,convoy_from,commerce[7];
 int32_t allocation_psych,allocation_labs,pending_efficiency,hq_distance;
 uint32_t creche,upkeep_stage;
 int32_t counters[9],faction_surplus;
};
struct EnergyAllocationResult {
 int32_t intake,consumption,inefficiency,surplus,economy,psych,labs,unknown_total;
 int32_t faction_surplus,counters[9];
};
/* Explicit plain energy-stage facts: caller supplies actual computed commerce,
 * not an assumed zero. No specialists, facility/project multipliers, alien grid
 * or stockpile conversion in this component. Allocation rates are0..10 with
 * sum<=10. Preserves original pre-commerce faction accumulation and BOTH
 * inefficiency calls/counter proposals. Returns accounting only, no writes to
 * a world, caches/factions, authority, psych processing, upkeep or turn.
 * All signed32 executed arithmetic checked; aliases/errors preserve output.
 */
InefficiencyError energy_allocation(const EnergyAllocationFacts&,EnergyAllocationResult&);
}
#endif
