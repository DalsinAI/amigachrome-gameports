/* SPDX-License-Identifier: GPL-3.0-or-later
 * Functions WP7 calls that other packages (or the executable) own but that
 * are not declared in engine_wp.h. Signatures are Thinker Mod's (dabe62f2,
 * MIT; see engine/THINKER-LICENSE). wp7_deps.cpp defines each one as a WEAK
 * fallback, so the owning package's ordinary definition replaces it at link
 * time without any edit here (ENGINE-WP7.md lists each fallback).
 * Include engine.h first.
 */
#pragma once

namespace alpha {
namespace engine {

void reset_state();                                                     // WP6 (Thinker plan.cpp)
void __cdecl clear_goals(int faction_id);                               // WP6 (Thinker goal.cpp)
void __cdecl clear_bunglist(int faction_id);                            // exe-only (AI bunglist)
void __cdecl consider_designs(int faction_id);                          // exe-only, WP5/WP6
void __cdecl clear_monuments();                                         // WP2 (monument)
void __cdecl agenda_on(int faction_id_1, int faction_id_2, uint32_t status);              // WP8
void __cdecl set_agenda(int faction_id_1, int faction_id_2, uint32_t status, int add);    // WP8
bool can_build_base(int x, int y, int faction_id, int triad);           // WP6 (Thinker move.cpp)
void __cdecl balance();                                                 // exe-only, skipped by default
extern int* GameVictoryType;                                            // WP8 victory

} // namespace engine
} // namespace alpha
