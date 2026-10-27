/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_COMBAT_CRT_H
#define ALPHA_WORLD_COMBAT_CRT_H
#include "world_field_combat.h"
#include "combat_crt.h"
namespace alpha {
/* Current-world bounded field facts + separate explicit CRT state. Atomic
 * facts/result/state proposal; no attack commit, casualty removal or save.
 * Same legality/support boundaries as world_field_combat. */
CombatError world_resolve_seeded_field_combat(const OriginalWorld&,const UnitDefinitions&,
 const Rules&,const BasicRules&,size_t attacker,size_t defender,
 CombatRandomState&,FieldCombatFacts&,CombatResult&);
}
#endif
