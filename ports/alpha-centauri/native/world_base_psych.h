/* SPDX-License-Identifier: GPL-3.0-or-later. Policy attribution in upkeep_base_psych.h. */
#ifndef ALPHA_WORLD_BASE_PSYCH_H
#define ALPHA_WORLD_BASE_PSYCH_H
#include "upkeep_base_psych.h"
#include "original_world.h"
#include "rules_text.h"
namespace alpha {
struct WorldPsychStage {int32_t specialists,fresh_psych;};
struct WorldPsychResult {PsychFacts facts;PsychResult psych;};
/* Current base-game human psych proposal. Specialist count and psych energy
 * must come from the same fresh proposed assignment/allocation. Derives fresh
 * support/pacifism, real police-unit plans/triads/abilities, owned base count,
 * map root, actual facilities/projects/difficulty/faction bonuses. Saved drone,
 * talent, police, support or accounting totals are not consumed. Explicit
 * Thinker default restoration policy, not recovered original base_psych.
 * All output/input/world regions disjoint; every failure preserves output.
 */
UpkeepPsychError world_base_psych_policy(const OriginalWorld&,const UnitDefinitions&,
 const BasicRules&,size_t base,const WorldPsychStage&,WorldPsychResult&);
}
#endif
