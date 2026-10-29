/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in defense_abilities.h. */
#ifndef AC_ALPHA_WORLD_DEFENSE_ABILITIES_H
#define AC_ALPHA_WORLD_DEFENSE_ABILITIES_H
#include "defense_abilities.h"
namespace alpha {
/* Current owned actors/prototypes, raw chassis speed, edition and real
 * design-owner FREEABIL prerequisite facts. Incoming defense stays explicit.
 * No final battle, live session authority, labels/caches/actions or World write.
 */
LogicError world_defense_abilities(const OriginalWorld&,const UnitDefinitions&,
 const Rules&,const BasicRules&,size_t attacker,size_t defender,int32_t defense,
 DefenseAbilitiesStrength&);
}
#endif
