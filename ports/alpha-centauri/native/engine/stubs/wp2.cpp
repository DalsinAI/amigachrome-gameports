/* SPDX-License-Identifier: GPL-3.0-or-later
 * Placeholder entry points for WP2 (base upkeep, production, growth, riots).
 * Written by engine/tools/gen_stubs.py from engine_wp.h. Each function raises
 * EV_NOT_IMPLEMENTED and returns a neutral value. WP2 deletes this file
 * when it lands its real implementation.
 */
#include "../engine.h"

namespace alpha {
namespace engine {

int __cdecl mod_base_upkeep(int UNUSED(base_id)) {
    not_implemented("mod_base_upkeep");
    return 0;
}

int __cdecl mod_base_production() {
    not_implemented("mod_base_production");
    return 0;
}

int __cdecl mod_base_build(int UNUSED(base_id), int UNUSED(has_gov)) {
    not_implemented("mod_base_build");
    return 0;
}

void __cdecl base_first(int UNUSED(base_id)) {
    not_implemented("base_first");
}

void __cdecl base_compute(int update_prev) {
    not_implemented("base_compute");
    if (update_prev || *ComputeBaseID != *CurrentBaseID) {
        *ComputeBaseID = *CurrentBaseID;
    }
}

void __cdecl mod_base_change(int UNUSED(base_id), int UNUSED(item_id)) {
    not_implemented("mod_base_change");
}

int __cdecl mod_base_making(int UNUSED(item_id), int UNUSED(base_id)) {
    not_implemented("mod_base_making");
    return 0;
}

int __cdecl mod_base_hurry() {
    not_implemented("mod_base_hurry");
    return 0;
}

int __cdecl mod_cost_factor(int UNUSED(faction_id), BaseResType UNUSED(type), int UNUSED(base_id)) {
    not_implemented("mod_cost_factor");
    return 10;
}

void __cdecl mod_base_support() {
    not_implemented("mod_base_support");
}

void __cdecl mod_base_yield() {
    not_implemented("mod_base_yield");
}

void __cdecl mod_base_nutrient() {
    not_implemented("mod_base_nutrient");
}

void __cdecl mod_base_minerals() {
    not_implemented("mod_base_minerals");
}

void __cdecl mod_base_energy() {
    not_implemented("mod_base_energy");
}

void __cdecl mod_base_psych(int UNUSED(base_id), int UNUSED(SE_talent), int UNUSED(SE_police)) {
    not_implemented("mod_base_psych");
}

int __cdecl mod_base_growth() {
    not_implemented("mod_base_growth");
    return 0;
}

void __cdecl mod_drone_riot() {
    not_implemented("mod_drone_riot");
}

void __cdecl mod_base_drones() {
    not_implemented("mod_base_drones");
}

void __cdecl mod_base_maint() {
    not_implemented("mod_base_maint");
}

void __cdecl mod_base_ecology() {
    not_implemented("mod_base_ecology");
}

void __cdecl mod_base_research() {
    not_implemented("mod_base_research");
}

void __cdecl mod_name_base(int UNUSED(faction_id), char* UNUSED(name), bool UNUSED(save_offset), bool UNUSED(water)) {
    not_implemented("mod_name_base");
}

void __cdecl mod_capture_base(int UNUSED(base_id), int UNUSED(faction_id), int UNUSED(is_probe)) {
    not_implemented("mod_capture_base");
}

void __cdecl mod_base_reset(int UNUSED(base_id), int UNUSED(has_gov)) {
    not_implemented("mod_base_reset");
}

int __cdecl resource_yield(BaseResType UNUSED(type), int UNUSED(faction_id), int UNUSED(base_id), int UNUSED(x), int UNUSED(y)) {
    not_implemented("resource_yield");
    return 0;
}

int __cdecl mod_crop_yield(int UNUSED(faction_id), int UNUSED(base_id), int UNUSED(x), int UNUSED(y), int UNUSED(flag)) {
    not_implemented("mod_crop_yield");
    return 0;
}

int __cdecl mod_mine_yield(int UNUSED(faction_id), int UNUSED(base_id), int UNUSED(x), int UNUSED(y), int UNUSED(flag)) {
    not_implemented("mod_mine_yield");
    return 0;
}

int __cdecl mod_energy_yield(int UNUSED(faction_id), int UNUSED(base_id), int UNUSED(x), int UNUSED(y), int UNUSED(flag)) {
    not_implemented("mod_energy_yield");
    return 0;
}

int __cdecl mod_black_market(int UNUSED(base_id), int UNUSED(energy)) {
    not_implemented("mod_black_market");
    return 0;
}

int __cdecl facility_avail(FacilityId UNUSED(item_id), int UNUSED(faction_id), int UNUSED(base_id), int UNUSED(queue_count)) {
    not_implemented("facility_avail");
    return 0;
}

void find_relocate_base(int UNUSED(faction_id)) {
    not_implemented("find_relocate_base");
}

} // namespace engine
} // namespace alpha
