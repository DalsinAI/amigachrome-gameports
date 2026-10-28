/* SPDX-License-Identifier: GPL-3.0-or-later
 * Placeholder entry points for WP1 (turn scheduler and End Turn).
 * Written by engine/tools/gen_stubs.py from engine_wp.h. Each function raises
 * EV_NOT_IMPLEMENTED and returns a neutral value. WP1 deletes this file
 * when it lands its real implementation.
 */
#include "../engine.h"

namespace alpha {
namespace engine {

void __cdecl control_turn() {
    not_implemented("control_turn");
}

void __cdecl turn_upkeep() {
    not_implemented("turn_upkeep");
}

void __cdecl faction_upkeep(int UNUSED(faction_id)) {
    not_implemented("faction_upkeep");
}

void __cdecl production_phase(int UNUSED(faction_id)) {
    not_implemented("production_phase");
}

void __cdecl repair_phase(int UNUSED(faction_id)) {
    not_implemented("repair_phase");
}

void __cdecl allocate_energy(int UNUSED(faction_id)) {
    not_implemented("allocate_energy");
}

int __cdecl next_player_turn() {
    not_implemented("next_player_turn");
    return 0;
}

void __cdecl social_upkeep(int UNUSED(faction_id)) {
    not_implemented("social_upkeep");
}

int __cdecl social_upheaval(int UNUSED(faction_id), CSocialCategory* UNUSED(choices)) {
    not_implemented("social_upheaval");
    return 0;
}

void __cdecl tech_research(int UNUSED(faction_id), int UNUSED(value)) {
    not_implemented("tech_research");
}

void __cdecl rankings(int UNUSED(flag)) {
    not_implemented("rankings");
}

int __cdecl game_year(int turn) {
    return Rules->normal_start_year + turn; // Thinker game.cpp, exact
}

void __cdecl do_fungal_towers() {
    not_implemented("do_fungal_towers");
}

void __cdecl reset_territory() {
    not_implemented("reset_territory");
}

void __cdecl mod_time_warp() {
    not_implemented("mod_time_warp");
}

void __cdecl mod_auto_save() {
    not_implemented("mod_auto_save");
}

} // namespace engine
} // namespace alpha
