/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_BASE_SUPPORT_H
#define ALPHA_WORLD_BASE_SUPPORT_H
#include "base_support.h"
#include "original_world.h"
#include "rules_text.h"
namespace alpha {
/* Connected, read-only base-game HUMAN restoration policy. Inputs come from
 * pending SE values, population, current vehicle home/order, actual prototype
 * component tables, raw clean ability and current tile ownership/fungus.
 * Base-game POLICE has no Brood Pit modifier. No saved mineral maintenance,
 * saved support flags, saved consumption, or saved pacifism totals are reused.
 * Rejects expansion and AI rather than inventing their grants/bonuses.
 * Output projected states are indexed by the complete world vehicle array.
 * Every world/definition/output region must be disjoint; errors preserve both
 * outputs. This is a calculation preview, not authority/upkeep mutation.
 */
SupportError world_base_support_policy(const OriginalWorld&,const UnitDefinitions&,
 size_t base_id,SupportResult&,uint32_t* projected_states,size_t capacity);
}
#endif
