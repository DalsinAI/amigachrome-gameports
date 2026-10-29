/* SPDX-License-Identifier: GPL-3.0-or-later. Original battle_fight_2/proto_power/
 * veh_health and CRT rand arithmetic. OpenSMACX pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c,
 * Brendan Casey2013-2021. Original instruction behavior takes precedence. */
#ifndef AC_ALPHA_COMBAT_ROUND_H
#define AC_ALPHA_COMBAT_ROUND_H
#include "terrain_defense.h"
namespace alpha {
struct CrtRandomDraw {uint32_t next_state,value;};
/* The original CRT stream is distinct from movement's1664525 LCG. This is an
 * explicit state transition, not initialization or a per-combat/per-unit reset.
 */
CrtRandomDraw original_crt_random_step(uint32_t state);
struct CombatHealthContext {uint32_t plan,reactor,damage;};
struct CombatHealth {uint32_t power,remaining;};
LogicError combat_health(const CombatHealthContext&,CombatHealth&);
struct CombatRoundContext {int32_t offense,defense;bool psi;CombatHealthContext attacker,defender;uint32_t crt_state;};
struct CombatRoundResult {uint32_t next_crt_state,draw_count,attacker_roll,defender_roll,attacker_damage,defender_damage,attacker_health,defender_health,hit_side;};
/* Numeric single-round stage only, both actors already selected. Nonnegative
 * strengths; explicit CRT state; initial health zero means no draws/hit. Ties
 * select attacker as recipient. hit_side0 none/1attacker/2defender is a branch,
 * not proof bytes changed (raw reactor0 psi increment may be zero). Byte damage
 * wraps exactly; no World/actor/RNG commit, legal attack or complete fight.
 */
LogicError combat_round_preview(const CombatRoundContext&,CombatRoundResult&);
}
#endif
