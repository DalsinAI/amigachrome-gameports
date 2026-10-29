/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original veh_cost/prototype_factor arithmetic, independently adapted from
 * OpenSMACX veh.cpp (Brendan Casey C2013-2021), pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 * Edition differences checked against the owner's original instructions.
 */
#ifndef ALPHA_PRODUCTION_COST_H
#define ALPHA_PRODUCTION_COST_H
#include "game_logic.h"
namespace alpha {
struct UnitProductionCostInput {
 Variant variant;
 int32_t design_id,stored_cost,component_base_cost,triad,offense,plan;
 int32_t design_owner_difficulty;
 bool design_owner_free_prototypes,prototype_complete,base_present;
 bool physical_brood_pit,physical_skunkworks;
 int32_t prototype_land_percent,prototype_sea_percent,prototype_air_percent;
};
struct UnitProductionCostResult {int32_t cost,prototype_surcharge;bool has_prototype_surcharge;};
/* Explicit current facts, NOT a saved-world binding or build eligibility.
 * component_base_cost is base_cost's no-abilities component price, NOT the
 * stored design cost or an abilities-inclusive price. The design OWNER supplies
 * difficulty/free-prototype facts, not the producing base owner.
 * Original SMAC has no Brood Pit discount; SMAX discounts predefined native
 * designs. Unsupported unsafe SMAX native/no-base inputs reject. Byte-domain
 * design fields, owner difficulty0..5, percentages0..1000. Every original signed
 * multiply/add intermediate must fit int32; failure preserves output.
 * No production completion, mineral spending, queue or turn mutation.
 */
LogicError unit_production_cost(const UnitProductionCostInput&,UnitProductionCostResult&);
/* Nonnegative already-qualified facility/unit cost and human/AI mineral factor.
 * Zero is meaningful in the original. No claim that the factor is derived here.
 */
LogicError production_mineral_requirement(int32_t cost,int32_t mineral_factor,int32_t& output);
}
#endif
