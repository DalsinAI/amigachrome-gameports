/* SPDX-License-Identifier: GPL-3.0-or-later. See FIELD-COMBAT.md for scope. */
#ifndef ALPHA_WORLD_FIELD_COMBAT_H
#define ALPHA_WORLD_FIELD_COMBAT_H
#include "combat_rounds.h"
#include "world_unit_stats.h"
namespace alpha {
struct FieldCombatFacts {
 CombatDuel duel;
 uint32_t attacker_morale,defender_morale;
 int32_t terrain_defense,attacker_moves_left;
};
/* Read-only, bounded base-game human field engagement proposal. Uses CURRENT
 * world records and recovered effective morale, not saved displayed strength.
 * Infantry only; ordinary conventional combat designs with no abilities;
 * exactly one unit on each tile, adjacent land, no bases/bunkers/fungus.
 * Nearby sensor/GSP defense is derived with source-default territory and
 * nearest-global-base rules. Human active attacker, mutual vendetta; ordinary
 * faction-owned defenders may be human or AI. Native faction0 is excluded.
 * Equal infantry speeds preclude retreat. Derives hasty/terrain/elevation/road
 * and fanatic/faction offense/defense effects under documented Thinker default
 * restoration policy. No seed inference, movement spending or authority token.
 * Nonqualifying engagements reject without changing output or world.
 */
CombatError world_field_combat(const OriginalWorld&,const UnitDefinitions&,const Rules&,
 const BasicRules&,size_t attacker,size_t defender,FieldCombatFacts&);
CombatError world_resolve_field_combat(const OriginalWorld&,const UnitDefinitions&,const Rules&,
 const BasicRules&,size_t attacker,size_t defender,CombatRollStream&,FieldCombatFacts&,CombatResult&);
}
#endif
