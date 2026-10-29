/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP6 computer players: research choices.
 * Ported from Thinker Mod src/tech.cpp at dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * Deviations from Thinker are marked "Fix:" (behaviour) or "Original:"
 * (replaced executable or Windows code) and listed in ENGINE-WP6.md.
 */
#include "ai.h"

namespace alpha {
namespace engine {
namespace ai {

/* Original: the assert(value == tech_val(...)) checks against the executable
 * function in mod_tech_val are removed. */
static bool revised_tech_cost() {
    // Not supported during multiplayer
    return conf.revised_tech_cost && !*MultiplayerActive;
}

/*
Determine if preq_tech_id is a prerequisite of parent_tech_id within descending range.
Return Value: Is preq_tech_id prerequisite of parent_tech_id? true/false
*/
int __cdecl tech_is_preq(int preq_tech_id, int parent_tech_id, int range) {
    if (preq_tech_id < 0 || parent_tech_id < 0) {
        return false;
    }
    if (preq_tech_id == parent_tech_id) {
        return true;
    }
    if (range <= 0) {
        return false;
    }
    return tech_is_preq(preq_tech_id, Tech[parent_tech_id].preq_tech1, range - 1)
        || tech_is_preq(preq_tech_id, Tech[parent_tech_id].preq_tech2, range - 1);
}

} // namespace ai

/*
Determine how valuable the specified tech_id is to a faction. This tech_id either corresponds
to a technology (0-88), another faction (89-96) or a prototype (97-608). The 3rd parameter
determines whether a simplistic or extended calculation is required for technology.
Return Value: Value of tech_id to the specified faction
*/
int __cdecl mod_tech_val(int tech_id, int faction_id, int simple_calc) {
    using namespace ai;
    Faction* f = &Factions[faction_id];
    MFaction* m = &MFactions[faction_id];
    if (tech_id == 9999) {
        return 2;
    }
    int value;
    if (tech_id < MaxTechnologyNum) {
        CTech* tech = &Tech[tech_id];
        int enemy_count = 0;
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (i != faction_id && has_treaty(faction_id, i, DIPLO_VENDETTA)) {
                enemy_count++;
            }
        }
        int factor_ai = 1;
        if (!simple_calc) {
            factor_ai = (*GameRules & RULES_BLIND_RESEARCH) ? 4 : 2;
        }
        value = tech->AI_growth * (factor_ai * f->AI_growth + 1)
            + tech->AI_wealth * (factor_ai * f->AI_wealth + 1)
            + tech->AI_tech * (factor_ai * f->AI_tech + 1)
            + tech->AI_power * (factor_ai * f->AI_power + 1);
        int base_count = Factions[faction_id].base_count;
        if ((!tech->AI_power || (!f->AI_power && !enemy_count))
        && (!tech->AI_tech || !f->AI_tech)
        && (!tech->AI_wealth || !f->AI_wealth)
        && (!tech->AI_growth || (!f->AI_growth && base_count >= 4))) {
            value = (value + 1) / 2;
        }
        bool is_player = is_human(faction_id);
        int tech_id_lvl = tech_level(tech_id, 0);
        if (!is_player && !has_tech(tech_id, faction_id) && simple_calc) {
            int owners = __builtin_popcount(TechOwners[tech_id]);
            if (owners > 1) {
                value += 2 - 2 * owners;
            }
            int search_lvl = 1;
            for (int i = 0; i < MaxTechnologyNum; i++) {
                if (has_tech(i, faction_id)) {
                    int current_lvl = tech_level(i, 0);
                    if (search_lvl < current_lvl) {
                        search_lvl = current_lvl;
                    }
                }
            }
            if (tech_id_lvl < search_lvl) {
                value = value * (tech_id_lvl + 1) / (search_lvl + 1);
            }
            if (value < 1) {
                value = 1;
            }
        }
        if (simple_calc) {
            return value;
        }
        if (base_count) {
            for (int region = 1; region < MaxRegionLandNum; region++) {
                if (!bad_reg(region)) {
                    int pwr_base = f->region_total_bases[region] * tech->AI_power;
                    int plan = f->region_base_plan[region];
                    if (plan == PLAN_NAVAL_TRANSPORT && enemy_count && !is_player) {
                        value += (pwr_base / base_count);
                    } else if (plan == PLAN_DEFENSE) {
                        value += (pwr_base * 4) / (base_count * (is_player + 1));
                    } else if (plan == PLAN_OFFENSE) {
                        value += (pwr_base * ((f->best_weapon_value
                            >= f->enemy_best_weapon_value) ? 2 : 4))
                             / (base_count * (is_player + 1));
                    } else {
                        for (int i = 1; i < MaxPlayerNum; i++) {
                            if (i != faction_id && Factions[i].region_total_bases[region]
                            && f->region_total_bases[region]
                            && has_treaty(faction_id, i, DIPLO_COMMLINK)
                            && (!has_treaty(faction_id, i, DIPLO_PACT | DIPLO_TREATY)
                            || has_treaty(faction_id, i, DIPLO_WANT_REVENGE))) {
                                value += (pwr_base / (base_count * (is_player + 1)));
                            }
                        }
                    }
                }
            }
        }
        if (has_tech(tech_id, faction_id)) {
            return value;
        }
        if (climactic_battle()
        && tech_is_preq(tech_id, Facility[FAC_ASCENT_TO_TRANSCENDENCE].preq_tech, 2)) {
            value *= 4;
        }
        if (f->SE_planet_base > 0 && f->AI_growth) {
            if (tech_is_preq(tech_id, TECH_CentMed, 9999)) {
                value *= 3;
            }
            if (tech_is_preq(tech_id, TECH_PlaEcon, 9999)) {
                value *= 2;
            }
            if (tech_is_preq(tech_id, TECH_AlphCen, 3)) {
                value *= 2;
            }
        }
        if (f->SE_probe_base <= 0) {
            if (tech_is_preq(tech_id, Facility[FAC_HUNTER_SEEKER_ALGORITHM].preq_tech, f->AI_tech + 2)) {
                if (!f->AI_power) {
                    value *= 2;
                }
                if (f->AI_tech) {
                    value *= 2;
                }
            }
        }
        if (f->AI_growth && tech_is_preq(tech_id, TECH_DocInit, 2)) {
            value *= 2;
        }
        if ((f->AI_wealth || !*MapCloudCover) && tech_is_preq(tech_id, TECH_EnvEcon, 9999)) {
            value *= 2;
        }
        if (Tech[tech_id].flags & TFLAG_SECRETS && !TechOwners[tech_id]
        && !(*GameRules & RULES_BLIND_RESEARCH)) {
            value *= (f->AI_power + 1) * 2;
        }
        if (m->rule_psi > 0) {
            if (tech_is_preq(tech_id, Facility[FAC_DREAM_TWISTER].preq_tech, 9999)) {
                value *= 2;
            }
        } else {
            int preq_tech_fusion = Reactor[REC_FUSION - 1].preq_tech;
            if (tech_id == preq_tech_fusion) {
                value *= 2;
            }
            if (tech_id == Reactor[REC_QUANTUM - 1].preq_tech) {
                value *= 2;
            }
            if (tech_is_preq(tech_id, preq_tech_fusion, 9999)) {
                value++;
            }
            if (tech_is_preq(tech_id, preq_tech_fusion, 1)
            && !(*GameRules & RULES_BLIND_RESEARCH)) {
                value *= 2;
            }
        }
        int eco_dmg_unk = f->unk_47 / clamp(base_count, 1, 9999);
        if (eco_dmg_unk > f->AI_power
        && (tech_is_preq(tech_id, Facility[FAC_HYBRID_FOREST].preq_tech, 9999)
        || tech_is_preq(tech_id, Facility[FAC_TREE_FARM].preq_tech, 9999)
        || tech_is_preq(tech_id, Facility[FAC_CENTAURI_PRESERVE].preq_tech, 9999)
        || tech_is_preq(tech_id, Facility[FAC_TEMPLE_OF_PLANET].preq_tech, 9999))) {
            value += eco_dmg_unk;
        }
        if (m->rule_population > 0) {
            if (tech_is_preq(tech_id, Facility[FAC_HAB_COMPLEX].preq_tech, 9999)) {
                value *= 2;
            } else if (*CurrentTurn > 250
            && tech_is_preq(tech_id, Facility[FAC_HABITATION_DOME].preq_tech, 9999)) {
                value = (value * 3) / 2;
            }
        }
        if (f->AI_power) {
            for (int i = 0; i < MaxWeaponNum; i++) {
                if (Weapon[i].offense_value) {
                    int weap_preq_tech = Weapon[i].preq_tech;
                    if (tech_id == weap_preq_tech) {
                        value *= (is_player + 3);
                    } else if (tech_is_preq(tech_id, weap_preq_tech, 1)) {
                        value *= (is_player + 2);
                    }
                }
            }
        }
        if (f->AI_tech || !f->AI_power) {
            for (int i = 0; i < MaxTechnologyNum; i++) {
                if (!has_tech(i, faction_id) && Tech[i].flags & TFLAG_SECRETS
                && !TechOwners[i] && tech_is_preq(tech_id, i, 1)) {
                    value *= 3;
                }
            }
        }
        if (tech_is_preq(tech_id, Units[BSC_FORMERS].preq_tech, 9999)
        && !has_tech(Units[BSC_FORMERS].preq_tech, faction_id)) {
            value *= 2;
            if (is_player) {
                value *= 2;
            }
        }
        if (tech_is_preq(tech_id, Chassis[CHS_FOIL].preq_tech, 9999)
        && !has_tech(Chassis[CHS_FOIL].preq_tech, faction_id)) {
            bool toggle = false;
            for (int region = 1; region < MaxRegionLandNum; region++) {
                if (f->region_total_bases[region]) {
                    for (int i = 1; i < MaxPlayerNum; i++) {
                        if (faction_id != i && !Factions[i].region_total_bases[region]) {
                            toggle = true;
                            break;
                        }
                    }
                    if (toggle && f->region_visible_tiles[region]
                    >= Continents[region].tile_count) {
                        value *= 3;
                        if (is_player) {
                            value *= 2;
                        }
                        break;
                    }
                }
            }
            if (toggle) {
                value = (value * 2) + 4;
            }
        }
        if (conf.tech_balance) {
            bool high_cost = revised_tech_cost() && tech_id_lvl > 2;
            if (tech_id == Weapon[WPN_TERRAFORMING_UNIT].preq_tech) {
                value += (high_cost ? 60 : 120);
            } else if (tech_id == Weapon[WPN_SUPPLY_TRANSPORT].preq_tech
            || tech_id == Facility[FAC_RECYCLING_TANKS].preq_tech
            || tech_id == Facility[FAC_CHILDREN_CRECHE].preq_tech
            || tech_id == Facility[FAC_RECREATION_COMMONS].preq_tech
            || tech_id == Rules->tech_preq_allow_3_nutrients_sq
            || tech_id == Rules->tech_preq_allow_3_minerals_sq
            || tech_id == Rules->tech_preq_allow_3_energy_sq) {
                value += (high_cost ? 20 : 40);
            }
        }
    } else if (tech_id < 97) { // Factions
        int factor = 1;
        int faction_id_2 = tech_id - MaxTechnologyNum;
        if (!mod_wants_to_attack(faction_id, faction_id_2, 0)) {
            factor++;
        }
        if (!mod_wants_to_attack(faction_id_2, faction_id, 0)) {
            factor++;
        }
        value = factor * (factor / (f->AI_fight + 2));
    } else {  // Prototypes
        UNIT* u = &Units[tech_id - 97];
        value = clamp(u->offense_value(), 1, 2)
            + clamp(u->defense_value(), 1, 2)
            + clamp((int)u->speed(), 1, 2)
            + u->reactor_id - 2;
    }
    return value;
}

int __cdecl mod_tech_ai(int faction_id) {
    using namespace ai;
    int tech_id = -1;
    int best_value = INT_MIN;
    for (int i = 0; i < MaxTechnologyNum; i++) {
        if (tech_avail(i, faction_id)) {
            int tech_value = mod_tech_val(i, faction_id, false);
            debug("tech_val %d %d value: %3d tech: %2d %s\n",
            *CurrentTurn, faction_id, tech_value, i, Tech[i].name);
            if (*GameRules & RULES_BLIND_RESEARCH) {
                if (is_human(faction_id) && i == Units[BSC_FORMERS].preq_tech
                && (Factions[faction_id].AI_growth || Factions[faction_id].AI_wealth)) {
                    return i;
                }
                int preq = tech_level(i, 0); // Replaces tech_recurse
                tech_value = preq ? (tech_value << 8) / preq : 0;
            }
            // Replace previous functions game_random (for players) and game_rand (for AIs)
            int value = random_get(0, tech_value + 1);
            if (value > best_value) {
                best_value = value;
                tech_id = i;
            }
        }
    }
    return tech_id;
}

} // namespace engine
} // namespace alpha
