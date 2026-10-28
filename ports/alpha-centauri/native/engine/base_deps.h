/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP2: helpers the base code calls that Thinker Mod defines outside base.cpp
 * (game.cpp, faction.cpp, probe.cpp, plan.cpp, path.cpp, veh.cpp at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af, MIT; see engine/THINKER-LICENSE)
 * and that no other package declares in engine_wp.h. engine/base_deps.cpp
 * defines them as weak symbols, ported verbatim where the dependencies exist,
 * so whichever package owns the Thinker file later provides the strong
 * definition without a link conflict. Signatures are Thinker's.
 * Include after engine.h.
 */
#pragma once

namespace alpha {
namespace engine {

/* game.cpp */
void __cdecl bitmask(uint32_t input, uint32_t* offset, uint32_t* mask);
bool un_charter();
bool global_trade_pact();
bool victory_done();
bool full_game_turn();
bool voice_of_planet();
int __cdecl generators(int faction_id, int* pop_size_req);

/* faction.cpp */
bool has_wmode(int faction_id, VehWeaponMode mode);
int __cdecl steal_tech(int faction_id, int faction_id_tgt, int is_steal);
int __cdecl spying(int faction_id);
int __cdecl council_votes(int faction_id);
int __cdecl great_beelzebub(int faction_id, int is_aggressive);
int __cdecl great_satan(int faction_id, int is_aggressive);
int __cdecl aah_ooga(int faction_id, int pact_faction_id);
int __cdecl climactic_battle();

/* terranx.exe diplomacy dialog state (WP8). Weak, zero until WP8 defines them. */
extern int* DiploFearScale;
extern int* DiploCanSurrender;

/* probe.cpp */
int __cdecl steal_energy(int base_id);

/* plan.cpp / path.cpp / build.cpp (AI support) */
int satellite_count(int faction_id, int item_id);
int satellite_goal(int faction_id, int item_id);
int defender_count(int x, int y, int veh_skip_id);
int garrison_count(int x, int y);
int consider_staple(int base_id);
/* Thinker mod_base_hurry ranks a project with facility_score(item,
 * governor_priorities(base)) from plan.cpp (WP6). Until WP6 provides it this
 * weak hook returns 0 (only reached with conf.base_hurry >= 2). */
int wp2_project_hurry_score(int base_id, FacilityId item_id);

/* veh.cpp */
int __cdecl prototype_factor(int unit_id);
int __cdecl mod_proto_cost(VehChassis chassis_id, VehWeapon weapon_id,
    VehArmor armor_id, VehAblFlag ability, VehReactor reactor_id);
int __cdecl mod_base_cost(int unit_id);
int __cdecl mod_veh_cost(int unit_id, int base_id, int32_t* has_proto_cost);

/* terranx.exe routines owned by other packages (weak placeholders raise
 * EV_NOT_IMPLEMENTED). */
void __cdecl prune_protos(int faction_id, int unit_id, int flag);
void __cdecl upgrade_prototypes(int faction_id, int unit_id);
void __cdecl mod_world_volcano(int x, int y, int skip_label);
void __cdecl world_climate();

/* basewin.cpp (WP2 base action, engine/base_actions.cpp) */
void __cdecl action_staple(int base_id);

/* Window/sound/movie calls of the original base code. They do not draw;
 * each raises EV_MESSAGE with a "ui:" tag so the frontend can react
 * (base_ui.cpp). */
void wp2_ui(const char* what, int a = 0, int b = 0, const char* text = nullptr);

} // namespace engine
} // namespace alpha
