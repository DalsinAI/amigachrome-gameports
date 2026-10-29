/* SPDX-License-Identifier: GPL-3.0-or-later
 * Vehicle/stack functions (Thinker Mod src/veh.h subset, MIT; see
 * engine/THINKER-LICENSE). Movement, orders, upgrades, spotting and combat are
 * declared in engine_wp.h and owned by WP3/WP4/WP5.
 * Include engine.h rather than this header directly.
 */
#pragma once

namespace alpha {
namespace engine {

enum EnemyVehMove { // return codes for enemy_veh processing (Thinker veh_turn.h)
    VEH_SYNC = 0,
    VEH_SKIP = 1,
};

int __cdecl can_arty(int unit_id, bool allow_sea_arty);
int __cdecl has_abil(int unit_id, VehAblFlag ability);
int __cdecl arty_range(int unit_id);
int __cdecl drop_range(int faction_id);
bool has_orbital_drops(int faction_id);
bool has_ability(int faction_id, VehAbl abl, VehChassis chs, VehWeapon wpn);
bool is_native_unit(int unit_id);
bool is_battle_ogre(int unit_id);
bool can_repair(int unit_id);
bool can_monolith(int unit_id);
int __cdecl veh_at(int x, int y);
int __cdecl veh_who(int x, int y);
int __cdecl veh_top(int veh_id);
void __cdecl stack_put(int veh_id, int x, int y);
void __cdecl stack_sort(int veh_id);
void __cdecl stack_sort_2(int veh_id);
int __cdecl stack_fix(int veh_id);
int __cdecl stack_veh(int veh_id, int flag);
void __cdecl stack_kill(int veh_id);
int __cdecl veh_lift(int veh_id);
int __cdecl veh_drop(int veh_id, int x, int y);
void __cdecl veh_put(int veh_id, int x, int y);
void __cdecl sleep(int veh_id);
void __cdecl veh_demote(int veh_id);
void __cdecl veh_promote(int veh_id);
void __cdecl veh_clear(int veh_id, int unit_id, int faction_id);
int __cdecl veh_init(int unit_id, int faction_id, int x, int y);
int __cdecl veh_init_free(int unit_id, int faction_id, int x, int y);
int __cdecl mod_veh_kill(int veh_id);
void __cdecl veh_kill(int veh_id);
void __cdecl kill(int veh_id);
int __cdecl veh_cargo(int veh_id);
int __cdecl arm_strat(int armor_id, int faction_id);
int __cdecl weap_strat(int weapon_id, int faction_id);
int __cdecl weap_val(int unit_id, int faction_id);
int __cdecl arm_val(int armor_id, int faction_id);
int __cdecl armor_val(int unit_id, int faction_id);
int __cdecl stack_check(int veh_id, int type, int cond1, int cond2, int cond3);
int __cdecl veh_jail(int veh_id);
int __cdecl veh_fake(int unit_id, int faction_id);

} // namespace engine
} // namespace alpha
