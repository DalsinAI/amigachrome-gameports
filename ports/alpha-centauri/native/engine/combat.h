/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP4 combat: battle strength computation, best defender, morale and psi
 * factors, promotion, interceptors, the full battle (rounds, damage, retreat,
 * marine detachment, collateral damage, casualties), artillery, planet
 * busters and the engine_attack() entry point used by the UI.
 *
 * Ported from Thinker Mod src/veh_combat.cpp and src/veh.cpp at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * Deviations are marked "Fix:" in the sources and listed in ENGINE-WP4.md.
 *
 * Include this header after engine.h (it includes engine.h itself).
 */
#pragma once
#include "engine.h"

namespace alpha {
namespace engine {

/* Thinker veh_combat.h */
enum VehCombatType {
    CT_CAN_ARTY = 0x1,
    CT_WEAPON_ONLY = 0x2,
    CT_AIR_DEFENSE = 0x8,
    CT_INTERCEPT = 0x10,
};

/* VehAttackFlags bits (original 0x93E904): set by the caller of battle_fight.
 * ATTACK_INTERACTIVE asks the attacking human through the event sink
 * (BADIDEA/GOODIDEA odds, breaking treaties, nerve gas); ATTACK_EXECUTE
 * performs the battle. Without ATTACK_EXECUTE only the movement bookkeeping
 * before the battle happens, as in the original. */
enum VehAttackFlag {
    ATTACK_INTERACTIVE = 0x1,
    ATTACK_EXECUTE = 0x2,
};

/* ---- Battle runtime globals of the original executable (not saved) ----
 * They describe the battle in progress for the status window, so they are
 * transient and shared by every EngineState, like in the original.
 * VehAttackFlags itself is a core global (game_state.h). */
extern int* const VehDrawAttackID;          // [1]
extern int* const VehDrawDefendID;           // [1]
extern int* const VehBasicBattleMorale;      // [2] ; [0] offense, [1] defense
extern int* const VehBattleModCount;         // [2] ; [0] offense, [1] defense
extern int (*const VehBattleModifier)[4];    // [2][4]
extern char (*const VehBattleDisplay)[4][80];// [2][4][80]
extern const char** const VehBattleDisplayTerrain;
extern int* const FactionCombatWin;          // [8]
extern int* const FactionCombatLoss;         // [8]

/* ---- Thinker functions without an engine_wp.h entry ---- */
void __cdecl mod_say_morale2(char* output, int veh_id, int faction_id_vs_native);
inline int mod_morale_veh(int veh_id, int check_drone_riot, int faction_id_vs_native) {
    return morale_veh(veh_id, check_drone_riot, faction_id_vs_native);
}
inline int mod_get_basic_offense(int veh_id_atk, int veh_id_def, int psi_combat_type, int is_bombard, int unk_tgl) {
    return get_basic_offense(veh_id_atk, veh_id_def, psi_combat_type, is_bombard, unk_tgl);
}
inline int mod_get_basic_defense(int veh_id_def, int veh_id_atk, int psi_combat_type, int is_bombard) {
    return get_basic_defense(veh_id_def, veh_id_atk, psi_combat_type, is_bombard);
}
int __cdecl mod_offense_proto(int unit_id, int veh_id_def, int is_bombard);
int __cdecl mod_armor_proto(int unit_id, int veh_id_atk, int is_bombard);
int __cdecl battle_fight_parse_num(int index, int value);
int terrain_defense(VEH* veh_def, VEH* veh_atk);
bool use_nerve_gas(int faction_id_atk, int faction_id_def);
bool use_nerve_gas(int faction_id_atk);
void __cdecl add_bat(int type, int modifier, const char* display_str);
int __cdecl mod_battle_fight(int veh_id, int offset, int table_offset, int option, int* def_id);
int __cdecl battle_kill_credits(int veh_id);
/* Thinker veh_combat.cpp file-local helpers, shared by combat.cpp and
 * combat_fight.cpp. */
int combat_rand(int value);
int stack_kill_check(int veh_id);
void combat_odds_fix(VEH* veh_atk, VEH* veh_def, int* attack_odds, int* defend_odds);
int __cdecl find_defender(int veh_id_def, int veh_id_atk, int combat_type);
/* Label id of VehBattleDisplay[type][offset], -1 for a name. */
int battle_label_id(int type, int offset);
/* shoot(): exe-only, new code. Asked before a planet buster hits a faction;
 * returns non-zero when the attack is called off (treaty kept). */
int __cdecl shoot(int faction_id, int faction_id_tgt, int x, int y);

/* Label id (labels.txt line) of a label_get() string, -1 for other text
 * such as unit or facility names. label_get() is the rules loader's. */
int label_id_of(const char* text);
const char* difficulty_name(int diff_level);

/* ---- Battle observation for the UI ----
 * The original drew the battle with boom(), veh_scoot() and StatusWin_draw().
 * The engine records the same moments as BattleEvents instead; a UI can
 * replay them (or install an observer to animate them as they happen). */
enum BattleEventType {
    BE_START = 0,        // a = veh_id_atk, b = veh_id_def, c = offense, d = defense, e = combat_type
    BE_ROUND = 1,        // a = side hit (0 attacker, 1 defender), b = attacker hp, c = defender hp
    BE_ARTILLERY = 2,    // a = veh_id_def, b = damage dealt, c = defender hp after
    BE_RETREAT = 3,      // a = veh_id_def, x/y = destination
    BE_MARINE_DETACH = 4,// a = faction captured from, b = faction capturing
    BE_PROMOTED = 5,     // a = veh_id, b = new morale
    BE_KILLED = 6,       // a = veh_id (id at that moment), b = unit_id, c = faction_id
    BE_RELIC = 7,        // a = veh_id captured artifact, b = new faction
    BE_INTERCEPT = 8,    // a = interceptor veh_id, x/y = target tile
    BE_BOOM = 9,         // original boom(x, y, flags): a = flags
    BE_PLANET_BUSTER = 10,// a = blast radius, x/y = target
    BE_BASE_POP_LOST = 11,// a = base_id, b = new pop_size
    BE_BASE_DESTROYED = 12,// a = old base_id, b = faction_id
    BE_BASE_CAPTURED = 13,// a = base_id, b = old faction, c = new faction
    BE_END = 14,         // a = return value of battle_fight_2
};

struct BattleEvent {
    BattleEventType type;
    int a;
    int b;
    int c;
    int d;
    int e;
    int x;
    int y;
};

class BattleObserver {
    public:
    virtual ~BattleObserver();
    virtual void battle_event(const BattleEvent& event);
};
/* Install an observer (null removes it). Returns the previous one. */
BattleObserver* set_battle_observer(BattleObserver* observer);

const int MaxBattleEvents = 512;

struct BattleModifier {
    int percent;          // VehBattleModifier
    int label_id;         // label id, or -1 when the text is a unit/facility name
    char text[80];        // VehBattleDisplay
};

enum BattleOutcome {
    BATTLE_INVALID = 0,        // bad arguments, nothing changed
    BATTLE_NO_RULES = 1,       // rules tables not loaded (Rules->move_rate_roads == 0)
    BATTLE_NO_TARGET = 2,      // nothing to attack or capture at x, y
    BATTLE_CANCELLED = 3,      // the attacker declined (treaty, odds prompt); moves refunded
    BATTLE_ATTACKER_WON = 4,   // defender destroyed (stack may have more units)
    BATTLE_DEFENDER_WON = 5,   // attacker destroyed
    BATTLE_DEFENDER_RETREATED = 6,
    BATTLE_BOTH_SURVIVED = 7,  // single round weapon duel (land/sea artillery vs ship)
    BATTLE_ARTILLERY = 8,      // bombardment of the whole stack
    BATTLE_MARINE_DETACH = 9,  // defending ship captured
    BATTLE_PLANET_BUSTER = 10, // planet buster detonated
    BATTLE_BASE_CAPTURED = 11, // no defender: the attacker moved in and captured the base
};

const char* battle_outcome_name(int outcome);

/* Result of engine_attack(). Ids are the values after the battle, -1 when the
 * unit no longer exists (vehicle ids shift down when a vehicle is removed). */
struct BattleResult {
    int outcome;
    int return_value;      // mod_battle_fight_2 return value
    int veh_id_atk;        // attacker after the battle, -1 if destroyed
    int veh_id_def;        // chosen defender after the battle, -1 if destroyed
    int unit_id_atk;
    int unit_id_def;
    int faction_id_atk;
    int faction_id_def;
    int x, y;              // attacker origin
    int tx, ty;            // target tile
    int combat_type;       // VehCombatType bits
    int offense;           // battle strengths as shown by the status window
    int defense;
    int morale_atk;        // VehBasicBattleMorale
    int morale_def;
    int hp_atk_before, hp_def_before;
    int hp_atk_after, hp_def_after;
    int rounds;
    int attacker_hits;     // rounds won by the attacker
    int defender_hits;
    int num_killed;        // vehicles removed (battle_kill)
    int num_relics;        // artifacts captured
    int num_damaged;       // collateral damage count
    int credits;           // planetpearls
    bool promoted_atk;
    bool promoted_def;
    int base_id;           // base at the target, -1 if none or destroyed
    bool base_destroyed;
    bool base_captured;
    int retreat_x, retreat_y;
    int mod_count[2];
    BattleModifier mods[2][4];
    const char* terrain_label;
    int event_count;
    bool events_truncated;
    BattleEvent events[MaxBattleEvents];
};

/* Attack the tile (x, y) with veh_id_atk, as a human or computer player would
 * from the map. Adjacent targets use the normal battle; targets further away
 * use artillery (when the unit can bombard and is in range). A tile with an
 * enemy base and no units is captured: the attacker moves in and WP2's
 * mod_capture_base runs. attack_flags is VehAttackFlags (ATTACK_EXECUTE by
 * default; add ATTACK_INTERACTIVE for the odds/treaty prompts of a human).
 * Fills *result (which may be large; allocate it on the heap on Amiga). */
int engine_attack(int veh_id_atk, int x, int y, BattleResult* result,
    int attack_flags = ATTACK_EXECUTE);
/* Odds preview without changing anything (mod_battle_fight_2 option 0):
 * returns offense * 8 / (defense + 1) (4 for artillery), 0 when not possible. */
int engine_attack_odds(int veh_id_atk, int x, int y, int* def_id);

/* ---- Functions of other packages that combat calls ----
 * Thinker signatures. They are not in engine_wp.h; engine/stubs/wp4_deps.cpp
 * holds placeholders until the owning package defines them (see
 * ENGINE-WP4.md). */
int __cdecl break_treaty(int faction_id, int faction_id_tgt, uint32_t status); // WP8
int __cdecl act_of_aggression(int faction_id_atk, int faction_id_def);         // WP8
void __cdecl cause_friction(int faction_id, int faction_id_with, int friction); // WP8
void __cdecl major_atrocity(int faction_id, int faction_id_tgt);              // WP8
bool un_charter();                                                             // WP8
void __cdecl mon_enemy_destroyed(int faction_id, int enemy_id);               // WP2 (monument.cpp)
void __cdecl add_goal(int faction_id, int type, int priority, int x, int y, int base_id); // WP6
void __cdecl world_climate();                                                  // WP7

} // namespace engine
} // namespace alpha
