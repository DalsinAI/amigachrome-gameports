/* SPDX-License-Identifier: GPL-3.0-or-later
 * Restoration policy adapted from Thinker base.cpp::mod_base_support and
 * engine_base.h::BaseSupportCosts, MIT, Copyright (c) Thinker Mod authors.
 * Pin dabe62f2d1abba03de40f3dd3048494843f6a9af; see THINKER-LICENSE.txt.
 * This is an explicit default-policy adaptation, not recovered original code.
 */
#ifndef ALPHA_BASE_SUPPORT_H
#define ALPHA_BASE_SUPPORT_H
#include <stdint.h>
#include <stddef.h>
namespace alpha {
enum class SupportError {ok,bad_state,bad_buffer,unsupported,out_of_memory,overflow};
struct SupportUnit {
 uint32_t state;
 int32_t plan,offense,triad;
 bool homed_here,clean_reactor,native_on_fungus,outside_own_territory_and_base;
};
struct SupportFacts {int32_t support_pending,police_pending,population;};
struct SupportResult {
 int32_t supported,paid_units,mineral_cost,pacifism_units,drone_units,free_skips;
 int32_t free_allowance,cost_per_paid_unit;
};
/* Includes formers/colony/transport but excludes plans above Terraform (9).
 * Clean/native fungus units do not consume free support slots. Pacifism is
 * independent of clean/native mineral exemptions; air-superiority aircraft
 * retain the ordinary outside-territory test. Input order is vehicle order.
 * Clears/recomputes only the three support/pacifism bits on home units.
 * Returns projected states, never changes units or applies faction aggregates.
 * No AI bonus, configurable crawler support, combat tally or convoy side effect.
 * All source/output storage must be disjoint. Any error preserves both outputs.
 */
SupportError base_support_policy(const SupportFacts&,const SupportUnit*,size_t,
 SupportResult&,uint32_t* projected_states,size_t capacity);
}
#endif
