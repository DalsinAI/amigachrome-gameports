/* SPDX-License-Identifier: GPL-3.0-or-later
 * Placeholder entry points for WP3 (unit orders, movement, terraforming).
 * Written by engine/tools/gen_stubs.py from engine_wp.h. Each function raises
 * EV_NOT_IMPLEMENTED and returns a neutral value. WP3 deletes this file
 * when it lands its real implementation.
 */
#include "../engine.h"

namespace alpha {
namespace engine {

int __cdecl order_veh(int UNUSED(veh_id), int UNUSED(offset), int UNUSED(flag)) {
    not_implemented("order_veh");
    return 0;
}

int __cdecl veh_speed(int UNUSED(veh_id), int UNUSED(skip_morale)) {
    not_implemented("veh_speed");
    return 0;
}

int __cdecl proto_speed(int UNUSED(unit_id)) {
    not_implemented("proto_speed");
    return 0;
}

int __cdecl mod_hex_cost(int UNUSED(unit_id), int UNUSED(faction_id), int UNUSED(x1), int UNUSED(y1), int UNUSED(x2), int UNUSED(y2), int UNUSED(toggle)) {
    not_implemented("mod_hex_cost");
    return 0;
}

int __cdecl terraform_cost(int UNUSED(x), int UNUSED(y), int UNUSED(faction_id)) {
    not_implemented("terraform_cost");
    return 0;
}

int __cdecl zoc_any(int UNUSED(x), int UNUSED(y), int UNUSED(faction_id)) {
    not_implemented("zoc_any");
    return 0;
}

int __cdecl zoc_move(int UNUSED(x), int UNUSED(y), int UNUSED(faction_id)) {
    not_implemented("zoc_move");
    return 0;
}

int __cdecl zoc_veh(int UNUSED(x), int UNUSED(y), int UNUSED(faction_id)) {
    not_implemented("zoc_veh");
    return 0;
}

int __cdecl zoc_sea(int UNUSED(x), int UNUSED(y), int UNUSED(faction_id)) {
    not_implemented("zoc_sea");
    return 0;
}

int __cdecl goody_box(int UNUSED(veh_id)) {
    not_implemented("goody_box");
    return 0;
}

int __cdecl mod_goody_at(int UNUSED(x), int UNUSED(y)) {
    not_implemented("mod_goody_at");
    return 0;
}

int __cdecl mod_bonus_at(int UNUSED(x), int UNUSED(y)) {
    not_implemented("mod_bonus_at");
    return 0;
}

int __cdecl mod_minerals_at(int UNUSED(x), int UNUSED(y)) {
    not_implemented("mod_minerals_at");
    return 0;
}

void __cdecl monolith(int UNUSED(veh_id)) {
    not_implemented("monolith");
}

int __cdecl want_monolith(int UNUSED(veh_id)) {
    not_implemented("want_monolith");
    return 0;
}

void __cdecl spot_all(int UNUSED(veh_id), int UNUSED(flag)) {
    not_implemented("spot_all");
}

void __cdecl spot_base(int UNUSED(base_id), int UNUSED(faction_id)) {
    not_implemented("spot_base");
}

void __cdecl spot_stack(int UNUSED(veh_id), int UNUSED(faction_id)) {
    not_implemented("spot_stack");
}

void __cdecl unspot_stack(int UNUSED(veh_id)) {
    not_implemented("unspot_stack");
}

void __cdecl spot_loc(int UNUSED(x), int UNUSED(y), int UNUSED(faction_id)) {
    not_implemented("spot_loc");
}

int __cdecl veh_wake(int UNUSED(veh_id)) {
    not_implemented("veh_wake");
    return 0;
}

void __cdecl veh_skip(int UNUSED(veh_id)) {
    not_implemented("veh_skip");
}

void __cdecl wake_stack(int UNUSED(veh_id)) {
    not_implemented("wake_stack");
}

void __cdecl alt_set(int UNUSED(x), int UNUSED(y), int UNUSED(altitude)) {
    not_implemented("alt_set");
}

void __cdecl alt_set_both(int UNUSED(x), int UNUSED(y), int UNUSED(altitude)) {
    not_implemented("alt_set_both");
}

void __cdecl world_alt_set(int UNUSED(x), int UNUSED(y), int UNUSED(altitude), int UNUSED(toggle)) {
    not_implemented("world_alt_set");
}

void __cdecl world_raise_alt(int UNUSED(x), int UNUSED(y)) {
    not_implemented("world_raise_alt");
}

void __cdecl world_lower_alt(int UNUSED(x), int UNUSED(y)) {
    not_implemented("world_lower_alt");
}

int __cdecl action_terraform(int UNUSED(veh_id), int UNUSED(item_id), int UNUSED(toggle)) {
    not_implemented("action_terraform");
    return 0;
}

int __cdecl action_airdrop(int UNUSED(veh_id), int UNUSED(tx), int UNUSED(ty), int UNUSED(flags)) {
    not_implemented("action_airdrop");
    return 0;
}

void __cdecl action_destroy(int UNUSED(veh_id), int UNUSED(tgt_item), int UNUSED(tgt_x), int UNUSED(tgt_y)) {
    not_implemented("action_destroy");
}

int __cdecl contribution(int UNUSED(veh_id), int UNUSED(frm_id)) {
    not_implemented("contribution");
    return 0;
}

int __cdecl find_return_base(int UNUSED(veh_id)) {
    not_implemented("find_return_base");
    return 0;
}

} // namespace engine
} // namespace alpha
