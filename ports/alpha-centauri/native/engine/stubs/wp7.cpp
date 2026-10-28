/* SPDX-License-Identifier: GPL-3.0-or-later
 * Placeholder entry points for WP7 (new game and world generation).
 * Written by engine/tools/gen_stubs.py from engine_wp.h. Each function raises
 * EV_NOT_IMPLEMENTED and returns a neutral value. WP7 deletes this file
 * when it lands its real implementation.
 */
#include "../engine.h"

namespace alpha {
namespace engine {

void world_generate(uint32_t UNUSED(seed)) {
    not_implemented("world_generate");
}

void __cdecl mod_world_build() {
    not_implemented("mod_world_build");
}

void __cdecl find_start(int UNUSED(faction_id), int* UNUSED(tx), int* UNUSED(ty)) {
    not_implemented("find_start");
}

int __cdecl mod_setup_player(int UNUSED(faction_id), int UNUSED(setup_id), int UNUSED(is_probe)) {
    not_implemented("mod_setup_player");
    return 0;
}

void __cdecl setup_game(int UNUSED(flag)) {
    not_implemented("setup_game");
}

void init_world_config() {
    not_implemented("init_world_config");
}

void __cdecl fixup_landmarks() {
    not_implemented("fixup_landmarks");
}

void __cdecl world_linearize_contours() {
    not_implemented("world_linearize_contours");
}

} // namespace engine
} // namespace alpha
