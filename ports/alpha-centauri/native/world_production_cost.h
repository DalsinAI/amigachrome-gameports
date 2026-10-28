/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_PRODUCTION_COST_H
#define ALPHA_WORLD_PRODUCTION_COST_H
#include "original_world.h"
#include "production_cost.h"
namespace alpha {
struct WorldProductionCost {
 int32_t mineral_factor,item_cost,component_base_cost,prototype_surcharge,mineral_requirement;
};
/* Original current-world HUMAN production cost, not build eligibility/order or
 * a turn. Producing base owner supplies pending Industry; saved human mask must
 * identify it as human. Design owner supplies difficulty/FREEPROTO only when
 * original untyped-custom branch reads them. Physical base bits are used.
 * Components are priced WITHOUT abilities for the prototype surcharge.
 * Exact runtime edition must match wire version; no expansion fixups guessed.
 * Original zero factor is retained. AI factors remain unsupported. Output and
 * world/RNG unchanged on failure. Output overlapping input tables, the World or
 * any owned wire/decoded allocation rejects before writes. No allocation, caches, gameplay mutation/UI.
 */
LogicError world_production_cost(const OriginalWorld&,const UnitDefinitions&,
 const FacilityDefinitions&,const BasicRules&,Variant runtime_profile,size_t base,
 int32_t item,WorldProductionCost& output);
}
#endif
