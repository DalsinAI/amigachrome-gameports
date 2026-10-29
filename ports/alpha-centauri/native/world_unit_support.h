/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original support-cost rules, independently adapted from original instruction
 * facts and Thinker base.cpp::mod_base_support / engine_base.h::BaseSupportCosts,
 * MIT, pin dabe62f2d1abba03de40f3dd3048494843f6a9af. See THINKER-LICENSE.txt.
 * Ability dependencies from GPL OpenSMACX, attribution in world_unit_stats.h.
 */
#ifndef ALPHA_WORLD_UNIT_SUPPORT_H
#define ALPHA_WORLD_UNIT_SUPPORT_H
#include "world_unit_stats.h"
namespace alpha {
struct UnitSupportTotals {
 int32_t support_rating,free_units,cost_per_unit,supported_units,maintained_units;
 int32_t clean_exempt,native_fungus_exempt,mineral_cost;
};
/* UNQUALIFIED read-only original unit-mineral upkeep aggregate. Reads pending
 * SUPPORT, actual owned actors/designs/tiles and eligible Clean Reactor facts.
 * Does not assign per-actor support/pacifism flags, calculate convoys or update
 * faction social projections, base accounting, RNG or turns. Not base_support.
 * Output replaced only on complete success; signed32 overflow rejects.
 */
LogicError world_unit_support(const OriginalWorld&,const UnitDefinitions&,const Rules&,
 size_t base,UnitSupportTotals& output);
}
#endif
