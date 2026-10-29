/* SPDX-License-Identifier: GPL-3.0-or-later
 * Round loop adapted from Thinker veh_combat.cpp, MIT, pin
 * dabe62f2d1abba03de40f3dd3048494843f6a9af; see THINKER-LICENSE.txt.
 */
#ifndef ALPHA_COMBAT_ROUNDS_H
#define ALPHA_COMBAT_ROUNDS_H
#include <stddef.h>
#include <stdint.h>
namespace alpha {
enum class CombatError {ok,bad_state,bad_buffer,unsupported,overflow,short_random_stream};
struct CombatDuel {
 int32_t offense,defense;
 int32_t attacker_reactor,defender_reactor;
 int32_t attacker_damage,defender_damage;
};
struct CombatRollStream {
 const uint16_t* values;size_t count,cursor;
};
struct CombatResult {
 int32_t attacker_damage,defender_damage,attacker_hp,defender_hp;
 uint32_t rounds,attacker_hits,defender_hits,random_draws;
 bool attacker_won;
};
/* Conventional, non-retreating, non-artillery two-unit round loop. Strengths
 * are already-final combat values. Default reactor policy: each lost round
 * adds ONE damage, reactor r supplies 10*r HP. This is deliberately not the
 * ignore_reactor_power mod option and not psi combat. Two independent CRT
 * draws are reduced modulo offense/defense; ties hit the attacker. A strength
 * <=1 has zero roll and consumes no draw. Original game_rand is a different
 * stream from the recovered integer_random movement generator.
 * Caller provides recorded/engine-owned 15-bit draws with explicit cursor;
 * this resolver does not invent an original combat RNG seed or infer one from
 * saved tick/map state. Successful resolution advances that cursor atomically.
 * Short/invalid stream, bad input or alias leaves cursor and output unchanged.
 * No kill/removal, movement, promotion, capture or world mutation is performed.
 */
CombatError resolve_combat_rounds(const CombatDuel&,CombatRollStream&,CombatResult&);
const char* combat_error_name(CombatError);
}
#endif
