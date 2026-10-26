/* SPDX-License-Identifier: GPL-3.0-or-later
 * OpenSMACX (C)2013-2021 Brendan Casey, base.cpp::fac_maint004F6510 /
 * base_maint004F65F0, pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 */
#ifndef ALPHA_FACILITY_UPKEEP_H
#define ALPHA_FACILITY_UPKEEP_H
#include "base_energy.h"
namespace alpha {
struct FacilityUpkeepFacts {
 int32_t first_project,difficulty,best_reactor,credits_before,maintenance_before,mineral_cost_factor,queue_item;
 uint32_t player_flags_before;bool self_aware_colony;
 uint8_t built[12],free_maintenance[12];
 int32_t cost[70],maintenance[70];
};
struct FacilityUpkeepResult {
 int32_t credits_after,maintenance_after;uint32_t player_flags_after;
 uint8_t built_after[12],scrapped[12],shortage[12];int32_t charged[70];
};
/* Original ordinal1..<first_project loop (base39,SMAX70). Credits and ledger
 * flags must describe the actual post-income/pre-maintenance stage. Only
 * physical built bits below repeatable start(base34,SMAX65) are charged;
 * original single-argument has_fac_built rejects repeatable IDs. CommandCenter27 dynamically costs
 * min(best_reactor,(difficulty+1)/2), BEFORE free-facility bonus checks, as the
 * original does. Self-AwareColony carries half-credit rounding in flag0x20.
 * Shortages clamp reserves0 at difficulty<=1 or when the same facility is
 * queued; otherwise scrap the facility and REPLACE reserves with cost*factor.
 * The maintenance total retains each charged cost even when scrapping occurs.
 * Normal nonnegative cost/maintenance/credit domain; signed32 arithmetic and
 * all input/output overlaps are checked transactionally. No world mutation,
 * popup, free-facility regeneration, production completion or turn execution.
 */
EnergyError facility_upkeep(const FacilityUpkeepFacts&,FacilityUpkeepResult&);
}
#endif
