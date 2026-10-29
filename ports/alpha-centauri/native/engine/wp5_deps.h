/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP5: functions the research and design code calls that Thinker Mod defines
 * in files owned by other packages (base.cpp, monument.cpp, game.cpp,
 * plan.cpp at dabe62f2d1abba03de40f3dd3048494843f6a9af, MIT; see
 * engine/THINKER-LICENSE) and that engine_wp.h does not declare.
 * engine/wp5_deps.cpp defines them as weak symbols (ported verbatim where the
 * dependencies exist, otherwise neutral placeholders that raise
 * EV_NOT_IMPLEMENTED) so the owning package supplies the strong definition
 * without a link conflict. Include after engine.h.
 */
#pragma once

namespace alpha {
namespace engine {

/* base.cpp (WP2) */
void __cdecl mod_bases_reset(int region, int faction_id, int defend_only);
int unit_support_plan();
bool base_can_riot(int base_id, bool allow_staple);
/* monument.cpp (WP2) */
void __cdecl mon_secrets_of_tech(int faction_id);
/* game.cpp */
bool full_game_turn();
/* plan.cpp (WP6) */
bool need_police(int faction_id);
/* Original interface flag dword_90EA3C ("upgrade all without asking"),
 * shared with the base production code (WP2). */
extern int* dword_90EA3C;

} // namespace engine
} // namespace alpha
