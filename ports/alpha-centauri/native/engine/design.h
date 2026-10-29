/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP5 unit design: prototype costs, creation, naming, upgrades and the
 * prototype bookkeeping of the original game. Names and signatures follow
 * Thinker Mod src/veh.h, src/build.h and the function-pointer typedefs of
 * src/engine.h at dabe62f2d1abba03de40f3dd3048494843f6a9af (MIT; see
 * engine/THINKER-LICENSE). Routines that only exist in terranx.exe
 * (propose_proto, name_proto, retire_proto, prune_protos, scan_prototypes,
 * upgrade_prototypes, upgrade_any_prototypes, consider_designs,
 * design_new_veh) are written fresh; see ENGINE-WP5.md. Include after engine.h.
 */
#pragma once

namespace alpha {
namespace engine {

/* ---- Thinker veh.cpp ---- */
int __cdecl transport_val(VehChassis chassis_id, VehAblFlag abls, VehReactor reactor_id);
int __cdecl prototype_factor(int unit_id);
int __cdecl mod_proto_cost(VehChassis chassis_id, VehWeapon weapon_id,
    VehArmor armor_id, VehAblFlag ability, VehReactor reactor_id);
int __cdecl mod_base_cost(int unit_id);
int __cdecl mod_veh_cost(int unit_id, int base_id, int32_t* has_proto_cost);
int __cdecl mod_upgrade_cost(int faction_id, int new_unit_id, int old_unit_id);
void __cdecl part_upgrade(int faction_id, int new_unit_id, int old_unit_id);
void __cdecl full_upgrade(int faction_id, int new_unit_id, int old_unit_id);
int __cdecl create_proto(int faction_id, VehChassis chs, VehWeapon wpn, VehArmor arm,
    VehAblFlag abls, VehReactor rec, VehPlan ai_plan);
int __cdecl mod_propose_proto(int faction_id, VehChassis chs, VehWeapon wpn, VehArmor arm,
    VehAblFlag abls, VehReactor rec, VehPlan ai_plan, char* name);
void __cdecl mod_make_proto(int unit_id, VehChassis chassis_id,
    VehWeapon weapon_id, VehArmor armor_id, VehAblFlag abls, VehReactor reactor_id);
int __cdecl mod_name_proto(char* name, int unit_id, int faction_id,
    VehChassis chs, VehWeapon wpn, VehArmor arm, VehAblFlag abls, VehReactor rec);
VehArmor best_armor(int faction_id, int max_cost);
VehWeapon best_weapon(int faction_id);
VehReactor best_reactor(int faction_id);
int proto_offense(int unit_id);
int proto_defense(int unit_id);

/* ---- Thinker build.cpp (prototype choice helpers used by find_proto) ---- */
bool unit_is_better(int unit_id1, int unit_id2);
int unit_score(BASE* const base, int unit_id, int psi_score, int psi_atk, int psi_def, int flags);

/* ---- terranx.exe, written fresh ---- */
int __cdecl proto_cost(int chassis_id, int weapon_id, int armor_id, int abilities, int reactor_id);
/* Mark a prototype obsolete for the faction; an own prototype without units
 * or queue entries is deleted (its slot becomes free). */
void __cdecl retire_proto(int unit_id, int faction_id);
/* Retire the faction's prototypes that unit_id makes obsolete (same triad and
 * role, unit_is_better). Humans only with "auto-prune obsolete units" unless
 * flag is nonzero. */
void __cdecl prune_protos(int faction_id, int unit_id, int flag);
/* Offer the upgrade of every older prototype that unit_id improves on. */
void __cdecl upgrade_prototypes(int faction_id, int unit_id);
/* upgrade_prototypes for each own prototype flagged UNIT_UNK_100. */
void __cdecl upgrade_any_prototypes(int faction_id);
/* Propose the faction's standard designs for its current components. */
void __cdecl consider_designs(int faction_id);
/* Open the design workshop on unit_id (negative faction: automatic proposal
 * shown to the player). Raises EV_MESSAGE "design_new_veh". */
void __cdecl design_new_veh(int faction_id, int unit_id);
/* Diplomacy: list faction_id_2's prototypes that faction_id could build with
 * its own techs but has no equal design for. The list is read with
 * scanned_prototypes(). */
void __cdecl scan_prototypes(int faction_id, int faction_id_2);
int scanned_prototypes(int* unit_ids, int max_ids);
/* Free prototype slots of a faction (0..64). */
int free_proto_slots(int faction_id);
/* "A-D-M" statistics text (exe say_stats_2), appended to buf. */
void __cdecl say_stats_2(char* buf, int unit_id);

} // namespace engine
} // namespace alpha
