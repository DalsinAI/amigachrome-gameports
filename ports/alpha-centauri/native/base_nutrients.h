/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original nutrient accounting reconstructed from both complete original helpers.
 * Source reference: OpenSMACX base.cpp, GPL-3.0-or-later, Brendan Casey,
 * pin 2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 */
#ifndef ALPHA_BASE_NUTRIENTS_H
#define ALPHA_BASE_NUTRIENTS_H
#include <stddef.h>
#include <stdint.h>
namespace alpha {
enum class NutrientError {ok,bad_state,bad_buffer,overflow};
struct NutrientFacts {
 int32_t population,pending_growth,intake,convoy_to,convoy_from;
 int32_t requirement,stored,faction_surplus;
 uint32_t upkeep_stage,flags; // flags: physical Creche bit0, GoldenAge bit1
};
struct BaseNutrientResult {int32_t growth,intake,consumption,surplus,stored,faction_surplus;};
// Explicit facts only. No growth accumulation, authority, upkeep or world write.
// Population 0..127, nonnegative requirement; every original intermediate must
// fit signed32. Original wrapped overflow is rejected. Output/input disjoint;
// any failure preserves output. Only upkeep_stage==1 changes faction surplus.
NutrientError base_nutrients(const NutrientFacts&,BaseNutrientResult&);
}
#endif
