/* SPDX-License-Identifier: GPL-3.0-or-later
 * Integration of the engine packages: wires the WP1 turn hooks (turn.h) to
 * the routines of packages that have landed. Hooks whose owner has not landed
 * stay null, and WP1 skips them:
 *   interlude (UI), energy_limit,
 *   random_base_event (executable only).
 * Installed:
 *   mon_tech_discovered -> WP2 base_monument.cpp (Thinker monument.cpp);
 *   consider_designs    -> WP5 design.cpp (time warp);
 *   world_climate       -> WP7 (Thinker map.cpp);
 *   crash_landing       -> WP7 newgame.cpp (turn 0 landing popup);
 *   pact_ends, can_call_council, call_council -> WP8 install_wp8_turn_hooks();
 *   plans_upkeep, probe_upkeep, move_upkeep -> WP6 ai_api.h.
 * engine_reset_runtime() clears the WP8 diplomacy runtime and the WP6 AI
 * caches on load and new game.
 * The WP6 routines that other packages call in alpha::engine (add_goal,
 * clear_goals, contiguous, need_police, reset_state) are defined here as
 * forwarders to their alpha::engine::ai bodies; the packages' weak
 * placeholders were removed.
 */
#include "engine.h"
#include "turn.h"
#include "base_wp2.h"
#include "design.h"
#include "newgame.h"
#include "wp3_deps.h"
#include "diplo.h"
#include "ai.h"
#include "wp5_deps.h"
#include "wp7_deps.h"
#include "integration.h"

namespace alpha {
namespace engine {

void engine_install_turn_hooks() {
    TurnHooks& h = turn_hooks();
    h.mon_tech_discovered = mon_tech_discovered;
    h.consider_designs = consider_designs;
    h.world_climate = world_climate;
    h.crash_landing = crash_landing;
    install_wp8_turn_hooks();
    h.plans_upkeep = ai_plans_upkeep;
    h.probe_upkeep = ai_probe_upkeep;
    h.move_upkeep = ai_move_upkeep;
}

void engine_reset_runtime() {
    diplo_runtime_reset();
    ai_reset_state();
}

void __cdecl add_goal(int faction_id, int type, int priority, int x, int y, int base_id) {
    ai::add_goal(faction_id, type, priority, x, y, base_id);
}

void __cdecl clear_goals(int faction_id) {
    ai::clear_goals(faction_id);
}

int __cdecl contiguous(int faction_id_1, int faction_id_2, int region, int flag) {
    return ai::contiguous(faction_id_1, faction_id_2, region, flag);
}

bool need_police(int faction_id) {
    return ai::need_police(faction_id);
}

void reset_state() {
    ai::reset_state();
}

/* Installed once at start-up so every program that links the engine gets the
 * same turn behaviour; turn_hooks() is zero-initialised before this runs. */
[[maybe_unused]] static const bool s_hooks_installed = (engine_install_turn_hooks(), true);

} // namespace engine
} // namespace alpha
