/* SPDX-License-Identifier: GPL-3.0-or-later
 * Restoration policy adapted from Thinker base.cpp::mod_base_production,
 * pin dabe62f2d1abba03de40f3dd3048494843f6a9af, MIT; see THINKER-LICENSE.txt.
 * Original executable parity is unproven. No unit/project/satellite completion.
 */
#ifndef ALPHA_FACILITY_PRODUCTION_H
#define ALPHA_FACILITY_PRODUCTION_H
#include <stdint.h>
namespace alpha {
enum class FacilityProductionError {ok,bad_state,bad_buffer,overflow,not_human,not_owner,
 unsupported,unavailable,already_processed,stale_world};
struct FacilityProductionFacts {
 int32_t accumulated,baseline,mineral_intake,surplus,cost_units,cost_factor,exemption,best_output;
 uint32_t state_flags;
 bool human,starved,riots;
};
struct FacilityProductionResult {
 int32_t accumulated,baseline,cost,credited,best_output;
 uint32_t state_flags;
 bool completed;
};
/* Explicit fresh-stage mineral facts, not saved surplus. Preserves the pinned
 * accumulation/carryover branch, including completion during riots when already
 * funded and zero human carryover for negative surplus. Partial aliases and
 * invalid/overflow inputs leave output untouched. Starvation still updates the
 * best-output statistic and clears the previous production-done bit.
 */
FacilityProductionError facility_production(const FacilityProductionFacts&,FacilityProductionResult&);
const char* facility_production_error_name(FacilityProductionError);
}
#endif
