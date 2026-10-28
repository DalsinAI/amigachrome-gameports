/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in combat_factions.h. */
#ifndef AC_ALPHA_WORLD_COMBAT_FACTIONS_H
#define AC_ALPHA_WORLD_COMBAT_FACTIONS_H
#include "combat_factions.h"
namespace alpha {
/* Read-only normal flags0/non-Probe attacker inspection. Actual components,
 * edition missile/Spore psi classification and actual faction/airdrop/Planet plus design-owner FREEABIL/technology
 * facts. Explicit post-Empath offense and pre-terrain defense inputs retain distinct phases.
 * No complete battle/mode/session/attack authority or cache/UI/World/RNG writes.
 */
LogicError world_combat_faction_modifiers(const OriginalWorld&,const UnitDefinitions&,
 const Rules&,const BasicRules&,size_t attacker,size_t defender,int32_t offense,
 int32_t defense,CombatFactionStrengths&);
}
#endif
