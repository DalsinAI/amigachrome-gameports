/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independently owned adaptation of OpenSMACX technology.cpp::tech_effects,
 * Brendan Casey (C)2013-2021, and Thinker tech.cpp. Pins/evidence in
 * TECHNOLOGY-EFFECTS.md. No proprietary code/table data is distributed.
 */
#ifndef ALPHA_TECHNOLOGY_EFFECTS_H
#define ALPHA_TECHNOLOGY_EFFECTS_H
#include "game_logic.h"
#include "original_world.h"
namespace alpha {
struct TechnologyEffectsFacts {
 int32_t faction,rule_commerce,pending_economy,bonus_count;
 uint8_t owners[technology_count];
 int32_t bonus_kind[8],bonus_value[8];
};
struct TechnologyEffectsResult {
 int32_t commerce,fungus_nutrient,fungus_mineral,fungus_energy,fungus_unused;
};
/* Complete callback-free original recomputation. Qualified same-profile Rules
 * are required. Known-tech semantics ignore faction0/future88 and retain
 * None/Disable asymmetry. Edition does not insert extra tech exclusions here.
 * Output must be disjoint from the complete Rules/Facts ranges. Overlap or
 * wrapping address extents reject before writes. Every signed addition is
 * checked; errors preserve output. Does not award a
 * tech, mutate a world, refresh a tile, or execute income/turn/event callbacks.
 */
LogicError recompute_technology_effects(const Rules&,const TechnologyEffectsFacts&,TechnologyEffectsResult&);
/* Binds actual decoded owner bits, local rules/bonuses and pending SE economy.
 * Saved effect counters are not inputs. Output must be disjoint from Rules,
 * the complete World object, raw allocation and every decoded map/actor/prototype/faction/definition
 * allocation; even partial range overlap rejects before writes. Original wire
 * and world stay unchanged.
 */
LogicError world_technology_effects(const OriginalWorld&,const Rules&,int32_t faction,TechnologyEffectsResult&);
}
#endif
