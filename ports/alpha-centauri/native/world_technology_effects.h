/* SPDX-License-Identifier: GPL-3.0-or-later
 * Owned-world adaptation of OpenSMACX technology.cpp::tech_effects (005BAE60),
 * Brendan Casey (C) 2013-2021, pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 */
#ifndef ALPHA_WORLD_TECHNOLOGY_EFFECTS_H
#define ALPHA_WORLD_TECHNOLOGY_EFFECTS_H
#include "original_world.h"
#include "rules_text.h"
namespace alpha {
enum class TechnologyEffectsError {ok,bad_state,bad_buffer,overflow};
struct TechnologyEffects {int32_t fungus[4],commerce;};
/* Recompute from actual technology ownership, parsed flags, faction bonuses
 * and pending economy. Never consumes saved tech_fungus/commerce cache values.
 * A read-only proposal: failure preserves output and every owned record.
 * Signed intermediates outside int32 reject instead of wrapping.
 */
TechnologyEffectsError world_technology_effects(const OriginalWorld&,const Rules&,
 int32_t faction,TechnologyEffects&);
}
#endif
