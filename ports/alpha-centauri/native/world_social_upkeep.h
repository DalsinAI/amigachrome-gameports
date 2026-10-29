/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_SOCIAL_UPKEEP_H
#define ALPHA_WORLD_SOCIAL_UPKEEP_H
#include "social_upkeep.h"
#include "original_world.h"
namespace alpha {
/* Read-only binding for factions1..7. Same-edition qualified SOCIO table,
 * explicit profile and original World required. Derives pending selections,
 * current BASE effects, ordered actual faction bonuses, project locations/base
 * owners and the actual map temple/visibility/owner condition. No saved pending
 * effect feedback or compute_faction_modifiers substitution. Reject positive
 * project references outside the World; every negative location is absent as
 * in the original social_calc branches. Entire World/rules/output disjoint.
 * Does not apply the proposal or start social/repair/production/turn phases.
 */
LogicError world_social_upkeep(const OriginalWorld&,const SocialRules&,Variant,int32_t faction,SocialUpkeepResult&);
}
#endif
