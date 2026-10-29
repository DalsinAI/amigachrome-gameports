/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP4 combat, part 1 (strengths, morale, best defender). Ported from Thinker Mod src/veh_combat.cpp at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * Changes against Thinker are marked "Fix:" (behaviour) or "Engine:" (no
 * executable, no windows: popups and drawing go to the event sink and the
 * battle observer). See ENGINE-WP4.md.
 */
#include "combat.h"

namespace alpha {
namespace engine {

/* ---- Engine: battle runtime globals (original addresses in comments) ---- */
static int veh_draw_ids[2] = {-1, -1};          // 0x689F28, 0x689F2C
static int veh_basic_battle_morale[2];          // 0x912420
static int veh_battle_mod_count[2];             // 0x915614
static int veh_battle_modifier[2][4];           // 0x9155F0
static int veh_battle_label[2][4];              // engine: label ids of VehBattleDisplay
static char veh_battle_display[2][4][80];       // 0x90F554
static const char* veh_battle_display_terrain;  // 0x90F550
static int faction_combat_win[MaxPlayerNum];    // 0x93A96C
static int faction_combat_loss[MaxPlayerNum];   // 0x93A98C

int* const VehDrawAttackID = &veh_draw_ids[0];
int* const VehDrawDefendID = &veh_draw_ids[1];
int* const VehBasicBattleMorale = veh_basic_battle_morale;
int* const VehBattleModCount = veh_battle_mod_count;
int (*const VehBattleModifier)[4] = veh_battle_modifier;
char (*const VehBattleDisplay)[4][80] = veh_battle_display;
const char** const VehBattleDisplayTerrain = &veh_battle_display_terrain;
int* const FactionCombatWin = faction_combat_win;
int* const FactionCombatLoss = faction_combat_loss;

int battle_label_id(int type, int offset) {
    if (type < 0 || type > 1 || offset < 0 || offset > 3) {
        return -1;
    }
    return veh_battle_label[type][offset];
}

/* ---- Engine: label ids of the battle display ----
 * label_get() and Difficulty come from the rules loader (rules.h). */
int label_id_of(const char* text) {
    if (!text || !*text || !TextLabels || !TextLabels->labels) {
        return -1;
    }
    for (int i = 0; i < TextLabels->label_count; i++) {
        if (TextLabels->labels[i] == text) {
            return i;
        }
    }
    return -1;
}

const char* difficulty_name(int diff_level) {
    if (Difficulty && diff_level >= 0 && diff_level < MaxDiffNum && Difficulty[diff_level]) {
        return Difficulty[diff_level];
    }
    return "";
}

/* ---- Engine: battle observer ---- */
BattleObserver::~BattleObserver() {}
void BattleObserver::battle_event(const BattleEvent& UNUSED(event)) {}

int combat_rand(int value) {
    return (value > 1 ? game_rand() % value : 0);
}

int stack_kill_check(int veh_id) {
    return stack_check(veh_id, 1, -1, -1, -1)
        - stack_check(veh_id, 2, PLAN_COLONY, -1, -1)
        - stack_check(veh_id, 2, PLAN_SUPPLY, -1, -1)
        - stack_check(veh_id, 2, PLAN_PROBE, -1, -1)
        - stack_check(veh_id, 2, PLAN_ARTIFACT, -1, -1);
}

void combat_odds_fix(VEH* veh_atk, VEH* veh_def, int* attack_odds, int* defend_odds) {
    if (conf.ignore_reactor_power // psi combat odds are already correct
    && veh_atk->offense_value() >= 0 && veh_def->defense_value() >= 0) {
        // calculate attacker and defender power
        int attack_power = veh_atk->cur_hitpoints();
        int defend_power = veh_def->cur_hitpoints();
        // calculate firepower
        int attack_fp = veh_def->reactor_type();
        int defend_fp = veh_atk->reactor_type();
        // calculate hitpoints
        int attack_hp = (attack_power + (defend_fp - 1)) / defend_fp;
        int defend_hp = (defend_power + (attack_fp - 1)) / attack_fp;
        *attack_odds = *attack_odds * attack_hp * defend_power;
        *defend_odds = *defend_odds * defend_hp * attack_power;
        int divisor = std::__gcd(*attack_odds, *defend_odds);
        *attack_odds /= divisor;
        *defend_odds /= divisor;
    }
}

/*
Calculate the psi combat factor for an attacking or defending unit.
*/
int __cdecl psi_factor(int value, int faction_id, int is_attack, int is_fungal_twr) {
    int rule_psi = MFactions[faction_id].rule_psi;
    if (rule_psi) {
        value = ((rule_psi + 100) * value) / 100;
    }
    if (is_attack) {
        if (has_project(FAC_DREAM_TWISTER, faction_id)) {
            value += value * conf.dream_twister_bonus / 100;
        }
    } else {
        if (has_project(FAC_NEURAL_AMPLIFIER, faction_id)) {
            value += value * conf.neural_amplifier_bonus / 100;
        }
        if (is_fungal_twr) {
            value += value * conf.fungal_tower_bonus / 100;
        }
    }
    return value;
}

/*
Generate an output string for the specified unit's morale.
Contains rewrites to update Children's Creche, Brood Pit and Headquarters effects.
*/
void __cdecl mod_say_morale2(char* output, int veh_id, int faction_id_vs_native) {
    VEH* veh = &Vehs[veh_id];
    // Drone riot effect is displayed separately by ignoring it in morale_veh
    int morale = mod_morale_veh(veh_id, false, faction_id_vs_native);
    int faction_id = Vehs[veh_id].faction_id;
    bool native_unit = veh->is_native_unit();
    output[0] = '\0';
    if (native_unit) {
        strncat(output, Morale[morale].name_lifecycle, StrBufLen);
    } else {
        strncat(output, Morale[morale].name, StrBufLen);
    }
    if (veh->plan() < PLAN_COLONY) {
        int morale_modifier = 0;
        int morale_penalty = !native_unit && morale > 0 && veh->home_base_id >= 0
            && Bases[veh->home_base_id].state_flags & BSTATE_DRONE_RIOTS_ACTIVE
            && !(MFactions[faction_id].rule_flags & RFLAG_MORALE);
        int base_id = base_at(Vehs[veh_id].x, Vehs[veh_id].y);
        if (base_id >= 0 && morale < MORALE_ELITE) {
            bool has_creche = has_fac_built(FAC_CHILDREN_CRECHE, base_id);
            bool has_brood_pit = native_unit && has_fac_built(FAC_BROOD_PIT, base_id);
            if (has_fac_built(FAC_HEADQUARTERS, base_id)) {
                morale_modifier++;
            }
            if (has_creche || has_brood_pit) {
                morale_modifier++;
                int morale_active = clamp(Factions[faction_id].SE_morale, -4, 4);
                if (morale_active <= -2) {
                    morale_active++;
                }
                int morale_cap = morale + morale_modifier;
                while (morale_active < 0 && morale_cap < MORALE_ELITE) {
                    morale_cap++;
                    morale_active++;
                    morale_modifier++;
                }
            }
        }
        int morale_pending = Factions[faction_id].SE_morale_pending;
        if (!native_unit && (morale_pending == 2 || morale_pending == 3)) {
            morale_modifier++;
        }
        if (!morale && !morale_modifier) {
            morale_modifier = 1;
        }
        bool flag = false;
        if (morale_penalty) {
            strncat(output, " (-)", StrBufLen);
            flag = true;
        }
        if (morale_modifier) {
            strncat(output, flag ? "(" : " (", StrBufLen);
            for (int i = 0; i < morale_modifier; i++) {
                strncat(output, "+", StrBufLen);
            }
            strncat(output, ")", StrBufLen);
            flag = true;
        }
        if (Vehs[veh_id].state & VSTATE_DESIGNATE_DEFENDER) {
            strncat(output, flag ? "(d)" : " (d)", StrBufLen);
        }
    }
}

/*
Calculate the lifecycle (morale) of the specified planet-owned native unit.
*/
int __cdecl mod_morale_alien(int veh_id, int faction_id_vs_native) {
    int morale;
    // Fungal Tower specific code, shifted to start and added veh_id check to prevent crash
    if (veh_id >= 0 && Vehs[veh_id].unit_id == BSC_FUNGAL_TOWER) {
        VEH* veh = &Vehs[veh_id];
        morale = 0;
        // similar to is_coast() > except with fungus check + Ocean Shelf included
        for (int i = 1; i < 9; i++) {
            int x2 = wrap(veh->x + TableOffsetX[i]);
            int y2 = veh->y + TableOffsetY[i];
            MAP* sq = mapsq(x2, y2);
            if (sq && sq->is_fungus()) {
                morale++;
            }
        }
        morale -= 2;
    } else { // everything else
        if (*CurrentTurn < conf.native_lifecycle_levels[0]) {
            morale = 0;
        } else if (*CurrentTurn < conf.native_lifecycle_levels[1]) {
            morale = 1;
        } else if (*CurrentTurn < conf.native_lifecycle_levels[2]) {
            morale = 2;
        } else if (*CurrentTurn < conf.native_lifecycle_levels[3]) {
            morale = 3;
        } else if (*CurrentTurn < conf.native_lifecycle_levels[4]) {
            morale = 4;
        } else if (*CurrentTurn < conf.native_lifecycle_levels[5]) {
            morale = 5;
        } else {
            morale = 6;
        }
        if (faction_id_vs_native > 0) {
            morale += (Factions[faction_id_vs_native].major_atrocities != 0)
                + (TectonicDetonationCount[faction_id_vs_native] != 0);
        }
        if (veh_id >= 0) {
            if (Vehs[veh_id].state & VSTATE_MONOLITH_UPGRADED) {
                morale++;
            }
            if (Vehs[veh_id].unit_id == BSC_LOCUSTS_OF_CHIRON) {
                morale++;
            }
            morale += Vehs[veh_id].lifecycle_value();
        }
    }
    morale = clamp(morale, 0, 6);
    return morale;
}

/*
Calculate unit morale. TODO: Determine if 2nd param is a toggle for display vs actual morale.
*/
int __cdecl morale_veh(int veh_id, int check_drone_riot, int faction_id_vs_native) {
    VEH* veh = &Vehs[veh_id];
    int faction_id = veh->faction_id;
    int home_base_id = veh->home_base_id;
    int value;
    if (!faction_id) {
        return mod_morale_alien(veh_id, faction_id_vs_native);
    }
    if (veh->plan() == PLAN_PROBE) {
        int probe_morale = clamp(Factions[faction_id].SE_probe, 0, 3);
        probe_morale += has_project(FAC_TELEPATHIC_MATRIX, faction_id) ? 2 : 0;
        for (int i = 0; i < MaxTechnologyNum; i++) {
            if (Tech[i].flags & TFLAG_IMPROVE_PROBE && has_tech(i, faction_id)) {
                probe_morale++;
            }
        }
        probe_morale += veh->morale;
        value = clamp(probe_morale, 2, 6);
        return value;
    }
    if (veh->unit_id < MaxProtoFactionNum && veh->offense_value() < 0) {
        value = clamp((int)veh->morale, 0, 6); // Basic Psi Veh
        return value;
    }
    // everything else
    int morale_modifier = clamp(Factions[faction_id].SE_morale, -4, 4);
    if (morale_modifier <= -2) {
        morale_modifier++;
    } else if (morale_modifier >= 2) {
        morale_modifier--;
    }
    int rule_morale = MFactions[faction_id].rule_morale; // different from 'SOCIAL, MORALE'
    if (rule_morale < 0) { // negative effects 1st
        morale_modifier += rule_morale;
    }
    /*
    Fix: Removed checks for Children's Creche and Brood Pit in home_base_id.
    CC is not supposed to affect units outside base tile according to the manual.
    Brood Pit implementation is bugged in the game engine and the original check
    for BP in this location is never reached due to above 'Basic Psi Veh' checks.
    */
    if (rule_morale > 0) {
        morale_modifier += rule_morale;
    }
    bool morale_flag = MFactions[faction_id].rule_flags & RFLAG_MORALE;
    if (morale_flag && morale_modifier < 0) {
        morale_modifier = 0;
    }
    if (check_drone_riot && home_base_id >= 0
    && Bases[home_base_id].state_flags & BSTATE_DRONE_RIOTS_ACTIVE && !morale_flag) {
        // Fix: removed premature range bounding negating negative morale effects
        morale_modifier--;
    }
    value = clamp(veh->morale + morale_modifier, 0, 6);
    return value;
}

/*
Calculate the offense of the specified prototype. Optional param of the unit defending
against (-1 to ignore) as well as whether artillery or missile combat is being utilized.
*/
int __cdecl mod_offense_proto(int unit_id, int veh_id_def, int is_bombard) {
    UNIT* unit = &Units[unit_id];
    int value;
    if (unit->weapon_mode() == WMODE_PROBE
    && veh_id_def >= 0 && Vehs[veh_id_def].plan() == PLAN_PROBE) {
        value = 16; // probe attacking another probe
    // Fix: veh_id_def -1 could cause out of bounds memory read on reactor struct
    // due to lack of bounds checking when comparing veh_id_def unit_id to Spore Launcher
    } else if ((is_bombard || (unit->offense_value() >= 0
    && (veh_id_def < 0 || Vehs[veh_id_def].defense_value() >= 0)))
    && (veh_id_def < 0 || Vehs[veh_id_def].unit_id != BSC_SPORE_LAUNCHER)
    && unit_id != BSC_SPORE_LAUNCHER) {
        int off_rating = abs(unit->offense_value());
        if (unit->is_missile() && off_rating < 99) {
            off_rating = (off_rating * 3) / 2;
        }
        value = (veh_id_def < 0) ? off_rating : off_rating * 8; // conventional
    } else {
        value = (veh_id_def < 0) ? Rules->psi_combat_ratio_atk[TRIAD_LAND] : // PSI
           Rules->psi_combat_ratio_atk[Vehs[veh_id_def].triad()] * 8;
    }
    // Engine: removed assert against the executable's offense_proto.
    return value;
}

/*
Calculate the defense of the specified prototype. Optional param if unit is being attacked
(-1 to ignore) as well as whether artillery or missile combat is being utilized.
*/
int __cdecl mod_armor_proto(int unit_id, int veh_id_atk, int is_bombard) {
    UNIT* unit = &Units[unit_id];
    int value;
    if (unit->weapon_mode() == WMODE_PROBE
    && veh_id_atk >= 0 && Vehs[veh_id_atk].plan() == PLAN_PROBE) {
        value = 16; // probe defending against another probe
    // Fix: veh_id_def -1 could cause out of bounds memory read on reactor struct
    // due to lack of bounds checking when comparing veh_id_def unit_id to Spore Launcher
    } else if ((is_bombard && unit_id != BSC_SPORE_LAUNCHER
    && (veh_id_atk < 0 || Vehs[veh_id_atk].unit_id != BSC_SPORE_LAUNCHER))
    || (unit->defense_value() >= 0 && (veh_id_atk < 0 || Vehs[veh_id_atk].offense_value() >= 0))) {
        int def_rating = clamp(unit->defense_value(), 1, 9999);
        value = (veh_id_atk < 0) ? def_rating : def_rating * 8; // conventional
    } else {
        value = (veh_id_atk < 0) ? Rules->psi_combat_ratio_def[TRIAD_LAND] : // PSI
            Rules->psi_combat_ratio_def[unit->triad()] * 8;
    }
    // Engine: removed assert against the executable's armor_proto.
    return value;
}

/*
Get the basic offense value for an attacking unit with an optional defender unit parameter.
*/
int __cdecl get_basic_offense(int veh_id_atk, int veh_id_def, int psi_combat_type, int is_bombard, int unk_tgl) {
    // unk_tgl is only set to true in battle_compute when veh_id_atk and veh_id_def are switched places.
    int morale;
    int faction_id_atk = Vehs[veh_id_atk].faction_id;
    int unit_id_atk = Vehs[veh_id_atk].unit_id;
    bool native_unit = Vehs[veh_id_atk].is_native_unit();
    if (faction_id_atk) {
        morale = mod_morale_veh(veh_id_atk, true, 0);
    } else {
        morale = mod_morale_alien(veh_id_atk, veh_id_def >= 0 ? Vehs[veh_id_def].faction_id : -1);
    }
    int base_id_atk = base_at(Vehs[veh_id_atk].x, Vehs[veh_id_atk].y);
    if (base_id_atk >= 0) {
        // Morale effects with CC/BP are modified. SE Morale has no longer effect on native units.
        if (has_fac_built(FAC_CHILDREN_CRECHE, base_id_atk)) {
            morale++;
            int morale_active = clamp(Factions[faction_id_atk].SE_morale, -4, 0);
            if (morale_active <= -2) {
                morale_active++;
            }
            if (!native_unit) {
                morale -= morale_active;
            }
        } else if (native_unit && has_fac_built(FAC_BROOD_PIT, base_id_atk)) {
            morale++;
        }
        if (unk_tgl) {
            int morale_pending = Factions[faction_id_atk].SE_morale_pending;
            if (!native_unit && morale_pending >= 2 && morale_pending <= 3) {
                morale++;
            }
            if (veh_id_def >= 0) {
                if (Vehs[veh_id_def].faction_id) {
                    if (!native_unit && !has_abil(unit_id_atk, ABL_DISSOCIATIVE_WAVE)
                    && has_abil(Vehs[veh_id_def].unit_id, ABL_SOPORIFIC_GAS)) {
                        morale -= 2;
                    }
                } else {
                    morale++;
                }
            }
        }
    }
    if (unk_tgl) {
        morale = clamp(morale, 1, 6);
    }
    VehBasicBattleMorale[unk_tgl != 0] = morale; // shifted up from original
    morale += 6;
    int offense = mod_offense_proto(unit_id_atk, veh_id_def, is_bombard);
    if (psi_combat_type) {
        offense = psi_factor(offense, faction_id_atk, true, false);
    }
    offense = offense * morale * 4;
    return offense;
}

/*
Get the basic defense value for a defending unit with an optional attacker unit parameter.
*/
int __cdecl get_basic_defense(int veh_id_def, int veh_id_atk, int psi_combat_type, int is_bombard) {
    int morale;
    int faction_id_def = Vehs[veh_id_def].faction_id;
    int unit_id_def = Vehs[veh_id_def].unit_id;
    int base_id_def = base_at(Vehs[veh_id_def].x, Vehs[veh_id_def].y);
    bool native_unit = Vehs[veh_id_def].is_native_unit();
    if (faction_id_def) {
        morale = mod_morale_veh(veh_id_def, true, 0);
    } else {
        morale = mod_morale_alien(veh_id_def, veh_id_atk >= 0 ? Vehs[veh_id_atk].faction_id : -1);
    }
    if (base_id_def >= 0) {
        // Morale effects with CC/BP are modified. SE Morale has no longer effect on native units.
        if (has_fac_built(FAC_CHILDREN_CRECHE, base_id_def)) {
            morale++;
            int morale_active = clamp(Factions[faction_id_def].SE_morale, -4, 0);
            if (morale_active <= -2) {
                morale_active++;
            }
            if (!native_unit) {
                morale -= morale_active;
            }
        } else if (native_unit && has_fac_built(FAC_BROOD_PIT, base_id_def)) {
            morale++;
        }
        // Fix: manual has "Units in a headquarters base automatically gain +1 Morale when defending."
        if (has_fac_built(FAC_HEADQUARTERS, base_id_def)) {
            morale++;
        }
        int morale_pending = Factions[faction_id_def].SE_morale_pending;
        if (!native_unit && morale_pending >= 2 && morale_pending <= 3) {
            morale++;
        }
        if (veh_id_atk >= 0 && !Vehs[veh_id_atk].faction_id) {
            morale++;
        }
    }
    if (veh_id_atk >= 0 && Vehs[veh_id_atk].faction_id
    && has_abil(Vehs[veh_id_atk].unit_id, ABL_SOPORIFIC_GAS)
    && !native_unit && !has_abil(unit_id_def, ABL_DISSOCIATIVE_WAVE)) {
        morale -= 2;
    }
    morale = clamp(morale, 1, 6);
    VehBasicBattleMorale[1] = morale;
    morale += 6;
    int plan_def = Units[unit_id_def].plan;
    if (plan_def == PLAN_ARTIFACT) {
        return 1;
    }
    // Fix: added veh_id_atk bounds check to prevent potential read outside array
    if (plan_def == PLAN_PROBE && Units[unit_id_def].defense_value() == 1
    && (veh_id_atk < 0 || Units[Vehs[veh_id_atk].unit_id].plan != PLAN_PROBE)) {
        return 1;
    }
    int defense = mod_armor_proto(unit_id_def, veh_id_atk, is_bombard);
    if (psi_combat_type) {
        defense = psi_factor(defense, faction_id_def, false, unit_id_def == BSC_FUNGAL_TOWER);
    }
    defense *= morale;
    return defense;
}

/*
Determine the best defender in a stack for a given attacker.
Value comparisons start with INT_MIN to enable more generic code.
*/
int __cdecl find_defender(int veh_id_def, int veh_id_atk, int combat_type) {
    assert(veh_id_atk >= 0 && veh_id_atk < *VehCount);
    assert(veh_id_def >= 0 && veh_id_def < *VehCount);
    VEH* veh_def = &Vehs[veh_id_def];
    VEH* veh_atk = &Vehs[veh_id_atk];
    int offense_val;
    int best_veh_id = veh_id_def;
    int best_score = INT_MIN;

    if (veh_id_def < 0) {
        return veh_id_def;
    }
    if (veh_id_atk < 0) {
        offense_val = 8;
    } else {
        offense_val = mod_get_basic_offense(veh_id_atk, veh_id_def, 0, 0, 0);
    }
    MAP* sq = mapsq(veh_def->x, veh_def->y);
    int base_id = base_at(veh_def->x, veh_def->y);
    bool at_ocean = base_id < 0 && is_ocean(sq);
    bool is_airbase = sq && sq->items & BIT_AIRBASE;

    for (int veh_id = veh_top(veh_id_def); veh_id >= 0; veh_id = Vehs[veh_id].next_veh_id_stack) {
        VEH* veh = &Vehs[veh_id];
        if (veh->triad() == TRIAD_LAND && at_ocean) {
            continue;
        }
        // Added veh_id_atk bounds checking to avoid issues
        if (veh_id_atk >= 0 && veh_atk->is_probe() && !veh->is_probe()) {
            continue;
        }
        int flags = 0;
        if (veh_id_atk >= 0 && can_arty(veh->unit_id, true) && can_arty(veh_atk->unit_id, true)) {
            flags = CT_WEAPON_ONLY;
        }
        if (veh_id_atk >= 0 && veh_atk->triad() == TRIAD_AIR
        && has_abil(veh->unit_id, ABL_AIR_SUPERIORITY)
        && !veh->is_missile() && !veh_atk->is_missile()
        && veh_atk->offense_value() > 0 && veh_atk->defense_value() > 0
        && veh->offense_value() > 0) {
            flags |= (CT_WEAPON_ONLY | CT_AIR_DEFENSE);
            if (veh->triad() == TRIAD_AIR) {
                flags |= CT_INTERCEPT;
            }
        }
        int offense_out = 0;
        int defense_out = 0;
        mod_battle_compute(veh_id_atk, veh_id, &offense_out, &defense_out, flags);
        if (!offense_out) {
            break;
        }
        int value = defense_out * veh->cur_hitpoints() / veh->max_hitpoints();
        int score = offense_val * value / offense_out / 8 - veh->offense_value();
        if (veh->plan() <= PLAN_NAVAL_TRANSPORT || veh->plan() == PLAN_TERRAFORM) {
            score *= 16;
        }
        if (veh_id_atk >= 0
        && veh->triad() == TRIAD_AIR && veh_atk->triad() == TRIAD_AIR
        && !veh->is_missile() && !veh_atk->is_missile()
        && (has_abil(veh->unit_id, ABL_AIR_SUPERIORITY)
        || (has_abil(veh_atk->unit_id, ABL_AIR_SUPERIORITY)
        && base_id < 0 && !is_airbase && !stack_check(veh_id, 6, ABL_CARRIER, -1, -1)))) {
            score += 0x80000;
        } else if (combat_type) {
            if (can_arty(veh->unit_id, true)) {
                score += 0x80000;
            }
        } else if (veh->state & VSTATE_DESIGNATE_DEFENDER
        && (base_id >= 0 || is_airbase
        || stack_check(veh_id, 6, ABL_CARRIER, -1, -1)
        || !stack_check(veh_id, 3, TRIAD_AIR, -1, -1))) {
            score += 0x80000;
        }
        // Replace old score method to avoid overflow issues with larger veh_ids
        if (score > best_score || (score == best_score && veh_id > best_veh_id)) {
            best_score = score;
            best_veh_id = veh_id;
        }
    }
    return best_veh_id;
}

static int current_atk_veh_id = -1;
static int current_def_veh_id = -1;

int __cdecl mod_best_defender(int veh_id_def, int veh_id_atk, int combat_type) {
    int veh_id = find_defender(veh_id_def, veh_id_atk, combat_type);
    if (combat_type) {
        current_atk_veh_id = -1;
        current_def_veh_id = -1;
    } else {
        current_atk_veh_id = veh_id_atk;
        current_def_veh_id = veh_id;
    }
    return veh_id;
}

int __cdecl battle_fight_parse_num(int index, int value) {
    if (index > 9) {
        return 3;
    }
    ParseNumTable[index] = value;

    if (conf.ignore_reactor_power && index == 1
    && current_atk_veh_id >= 0 && current_def_veh_id >= 0) {
        VEH* veh_atk = &Vehs[current_atk_veh_id];
        VEH* veh_def = &Vehs[current_def_veh_id];
        combat_odds_fix(veh_atk, veh_def, &ParseNumTable[0], &ParseNumTable[1]);
    }
    return 0;
}

/*
This function replaces defense_value and uses less parameters for simplicity.
*/
int terrain_defense(VEH* veh_def, VEH* veh_atk)
{
    MAP* sq;
    *VehBattleDisplayTerrain = NULL;
    if (!veh_def || veh_def->triad() == TRIAD_AIR
    || !(sq = mapsq(veh_def->x, veh_def->y))
    || is_ocean(sq) || sq->base_who() >= 0) {
        return 2;
    }
    bool native = veh_atk && (veh_atk->is_native_unit()
        || has_project(FAC_PHOLUS_MUTAGEN, veh_atk->faction_id));
    if (sq->is_fungus() && veh_atk && (!veh_atk->faction_id || native)) {
        return 2;
    }
    int defense = sq->is_rocky();
    if (defense) {
        *VehBattleDisplayTerrain = label_get(91); // Rocky
    }
    if (!native && !defense && sq->is_fungus() && veh_def->triad() != TRIAD_AIR) {
        *VehBattleDisplayTerrain = label_get(338); // Fungus
        defense = (has_project(FAC_PHOLUS_MUTAGEN, veh_def->faction_id)
            || veh_def->is_native_unit()) ? 2 : 1;
    }
    if (sq->items & BIT_FOREST && !defense && (!veh_atk || veh_atk->triad() == TRIAD_LAND)) {
        *VehBattleDisplayTerrain = label_get(291); // Forest
        defense = 1;
    }
    return defense + 2;
}

/*
Original game decision making for using nerve gas ability.
*/
bool use_nerve_gas(int faction_id_atk, int faction_id_def) {
    Faction* f_atk = &Factions[faction_id_atk];
    Faction* f_def = &Factions[faction_id_def];
    int value = 0;
    if (*GameRules & RULES_INTENSE_RIVALRY && (f_def->ranking > f_atk->ranking
    || f_def->diplo_combat_count[faction_id_atk] > f_atk->diplo_combat_count[faction_id_def])) {
        value = 3 * (f_atk->AI_fight + 1);
    }
    if ((f_atk->diplo_status[faction_id_def] & DIPLO_ATROCITY_VICTIM)
    || (f_atk->player_flags & PFLAG_COMMIT_ATROCITIES_WANTONLY)
    || (is_human(faction_id_def) && !un_charter())
    || (is_human(faction_id_def)
    && f_atk->diplo_status[faction_id_def] & DIPLO_WANT_REVENGE
    && value + f_def->integrity_blemishes >= 6)) {
        return true;
    }
    return false;
}

bool use_nerve_gas(int faction_id_atk) {
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (i != faction_id_atk && is_alive(i) && !has_pact(faction_id_atk, i)
        && use_nerve_gas(faction_id_atk, i)) {
            return true;
        }
    }
    return false;
}

void __cdecl add_bat(int type, int modifier, const char* display_str)
{
    int offset = VehBattleModCount[type];
    if (modifier && offset >= 0 && offset < 4 && (type == 0 || type == 1)) {
        strcpy_n(VehBattleDisplay[type][offset], 80, display_str);
        VehBattleModifier[type][offset] = modifier;
        veh_battle_label[type][offset] = label_id_of(display_str); // Engine
        VehBattleModCount[type]++;
    }
}

void __cdecl mod_battle_compute(int veh_id_atk, int veh_id_def, int* offense_out, int* defense_out, int combat_type)
{
    enum DefenseType {
        DF_Enable = 1,
        DF_Flechette = 2,
        DF_Sensor = 4,
        DF_GSP = 8,
    };
    assert(veh_id_atk >= 0 && veh_id_atk < *VehCount);
    assert(veh_id_def >= 0 && veh_id_def < *VehCount);
    VehBattleModCount[0] = 0;
    VehBattleModCount[1] = 0;
    // Engine: VehBattleState is a single word in EngineState; [1] is unused.
    VehBattleState[0] = 0;

    VEH* veh_atk = {};
    VEH* veh_def = {};
    MAP* sq_atk = {};
    MAP* sq_def = {};
    int defense = 8;
    int offense = 8;
    int psi_combat = 0;
    int faction_id_atk = 0;
    int faction_id_def = 0;

    if (veh_id_atk >= 0) {
        veh_atk = &Vehs[veh_id_atk];
        faction_id_atk = veh_atk->faction_id;
        sq_atk = mapsq(veh_atk->x, veh_atk->y);
        assert(veh_atk->chassis_type() >= 0);
        assert(veh_atk->weapon_type() >= 0);
        assert(veh_atk->armor_type() >= 0);
    }
    if (veh_id_def >= 0) {
        veh_def = &Vehs[veh_id_def];
        faction_id_def = veh_def->faction_id;
        sq_def = mapsq(veh_def->x, veh_def->y);
        assert(veh_def->chassis_type() >= 0);
        assert(veh_def->weapon_type() >= 0);
        assert(veh_def->armor_type() >= 0);
    }
    const bool is_arty = combat_type & CT_CAN_ARTY;
    const bool is_bombard = (is_arty || (veh_id_atk >= 0 && veh_atk->is_missile()));
    const bool is_rocky = sq_def && sq_def->is_rocky();
    const int alt_atk = sq_atk ? sq_atk->alt_level() : 0;
    const int alt_def = sq_def ? sq_def->alt_level() : 0;

    if (!is_bombard
    || (veh_id_atk >= 0 && veh_atk->unit_id == BSC_SPORE_LAUNCHER)
    || (veh_id_def >= 0 && veh_def->unit_id == BSC_SPORE_LAUNCHER)) {
        if (veh_id_atk >= 0 && veh_atk->offense_value() < 0) {
            psi_combat = 1;
        }
        if (veh_id_def >= 0 && veh_def->defense_value() < 0) {
            psi_combat = psi_combat | 2;
        }
        if (psi_combat) {
            // Display PSI attack/defend bonuses
            int rule_psi_atk = MFactions[faction_id_atk].rule_psi;
            if (rule_psi_atk) {
                add_bat(0, rule_psi_atk, label_get(342)); // Gaian Psi
            }
            if (has_project(FAC_DREAM_TWISTER, faction_id_atk)) {
                add_bat(0, conf.dream_twister_bonus, label_get(343)); // Dream Twist
            }
            int rule_psi_def = MFactions[faction_id_def].rule_psi;
            if (rule_psi_def) {
                add_bat(1, rule_psi_def, label_get(342)); // Gaian Psi
            }
            if (has_project(FAC_NEURAL_AMPLIFIER, faction_id_def)) {
                add_bat(1, conf.neural_amplifier_bonus, label_get(344)); // Neural Amp
            }
            if (veh_id_def >= 0 && veh_def->unit_id == BSC_FUNGAL_TOWER) {
                add_bat(1, conf.fungal_tower_bonus, Units[BSC_FUNGAL_TOWER].name); // Fungal Tower
            }
        }
    }
    if (veh_id_atk >= 0) {
        offense = mod_get_basic_offense(veh_id_atk, veh_id_def, psi_combat, is_bombard, 0);
        if (!veh_atk->is_probe()) {
            if (veh_id_def >= 0 && !combat_type) { // checking if isn't wpn vs wpn ; air toggle
                if (sq_def && sq_def->is_fungus()
                && (veh_atk->is_native_unit() || has_project(FAC_PHOLUS_MUTAGEN, faction_id_atk))
                && veh_def->offense_value() >= 0) {
                    offense = offense * 150 / 100;
                    add_bat(0, 50, label_get(338)); // Fungus
                }
                if (psi_combat & 2 && veh_atk->is_resonance_weapon()) {
                    offense = offense * (ResonanceWeaponValue + 100) / 100;
                    add_bat(0, ResonanceWeaponValue, label_get(1110)); // Resonance Attack
                }
                if (psi_combat
                && has_abil(veh_atk->unit_id, ABL_EMPATH)
                && !has_abil(veh_def->unit_id, ABL_DISSOCIATIVE_WAVE)
                && Rules->combat_bonus_empath_song_vs_psi) {
                    offense = offense * (Rules->combat_bonus_empath_song_vs_psi + 100) / 100;
                    add_bat(0, Rules->combat_bonus_empath_song_vs_psi, Ability[ABL_ID_EMPATH].name);
                }
            }
            if (Rules->combat_penalty_atk_airdrop
            && veh_atk->state & VSTATE_MADE_AIRDROP
            && has_abil(veh_atk->unit_id, ABL_DROP_POD)) {
                offense = offense * (100 - Rules->combat_penalty_atk_airdrop) / 100;
                add_bat(0, -Rules->combat_penalty_atk_airdrop,
                    (drop_range(faction_id_atk) <= Rules->max_airdrop_rng_wo_orbital_insert
                    ? label_get(437) // Air Drop
                    : label_get(438))); // Orbital Insertion
            }
            if (MFactions[faction_id_atk].rule_flags & RFLAG_FANATIC
            && Rules->combat_bonus_fanatic && !combat_type && !psi_combat) {
                offense = offense * (Rules->combat_bonus_fanatic + 100) / 100;
                add_bat(0, Rules->combat_bonus_fanatic, label_get(528)); // Belief
            }
            int bonus_count = MFactions[faction_id_atk].faction_bonus_count;
            for (int i = 0; i < bonus_count; i++) {
                if (MFactions[faction_id_atk].faction_bonus_id[i] == RULE_OFFENSE) {
                    int rule_off_bonus = MFactions[faction_id_atk].faction_bonus_val1[i];
                    offense = offense * rule_off_bonus / 100;
                    add_bat(0, rule_off_bonus - 100, label_get(1108)); // Alien Offense
                }
            }
            if (psi_combat && faction_id_atk) {
                int planet_atk = Factions[faction_id_atk].SE_planet;
                if (planet_atk && Rules->combat_psi_bonus_per_planet) {
                    int modifier = planet_atk * Rules->combat_psi_bonus_per_planet;
                    offense = offense * (modifier + 100) / 100;
                    add_bat(0, modifier, label_get(625)); // Planet
                }
            }
        }
    }
    if ((combat_type & CT_WEAPON_ONLY) && veh_id_def >= 0) { // Weapon-to-weapon battles
        defense = mod_get_basic_offense(veh_id_def, veh_id_atk, psi_combat, is_bombard, 1);
        if (!(combat_type & (CT_INTERCEPT|CT_AIR_DEFENSE))) { // Artillery duel
            int gun_vs_ship_art = Rules->combat_land_vs_sea_artillery;
            if (veh_atk->triad() == TRIAD_LAND && veh_def->triad() == TRIAD_SEA) {
                if (gun_vs_ship_art) {
                    offense = offense * (gun_vs_ship_art + 100) / 100;
                    add_bat(0, gun_vs_ship_art, label_get(435)); // Land Based Guns
                }
            } else if (veh_atk->triad() == TRIAD_SEA && veh_def->triad() == TRIAD_LAND) {
                if (gun_vs_ship_art) {
                    defense = defense * (gun_vs_ship_art + 100) / 100;
                    add_bat(1, gun_vs_ship_art, label_get(435)); // Land Based Guns
                }
            } else if (Rules->combat_artillery_bonus_altitude && sq_atk && sq_def) {
                if (alt_def > alt_atk) {
                    int modifier = (alt_def - alt_atk) * Rules->combat_artillery_bonus_altitude;
                    defense = defense * (modifier + 100) / 100;
                    add_bat(1, modifier, label_get(576)); // Altitude
                } else if (alt_def < alt_atk) {
                    int modifier = (alt_atk - alt_def) * Rules->combat_artillery_bonus_altitude;
                    offense = offense * (modifier + 100) / 100;
                    add_bat(0, modifier, label_get(576)); // Altitude
                }
            }
        }
    } else if (veh_id_def >= 0) {
        defense = mod_get_basic_defense(veh_id_def, veh_id_atk, psi_combat, is_bombard);
        int bonus_count = MFactions[faction_id_def].faction_bonus_count;
        for (int i = 0; i < bonus_count; i++) {
            if (MFactions[faction_id_def].faction_bonus_id[i] == RULE_DEFENSE) {
                int rule_def_bonus = MFactions[faction_id_def].faction_bonus_val1[i];
                defense = defense * rule_def_bonus / 100;
                add_bat(1, rule_def_bonus - 100, label_get(1109)); // Alien Defense
            }
        }
        if (veh_id_atk >= 0 && veh_atk->is_probe()) {
            defense *= 4;
        } else {
            int base_id_def = base_at(veh_def->x, veh_def->y);
            if (psi_combat) {
                if (base_id_def >= 0 && Rules->combat_bonus_intrinsic_base_def) {
                    add_bat(1, Rules->combat_bonus_intrinsic_base_def, label_get(332)); // Base
                    defense = defense * (Rules->combat_bonus_intrinsic_base_def + 100) / 100;
                }
                if (conf.planet_defense_bonus && faction_id_def) {
                    int planet_def = Factions[faction_id_def].SE_planet;
                    int modifier = planet_def * Rules->combat_psi_bonus_per_planet
                        / (conf.planet_defense_bonus <= 1 ? 2 : 1);
                    if (modifier != 0) {
                        defense = defense * (modifier + 100) / 100;
                        add_bat(1, modifier, label_get(625)); // Planet
                    }
                }
                defense *= 4;
            } else {
                int terrain_def = terrain_defense(veh_def, veh_atk);
                // Engine: removed assert against the executable's defense_value.
                bool plain_terrain = terrain_def <= 2;
                if (veh_id_atk >= 0 && veh_atk->triad() == TRIAD_LAND) {
                    if (Rules->combat_artillery_bonus_altitude
                    && combat_type && veh_def->triad() == TRIAD_LAND && alt_atk > alt_def) {
                        offense = offense * (Rules->combat_artillery_bonus_altitude
                            * (alt_atk - alt_def) + 100) / 100;
                        add_bat(0, Rules->combat_artillery_bonus_altitude, label_get(576)); // Altitude
                    }
                    if (Rules->combat_mobile_open_ground
                    && !combat_type && base_id_def < 0
                    && plain_terrain && !is_rocky) {
                        int speed_atk = proto_speed(veh_atk->unit_id);
                        int speed_def = proto_speed(veh_def->unit_id);
                        if (speed_atk > Rules->move_rate_roads
                        && speed_atk >= speed_def) {
                            offense = offense * (Rules->combat_mobile_open_ground + 100) / 100;
                            add_bat(0, Rules->combat_mobile_open_ground, label_get(611)); // Mobile in open
                        }
                    }
                    if (Rules->combat_mobile_def_in_rough
                    && !combat_type && (!plain_terrain || base_id_def >= 0)
                    && proto_speed(veh_atk->unit_id) > Rules->move_rate_roads) {
                        defense = defense * (Rules->combat_mobile_def_in_rough + 100) / 100;
                        // "Rough vs. Mobile" / "Mobile vs. Base"
                        add_bat(1, Rules->combat_mobile_def_in_rough, base_id_def < 0
                            ? label_get(548) : label_get(612));
                    }
                    // Modify this bonus to apply only when both tiles have roads or magtubes
                    // and the defender does not have a base or bunker
                    if (Rules->combat_bonus_atk_road
                    && !combat_type && !is_ocean(sq_atk) && !is_ocean(sq_def)
                    && !sq_def->is_base_or_bunker()
                    && sq_atk->items & (BIT_BASE_IN_TILE|BIT_ROAD|BIT_MAGTUBE)
                    && sq_def->items & (BIT_ROAD|BIT_MAGTUBE)) {
                        offense = offense * (Rules->combat_bonus_atk_road + 100) / 100;
                        add_bat(0, Rules->combat_bonus_atk_road, label_get(606)); // Road Attack
                    }
                    if (Rules->combat_penalty_atk_lower_elevation
                    && !combat_type && alt_def > alt_atk
                    && !has_abil(veh_atk->unit_id, ABL_ANTIGRAV_STRUTS)) {
                        defense = defense * (Rules->combat_penalty_atk_lower_elevation + 100) / 100;
                        add_bat(1, Rules->combat_penalty_atk_lower_elevation, label_get(441)); // Uphill
                    }
                    if (Rules->combat_bonus_atk_higher_elevation
                    && !combat_type && alt_atk > alt_def
                    && !has_abil(veh_def->unit_id, ABL_ANTIGRAV_STRUTS)) {
                        offense = offense * (Rules->combat_bonus_atk_higher_elevation + 100) / 100;
                        add_bat(0, Rules->combat_bonus_atk_higher_elevation, label_get(330)); // Downhill
                    }
                }
                defense *= terrain_def;
                if (!plain_terrain) {
                    add_bat(1, 50 * terrain_def - 100,
                        (*VehBattleDisplayTerrain ? *VehBattleDisplayTerrain : label_get(331))); // Terrain
                }
                const char* display_def = NULL; // only one is displayed
                int def_multi = 100;
                if (base_id_def >= 0 && veh_id_atk >= 0) {
                    display_def = label_get(332); // Base
                    int fac_modifier = 0;
                    // Fix: original code checks if faction_id_def has Citizens Defense Force
                    switch (veh_atk->triad()) {
                      case TRIAD_LAND:
                        if (has_facility(FAC_PERIMETER_DEFENSE, base_id_def)) {
                            fac_modifier = conf.facility_defense_value[0];
                            display_def = label_get(354); // Perimeter
                        }
                        break;
                      case TRIAD_SEA:
                        if (has_facility(FAC_NAVAL_YARD, base_id_def)) {
                            fac_modifier = conf.facility_defense_value[1];
                            display_def = Facility[FAC_NAVAL_YARD].name;
                        }
                        break;
                      case TRIAD_AIR:
                        if (has_facility(FAC_AEROSPACE_COMPLEX, base_id_def)) {
                            fac_modifier = conf.facility_defense_value[2];
                            display_def = Facility[FAC_AEROSPACE_COMPLEX].name;
                        }
                        break;
                      default:
                        break;
                    }
                    if (has_fac_built(FAC_TACHYON_FIELD, base_id_def)) {
                        fac_modifier += conf.facility_defense_value[3];
                        display_def = label_get(357); // Tachyon
                    }
                    if (Rules->combat_bonus_intrinsic_base_def
                    && !fac_modifier && !has_abil(veh_atk->unit_id, ABL_BLINK_DISPLACER)) {
                        defense = defense * (Rules->combat_bonus_intrinsic_base_def + 100) / 100;
                        add_bat(1, Rules->combat_bonus_intrinsic_base_def, label_get(332)); // Base
                    }
                    if (fac_modifier && has_abil(veh_atk->unit_id, ABL_BLINK_DISPLACER)) {
                        fac_modifier = 0;
                        display_def = label_get(428); // Base vs. Blink
                    }
                    if (Rules->combat_infantry_vs_base
                    && faction_id_atk && !combat_type
                    && !veh_atk->is_native_unit()
                    && veh_atk->triad() == TRIAD_LAND
                    && veh_atk->speed() == 1) {
                        offense = offense * (Rules->combat_infantry_vs_base + 100) / 100;
                        add_bat(0, Rules->combat_infantry_vs_base, label_get(547)); // Infantry vs. Base
                    }
                    def_multi += fac_modifier;
                }
                else if (base_id_def < 0 && sq_def && sq_def->is_bunker()
                && (veh_id_atk < 0 || veh_atk->triad() != TRIAD_AIR)) {
                    def_multi = 150;
                    display_def = label_get(358); // Bunker
                }
                else if (is_arty && base_id_def < 0 && sq_def && !is_rocky
                && !(sq_def->items & BIT_FOREST) && !sq_def->is_fungus()) {
                    def_multi = 150;
                    display_def = label_get(525); // Open Ground
                }
                defense = defense * def_multi / 50;
                if (def_multi > 100) {
                    add_bat(1, def_multi - 100, display_def);
                }
            }
            if (faction_id_def) {
                int def_state = 0;
                int flechette = 100;
                if (veh_id_atk >= 0 && veh_atk->is_missile()) {
                    for (int i = 0, cnt = *BaseCount; i < cnt; ++i) {
                        if (Bases[i].faction_id == faction_id_def
                        && map_range(veh_def->x, veh_def->y, Bases[i].x, Bases[i].y)
                        <= FlechetteDefenseRange
                        && has_fac_built(FAC_FLECHETTE_DEFENSE_SYS, i)) {
                            // Multiple facilities are cumulative with missile defense
                            defense = defense * (FlechetteDefenseValue + 100) / 100;
                            flechette = flechette * (FlechetteDefenseValue + 100) / 100;
                            def_state |= DF_Flechette;
                        }
                    }
                }
                for (auto& m : iterate_tiles(veh_def->x, veh_def->y, 0, 25)) {
                    int value = is_sensor(m.x, m.y);
                    bool found = false;
                    if (value > 0) {
                        if (!conf.sensor_defense_ocean && is_ocean(m.sq)) {
                            int near_id = base_find(m.x, m.y);
                            if (near_id >= 0 && Bases[near_id].faction_id == faction_id_def) {
                                found = true;
                            }
                        } else {
                            if (m.sq->owner < 0 || m.sq->owner == faction_id_def) {
                                found = true;
                            }
                        }
                        if (found) {
                            def_state |= (DF_Enable|DF_Sensor);
                            if (value > 1) {
                                def_state |= DF_GSP;
                                break;
                            }
                        }
                    }
                }
                if (def_state & DF_Flechette) {
                    add_bat(1, flechette - 100, label_get(1113)); // Flechette
                }
                if (def_state & DF_Sensor) {
                    defense = defense * (Rules->combat_defend_sensor + 100) / 100;
                    add_bat(1, Rules->combat_defend_sensor, label_get(613)); // Sensor
                }
                if (def_state & DF_GSP) {
                    defense = defense * (Rules->combat_defend_sensor + 100) / 100;
                    add_bat(1, Rules->combat_defend_sensor, label_get(1123)); // GSP
                }
            }
            if (veh_id_atk >= 0 && psi_combat & 1
            && Rules->combat_bonus_trance_vs_psi
            && has_abil(veh_def->unit_id, ABL_TRANCE)
            && !has_abil(veh_atk->unit_id, ABL_DISSOCIATIVE_WAVE)) {
                defense = defense * (Rules->combat_bonus_trance_vs_psi + 100) / 100;
                add_bat(1, Rules->combat_bonus_trance_vs_psi, label_get(329)); // Trance
            }
            if (veh_id_atk >= 0 && psi_combat & 1 && veh_def->is_resonance_armor()) {
                defense = defense * (ResonanceArmorValue + 100) / 100;
                add_bat(1, ResonanceArmorValue, label_get(1111)); // Resonance Def.
            }
            // added veh_id_atk checks below when needed
            if (veh_id_atk >= 0 && !psi_combat
            && veh_atk->triad() == TRIAD_AIR
            && has_abil(veh_atk->unit_id, ABL_AIR_SUPERIORITY)) {
                if (veh_def->triad() != TRIAD_AIR) {
                    int penalty = Rules->combat_penalty_air_supr_vs_ground;
                    if (penalty) {
                        offense = offense * (100 - penalty) / 100;
                        add_bat(0, -penalty, label_get(448)); // Ground Strike
                    }
                } else if (Rules->combat_bonus_air_supr_vs_air
                && !has_abil(veh_def->unit_id, ABL_AIR_SUPERIORITY)) {
                    offense = offense * (Rules->combat_bonus_air_supr_vs_air + 100) / 100;
                    add_bat(0, Rules->combat_bonus_air_supr_vs_air, label_get(449)); // Air-to-Air
                }
            }
            if (veh_id_atk >= 0 && !psi_combat
            && Rules->combat_bonus_air_supr_vs_air
            && veh_atk->triad() == TRIAD_AIR
            && veh_def->triad() == TRIAD_AIR
            && !has_abil(veh_atk->unit_id, ABL_AIR_SUPERIORITY)
            && has_abil(veh_def->unit_id, ABL_AIR_SUPERIORITY)
            && !veh_atk->is_missile() && !veh_def->is_missile()) {
                defense = defense * (Rules->combat_bonus_air_supr_vs_air + 100) / 100;
                add_bat(1, Rules->combat_bonus_air_supr_vs_air, label_get(449)); // Air-to-Air
            }
            if (Rules->combat_penalty_non_combat_vs_combat
            && !veh_def->offense_value() && veh_def->defense_value() == 1
            && (faction_id_atk || base_id_def < 0)) {
                defense = defense * (100 - Rules->combat_penalty_non_combat_vs_combat) / 100;
                add_bat(1, -Rules->combat_penalty_non_combat_vs_combat, label_get(439)); // Non Combat
            }
            if (veh_id_atk >= 0 && base_id_def >= 0
            && Rules->combat_bonus_vs_ship_port
            && veh_atk->triad() != TRIAD_SEA
            && veh_def->triad() == TRIAD_SEA) {
                offense = offense * (Rules->combat_bonus_vs_ship_port + 100) / 100;
                add_bat(0, Rules->combat_bonus_vs_ship_port, label_get(335)); // In Port
            }
            if (veh_id_atk >= 0
            && veh_def->is_pulse_armor()
            && veh_atk->triad() == TRIAD_LAND
            && veh_atk->speed() > 1) {
                defense = defense * (PulseArmorValue + 100) / 100;
                add_bat(1, PulseArmorValue, label_get(1112)); // Pulse Defense
            }
            if (veh_id_atk >= 0
            && Rules->combat_comm_jammer_vs_mobile
            && has_abil(veh_def->unit_id, ABL_COMM_JAMMER)
            && !has_abil(veh_atk->unit_id, ABL_DISSOCIATIVE_WAVE)
            && veh_atk->triad() == TRIAD_LAND
            && veh_atk->speed() > 1) {
                defense = defense * (Rules->combat_comm_jammer_vs_mobile + 100) / 100;
                add_bat(1, Rules->combat_comm_jammer_vs_mobile, label_get(336)); // Jammer
            }
            if (veh_id_atk >= 0
            && Rules->combat_aaa_bonus_vs_air
            && veh_atk->triad() == TRIAD_AIR
            && has_abil(veh_def->unit_id, ABL_AAA)
            && !has_abil(veh_atk->unit_id, ABL_DISSOCIATIVE_WAVE)) {
                defense = defense * (Rules->combat_aaa_bonus_vs_air + 100) / 100;
                add_bat(1, Rules->combat_aaa_bonus_vs_air, label_get(337)); // Tracking
            }
        }
    }
    if (offense_out) {
        *offense_out = offense;
    }
    if (defense_out) {
        *defense_out = defense;
    }
}


} // namespace engine
} // namespace alpha
