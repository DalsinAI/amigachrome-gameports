/* SPDX-License-Identifier: GPL-3.0-or-later
 * Placeholder entry points for WP8 (diplomacy, council, victory, score).
 * Written by engine/tools/gen_stubs.py from engine_wp.h. Each function raises
 * EV_NOT_IMPLEMENTED and returns a neutral value. WP8 deletes this file
 * when it lands its real implementation.
 */
#include "../engine.h"

namespace alpha {
namespace engine {

void __cdecl treaty_on(int UNUSED(faction_id_1), int UNUSED(faction_id_2), uint32_t UNUSED(status)) {
    not_implemented("treaty_on");
}

void __cdecl treaty_off(int UNUSED(faction_id_1), int UNUSED(faction_id_2), uint32_t UNUSED(status)) {
    not_implemented("treaty_off");
}

void __cdecl set_treaty(int UNUSED(faction_id_1), int UNUSED(faction_id_2), uint32_t UNUSED(status), bool UNUSED(add)) {
    not_implemented("set_treaty");
}

void __cdecl atrocity(int UNUSED(faction_id), int UNUSED(faction_id_tgt), int UNUSED(skip_init_check), int UNUSED(skip_human_check)) {
    not_implemented("atrocity");
}

int __cdecl get_mood(int UNUSED(friction)) {
    not_implemented("get_mood");
    return 0;
}

void __cdecl encounter(int UNUSED(faction_id_1), int UNUSED(faction_id_2), int UNUSED(x), int UNUSED(y), int UNUSED(fungal_tower), int UNUSED(native_unit)) {
    not_implemented("encounter");
}

void __cdecl enemy_diplomacy(int UNUSED(faction_id)) {
    not_implemented("enemy_diplomacy");
}

void __cdecl diplomacy_check(int UNUSED(faction_id), int UNUSED(faction_id_2), int UNUSED(full_check)) {
    not_implemented("diplomacy_check");
}

int __cdecl end_of_game(int UNUSED(flag)) {
    not_implemented("end_of_game");
    return 0;
}

int __cdecl mod_eliminate_player(int UNUSED(faction_id), int UNUSED(setup_id)) {
    not_implemented("mod_eliminate_player");
    return 0;
}

int __cdecl transcending(int UNUSED(faction_id)) {
    not_implemented("transcending");
    return 0;
}

int __cdecl corner_market(int UNUSED(faction_id)) {
    not_implemented("corner_market");
    return 0;
}

int __cdecl is_objective(int UNUSED(base_id)) {
    not_implemented("is_objective");
    return 0;
}

} // namespace engine
} // namespace alpha
