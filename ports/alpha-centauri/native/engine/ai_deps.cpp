/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP6 computer players: helper copies.
 * Ported from Thinker Mod src/base.cpp, src/faction.cpp, src/game.cpp,
 * src/map.cpp, src/net.cpp, src/veh.cpp, src/veh_action.cpp and src/veh_combat.cpp at dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * Deviations from Thinker are marked "Fix:" (behaviour) or "Original:"
 * (replaced executable or Windows code) and listed in ENGINE-WP6.md.
 */
#include "ai.h"

namespace alpha {
namespace engine {
namespace ai {

/*
 * These are private copies (namespace alpha::engine::ai) of Thinker helpers
 * that the AI needs and that are not in engine_wp.h. Several of them are also
 * ported by WP2..WP5 inside their own packages; the copies here are pure
 * queries or AI-only actions and can be replaced by the shared versions once
 * those land (see ENGINE-WP6.md).
 */

// Thinker src/game.cpp un_charter
bool un_charter() {
    return ProposalPassCount[PROP_REPEAL_UN_CHARTER]
        <= ProposalPassCount[PROP_REINSTATE_UN_CHARTER];
}

// Thinker src/game.cpp victory_done
bool victory_done() {
    return (*GameState & STATE_GAME_DONE);
}

// Thinker src/game.cpp voice_of_planet
bool voice_of_planet() {
    // Even destroyed Voice of Planet allows building Ascent to Transcendence
    return project_base(FAC_VOICE_OF_PLANET) != SP_Unbuilt;
}

// Thinker src/faction.cpp has_aircraft
bool has_aircraft(int faction_id) {
    for (int i = 0; i < MaxChassisNum; i++) {
        if (Chassis[i].triad == TRIAD_AIR && !Chassis[i].missile
        && has_tech(Chassis[i].preq_tech, faction_id)) {
            return true;
        }
    }
    for (int i = 0; i < MaxProtoFactionNum; i++) {
        UNIT* u = &Units[i];
        if (veh_avail(i, faction_id, -1) && u->triad() == TRIAD_AIR && !u->is_missile()) {
            return true;
        }
    }
    return false;
}

// Thinker src/faction.cpp has_ships
bool has_ships(int faction_id) {
    for (int i = 0; i < MaxChassisNum; i++) {
        if (Chassis[i].triad == TRIAD_SEA
        && has_tech(Chassis[i].preq_tech, faction_id)) {
            return true;
        }
    }
    for (int i = 0; i < MaxProtoFactionNum; i++) {
        UNIT* u = &Units[i];
        if (veh_avail(i, faction_id, -1) && u->triad() == TRIAD_SEA) {
            return true;
        }
    }
    return false;
}

// Thinker src/faction.cpp has_wmode
bool has_wmode(int faction_id, VehWeaponMode mode) {
    for (int i = 0; i < MaxWeaponNum; i++) {
        if (Weapon[i].mode == mode && has_tech(Weapon[i].preq_tech, faction_id)) {
            return true;
        }
    }
    for (int i = 0; i < MaxProtoFactionNum; i++) {
        UNIT* u = &Units[i];
        if (veh_avail(i, faction_id, -1) && u->weapon_mode() == mode) {
            return true;
        }
    }
    return false;
}

// Thinker src/faction.cpp council_votes
/*
Calculate faction's vote count. Used for Planetary Governor and Supreme Leader.
Return Value: Faction vote count
*/
int __cdecl council_votes(int faction_id) {
    if (is_alien(faction_id)) {
        return 0;
    }
    int votes = 0;
    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].faction_id == faction_id ) {
            votes += Bases[i].pop_size;
        }
    }
    if (has_project(FAC_EMPATH_GUILD, faction_id)) {
        votes += votes / 2; // +50% votes
    }
    if (has_project(FAC_CLINICAL_IMMORTALITY, faction_id)) {
        votes *= 2; // Doubles votes
    }
    int bonus_count = MFactions[faction_id].faction_bonus_count;
    for (int i = 0; i < bonus_count; i++) {
        if (MFactions[faction_id].faction_bonus_id[i] == RULE_VOTES) {
            int votes_bonus = MFactions[faction_id].faction_bonus_val1[i];
            if (votes_bonus >= 0) {
                votes *= votes_bonus; // Peacekeeper bonus
            }
        }
    }
    return votes;
}

// Thinker src/faction.cpp aah_ooga
/*
Check whether the specified faction is nearing the diplomatic victory requirements to be
able to call a Supreme Leader vote. Optional 2nd parameter (0/-1 to disable) that specifies
a faction to skip if they have a pact with faction from the 1st parameter.
Return Value: faction_id if nearing diplomatic victory or zero
*/
int __cdecl aah_ooga(int faction_id, int pact_faction_id) {
    if (!(*GameRules & RULES_VICTORY_DIPLOMATIC)) {
        return 0; // Diplomatic Victory not allowed
    }
    int votes_total = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        votes_total += council_votes(i);
    }
    int value = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (i != pact_faction_id
        && (pact_faction_id <= 0 || !has_treaty(i, pact_faction_id, DIPLO_PACT)
        || !(*GameRules & RULES_VICTORY_COOPERATIVE))) {
            int proposal_preq = Proposal[PROP_UNITE_SUPREME_LEADER].preq_tech;
            if ((has_tech(proposal_preq, faction_id)
            || (proposal_preq >= 0
            && (has_tech(Tech[proposal_preq].preq_tech1, faction_id)
            || has_tech(Tech[proposal_preq].preq_tech2, faction_id))))
            && council_votes(i) * 2 >= votes_total && (!value || i == faction_id)) {
                value = i;
            }
        }
    }
    return value;
}

// Thinker src/faction.cpp great_beelzebub
/*
Determine if the overall dominant human faction is a minor threat based on base count.
Return Value: Is faction minor threat? true/false
*/
int __cdecl great_beelzebub(int faction_id, int is_aggressive) {
    if (is_human(faction_id) && FactionRankings[7] == faction_id) {
        int bases_threat = (*CurrentTurn + 25) / 50;
        if (bases_threat < 4) {
            bases_threat = 4;
        }
        if (Factions[faction_id].base_count > bases_threat
        && (Factions[faction_id].diff_level > DIFF_SPECIALIST
        || *GameRules & RULES_INTENSE_RIVALRY || is_aggressive)) {
            return true;
        }
    }
    return false;
}

// Thinker src/faction.cpp climactic_battle
/*
Check if the human controlled player is nearing the endgame.
Return Value: Is human player nearing endgame? true/false
*/
int __cdecl climactic_battle() {
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (is_human(i) && Factions[i].corner_market_turn > *CurrentTurn) {
            return true;
        }
    }
    // Removed following code since this will always return false.
    // This may have been disabled as a design decision rather than a bug.
    // if (aah_ooga(0, -1)) { return true; }
    assert(!aah_ooga(0, -1));

    if (voice_of_planet()) { // Replace function ascending(0)
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (is_human(i) && (has_tech(Facility[FAC_PSI_GATE].preq_tech, i)
            || has_tech(Facility[FAC_VOICE_OF_PLANET].preq_tech, i))) {
                return true;
            }
        }
    }
    return false;
}

// Thinker src/faction.cpp great_satan
/*
Determine if the specified faction is considered a threat based on the game state and ranking.
Return Value: Is the specified faction a threat? true/false
*/
int __cdecl great_satan(int faction_id, int is_aggressive) {
    if (great_beelzebub(faction_id, is_aggressive)) {
        bool intense_rivalry = (*GameRules & RULES_INTENSE_RIVALRY);
        if (*CurrentTurn <= ((intense_rivalry ? 0
        : (DIFF_TRANSCEND - Factions[faction_id].diff_level) * 50) + 100)) {
            return false;
        }
        if (climactic_battle() && aah_ooga(faction_id, -1) == faction_id) {
            return true;
        }
        int diff_factor;
        int factor;
        if (intense_rivalry) {
            factor = 4;
            diff_factor = DIFF_TRANSCEND;
        } else if (Factions[faction_id].diff_level >= DIFF_LIBRARIAN
        || *GameRules & RULES_VICTORY_CONQUEST || *ObjectiveReqVictory <= 1000) {
            factor = 2;
            diff_factor = DIFF_LIBRARIAN;
        } else {
            factor = 1;
            diff_factor = DIFF_TALENT;
        }
        return (factor * FactionRankingsUnk[FactionRankings[7]]
            >= diff_factor * FactionRankingsUnk[FactionRankings[6]]);
    }
    return false;
}

// Thinker src/faction.cpp evaluate_attack
/*
Determine if the specified faction wants to attack the target faction.
Lower values of modifier will make the faction more likely to attack.
*/
static int __cdecl evaluate_attack(int faction_id, int faction_id_tgt, int faction_id_unk) {
    Faction* plr = &Factions[faction_id];
    Faction* plr_tgt = &Factions[faction_id_tgt];
    int32_t peace_faction_id = 0;
    bool common_enemy = false; // Target faction is at war with a human faction we are allied with
    if (MFactions[faction_id].is_alien() && MFactions[faction_id_tgt].is_alien()) {
        return true;
    }
    if (has_treaty(faction_id, faction_id_tgt, DIPLO_WANT_REVENGE | DIPLO_UNK_40 | DIPLO_ATROCITY_VICTIM)) {
        return true;
    }
    if (!plr->major_atrocities && plr_tgt->major_atrocities) {
        return true;
    }
    if (has_treaty(faction_id, faction_id_tgt, DIPLO_UNK_4000000)) {
        return false;
    }
    if (!is_human(faction_id_tgt) && plr->player_flags & PFLAG_TEAM_UP_VS_HUMAN) {
        return false;
    }
    int32_t modifier = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (i != faction_id && i != faction_id_tgt) {
            bool has_surrender = has_treaty(faction_id, i, DIPLO_HAVE_SURRENDERED);
            if (has_surrender && has_treaty(faction_id, i, DIPLO_PACT)) {
                peace_faction_id = i;
            }
            if (has_treaty(faction_id, i, DIPLO_VENDETTA)
            && !has_treaty(faction_id_tgt, i, DIPLO_PACT)) {
                modifier++;
                if (Factions[i].mil_strength_1 > ((plr_tgt->mil_strength_1 * 3) / 2)) {
                    modifier++;
                }
            }
            if (great_beelzebub(i, false)
            && (*CurrentTurn >= 100 || *GameRules & RULES_INTENSE_RIVALRY)) {
                if (has_treaty(faction_id_tgt, i, DIPLO_VENDETTA)) {
                    modifier++;
                }
                if (has_treaty(faction_id_tgt, i, DIPLO_COMMLINK)
                && has_treaty(faction_id, i, DIPLO_COMMLINK)) {
                    modifier++;
                }
            }
            if (has_treaty(faction_id, i, DIPLO_PACT) && is_human(i)) {
                if (has_treaty(faction_id_tgt, i, DIPLO_PACT)) {
                    modifier += 2;
                }
                if (has_surrender && has_treaty(i, faction_id_tgt, DIPLO_PACT | DIPLO_TREATY)) {
                    return false;
                }
                if (has_treaty(faction_id_tgt, i, DIPLO_VENDETTA)) {
                    common_enemy = true;
                    modifier -= (has_surrender ? 4 : 2);
                }
            }
        }
    }
    if (peace_faction_id) {
        if (has_treaty(faction_id_tgt, peace_faction_id, DIPLO_VENDETTA)) {
            return true;
        }
        if (has_treaty(faction_id_tgt, peace_faction_id, DIPLO_PACT | DIPLO_TREATY)) {
            return false;
        }
    }
    if (plr->AI_fight < 0 && !common_enemy && FactionRankings[7] != faction_id_tgt) {
        return false;
    }
    // Modify attacks to be less likely when AI has only few bases
    int32_t score = plr->base_count
        + (plr->best_armor_value > 1 ? 5 : 0)
        + (plr->best_weapon_value > 1 ? 5 : 0);
    if (score <= ((*CurrentTurn + faction_id) & 15)) {
        return false;
    }
    // Fix: initialize array to zero, original doesn't and compares arbitrary data on stack
    uint32_t region_top_base_count[MaxPlayerNum] = {};
    for (int region = 1; region < MaxRegionLandNum; region++) {
        for (int i = 1; i < MaxPlayerNum; i++) {
            uint32_t total_bases = Factions[i].region_total_bases[region];
            if (total_bases > region_top_base_count[i]) {
                region_top_base_count[i] = total_bases;
            }
        }
    }
    for (int i = 1; i < MaxPlayerNum; i++) {
        region_top_base_count[i] -= (region_top_base_count[i] / 4);
    }
    int32_t region_target_hq = -1;
    int32_t region_hq = -1;
    for (int i = 0; i < *BaseCount; i++) {
        if (has_fac_built(FAC_HEADQUARTERS, i)) {
            if (Bases[i].faction_id == faction_id) {
                region_hq = region_at(Bases[i].x, Bases[i].y);
            } else if (Bases[i].faction_id == faction_id_tgt) {
                region_target_hq = region_at(Bases[i].x, Bases[i].y);
            }
        }
    }
    int32_t factor_force_rating = 0;
    int32_t factor_count = 0;
    int32_t factor_unk = 1;
    for (int region = 1; region < MaxRegionLandNum; region++) {
        uint32_t force_rating = plr->region_force_rating[region];
        if (!bad_reg(region) && force_rating) {
            uint32_t total_cmbt_vehs = plr_tgt->region_total_combat_units[region];
            uint32_t total_bases_tgt = plr_tgt->region_total_bases[region];

            if (total_cmbt_vehs || total_bases_tgt) {
                if (plr->region_total_bases[region]
                >= ((region_top_base_count[faction_id] / 4) * 3) || region == region_hq) {
                    int32_t compare = force_rating +
                        plr->region_total_combat_units[region] +
                        (faction_id_unk > 0
                            ? Factions[faction_id_unk].region_force_rating[region] / 4 : 0);
                    if (plr_tgt->region_force_rating[region] > compare) {
                        return false;
                    }
                }
                if (total_bases_tgt) {
                    factor_force_rating += force_rating + (faction_id_unk > 0
                        ? Factions[faction_id_unk].region_force_rating[region] / 2 : 0);
                }
                if ((total_bases_tgt >= ((region_top_base_count[faction_id_tgt] / 4) * 3)
                || region == region_target_hq) && force_rating > total_cmbt_vehs) {
                    factor_force_rating += force_rating + (faction_id_unk > 0
                        ? Factions[faction_id_unk].region_force_rating[region] / 2 : 0);
                }
                factor_unk += total_cmbt_vehs + (plr->region_total_bases[region]
                    ? plr_tgt->region_force_rating[region] / 2 : 0);
                if (plr->region_total_bases[region]) {
                    factor_count++;
                }
            }
        }
    }
    modifier -= plr->AI_fight * 2;
    if (plr->tech_commerce_bonus > ((plr_tgt->tech_commerce_bonus * 3) / 2)) {
        modifier++;
    }
    if (plr->tech_commerce_bonus < ((plr_tgt->tech_commerce_bonus * 2) / 3)) {
        modifier--;
    }
    if (plr->best_weapon_value > (plr_tgt->best_armor_value * 2)) {
        modifier--;
    }
    if (plr->best_weapon_value <= plr_tgt->best_armor_value) {
        modifier++;
    }
    if (!has_treaty(faction_id, faction_id_tgt, DIPLO_VENDETTA)) {
        modifier++;
    }
    if (has_treaty(faction_id, faction_id_tgt, DIPLO_PACT)) {
        modifier++;
    }
    if (faction_id_unk > 0 && !great_satan(faction_id_unk, false)) {
        modifier--;
    }
    if (has_agenda(faction_id, faction_id_tgt, AGENDA_UNK_200)
    && *GameRules & RULES_INTENSE_RIVALRY) {
        modifier--;
    }
    modifier -= clamp((plr_tgt->integrity_blemishes
        - plr->integrity_blemishes + 2) / 3, 0, 2);
    int32_t morale_factor = clamp(plr->SE_morale_pending, -4, 4)
        + MFactions[faction_id].rule_morale + 16;
    int32_t morale_divisor = factor_unk * (clamp(plr_tgt->SE_morale_pending, -4, 4)
        + MFactions[faction_id_tgt].rule_morale + 16);

    if (!morale_divisor) {
        assert(0);
        return false;
    }
    if ((factor_count || modifier > 0 || has_treaty(faction_id, faction_id_tgt, DIPLO_UNK_20000000))
    && ((morale_factor * factor_force_rating * 6) / morale_divisor) < (modifier + 6)) {
        return false;
    }
    return true;
}

// Thinker src/faction.cpp mod_wants_to_attack
int __cdecl mod_wants_to_attack(int faction_id, int faction_id_tgt, int faction_id_unk) {
    int value = evaluate_attack(faction_id, faction_id_tgt, faction_id_unk);

    debug("wants_to_attack turn: %d factions: %d %d %d value: %d\n",
        *CurrentTurn, faction_id, faction_id_tgt, faction_id_unk, value);
    return value;
}

// Thinker src/base.cpp base_can_riot
bool base_can_riot(int base_id, bool allow_staple) {
    BASE* b = &Bases[base_id];
    return (!allow_staple || !b->nerve_staple_turns_left)
        && !has_project(FAC_TELEPATHIC_MATRIX, b->faction_id)
        && !has_fac_built(FAC_PUNISHMENT_SPHERE, base_id);
}

// Thinker src/base.cpp base_unused_space
int base_unused_space(int base_id) {
    BASE* base = &Bases[base_id];
    int limit_mod = (has_project(FAC_ASCETIC_VIRTUES, base->faction_id) ? 2 : 0)
        - MFactions[base->faction_id].rule_population;

    if (!has_fac_built(FAC_HAB_COMPLEX, base_id)) {
        return max(0, Rules->pop_limit_wo_hab_complex + limit_mod - base->pop_size);
    }
    if (!has_fac_built(FAC_HABITATION_DOME, base_id)) {
        return max(0, Rules->pop_limit_wo_hab_dome + limit_mod - base->pop_size);
    }
    return max(0, MaxBasePopSize - base->pop_size);
}

// Thinker src/base.cpp base_growth_goal
int base_growth_goal(int base_id) {
    BASE* base = &Bases[base_id];
    return clamp((32 - base->pop_size)/2, 0, base_unused_space(base_id));
}

// Thinker src/base.cpp valid_relocate_base
static bool valid_relocate_base(int base_id) {
    int faction_id = Bases[base_id].faction_id;
    int best_id = -1;
    int best_score = 0;

    if (conf.auto_relocate_hq || Bases[base_id].mineral_surplus < 2) {
        return false;
    }
    if (!has_tech(Facility[FAC_HEADQUARTERS].preq_tech, faction_id)) {
        return false;
    }
    for (int i = 0; i < *BaseCount; i++) {
        BASE* b = &Bases[i];
        if (b->faction_id == faction_id) {
            if (has_fac_built(FAC_HEADQUARTERS, i)) {
                return false;
            }
            if (b->item() == -FAC_HEADQUARTERS) {
                return base_id == i;
            }
            int score = 8*(i == base_id) + 2*b->pop_size
                + b->mineral_surplus - b->assimilation_turns_left;
            if (best_id < 0 || score > 2 * best_score) {
                best_id = i;
                best_score = score;
            }
        }
    }
    return best_id == base_id;
}

// Thinker src/base.cpp can_build
bool can_build(int base_id, int item_id) {
    assert(base_id >= 0 && base_id < *BaseCount);
    assert(item_id > 0 && item_id <= FAC_EMPTY_SP_64);
    BASE* base = &Bases[base_id];
    int faction_id = base->faction_id;
    Faction* f = &Factions[faction_id];

    // First check strict facility availability by the original games rules
    // Then check various other usefulness conditions to avoid unnecessary builds
    if (!facility_avail((FacilityId)item_id, faction_id, base_id, 0)) {
        return false;
    }
    if (item_id == FAC_STOCKPILE_ENERGY) {
        return base->mineral_consumption > base->mineral_surplus && random(4);
    }
    if (item_id == FAC_ASCENT_TO_TRANSCENDENCE || item_id == FAC_VOICE_OF_PLANET) {
        if (victory_done()) {
            return false;
        }
        if (is_alien(faction_id) && has_tech(Facility[FAC_SUBSPACE_GENERATOR].preq_tech, faction_id)) {
            return false;
        }
    }
    if (item_id >= SP_ID_First && item_id <= SP_ID_Last) {
        int tech_id = Facility[item_id].preq_tech;
        return tech_id == TECH_None || *DiffLevel >= conf.limit_project_start
            || (tech_id >= 0 && TechOwners[tech_id] & FactionStatus[0]);
    }
    if (item_id == FAC_HEADQUARTERS && !valid_relocate_base(base_id)) {
        return false;
    }
    if ((item_id == FAC_RECREATION_COMMONS || item_id == FAC_HOLOGRAM_THEATRE
    || item_id == FAC_PUNISHMENT_SPHERE || item_id == FAC_PARADISE_GARDEN)
    && !base_can_riot(base_id, false)) {
        return false;
    }
    if (item_id == FAC_PUNISHMENT_SPHERE && (project_base(FAC_SUPERCOLLIDER) == base_id
    || project_base(FAC_THEORY_OF_EVERYTHING) == base_id)) {
        return false;
    }
    if (*GameRules & RULES_SCN_NO_TECH_ADVANCES) {
        if (item_id == FAC_RESEARCH_HOSPITAL || item_id == FAC_NANOHOSPITAL
        || item_id == FAC_FUSION_LAB || item_id == FAC_QUANTUM_LAB) {
            return false;
        }
    }
    if (item_id == FAC_GENEJACK_FACTORY && base_can_riot(base_id, false)
    && base->talent_total + 1 < base->drone_total + Rules->drones_induced_genejack_factory) {
        return false;
    }
    if ((item_id == FAC_HAB_COMPLEX || item_id == FAC_HABITATION_DOME)
    && (base->nutrient_surplus < 0 || base_unused_space(base_id) > 1)) {
        return false;
    }
    if (item_id == FAC_PRESSURE_DOME && (*ClimateFutureChange <= 0
    || !is_shore_level(mapsq(base->x, base->y)))) {
        return false;
    }
    if (item_id >= FAC_EMPTY_FACILITY_42 && item_id <= FAC_EMPTY_FACILITY_64
    && Facility[item_id].maint >= 0) { // Negative maintenance or income is possible
        return false;
    }
    if ((item_id == FAC_COMMAND_CENTER || item_id == FAC_NAVAL_YARD)
    && (!conf.repair_base_facility || f->SE_morale < -1)) {
        return false;
    }
    if (item_id == FAC_BIOENHANCEMENT_CENTER && f->SE_morale < -1)  {
        return false;
    }
    if (item_id == FAC_COVERT_OPS_CENTER && has_project(FAC_HUNTER_SEEKER_ALGORITHM, faction_id)) {
        return false;
    }
    if (item_id == FAC_PSI_GATE && 4*facility_count(FAC_PSI_GATE, faction_id) >= f->base_count) {
        return false;
    }
    if (item_id == FAC_SUBSPACE_GENERATOR) {
        if (!is_alien(faction_id) || base->pop_size < Rules->base_size_subspace_gen) {
            return false;
        }
        int n = 0;
        for (int i = 0; i < *BaseCount; i++) {
            BASE* b = &Bases[i];
            if (b->faction_id == faction_id && has_facility(FAC_SUBSPACE_GENERATOR, i)
            && b->pop_size >= Rules->base_size_subspace_gen
            && ++n >= Rules->subspace_gen_req) {
                return false;
            }
        }
    }
    if (item_id >= FAC_SKY_HYDRO_LAB && item_id <= FAC_ORBITAL_DEFENSE_POD) {
        int prod_num = prod_count(-item_id, faction_id, base_id);
        int goal_num = satellite_goal(faction_id, item_id);
        int built_num = satellite_count(faction_id, item_id);
        if (built_num + prod_num >= goal_num) {
            return false;
        }
    }
    if (item_id == FAC_FLECHETTE_DEFENSE_SYS && f->satellites_ODP > 0) {
        return false;
    }
    if (item_id == FAC_GEOSYNC_SURVEY_POD || item_id == FAC_FLECHETTE_DEFENSE_SYS) {
        for (int i = 0; i < *BaseCount; i++) {
            BASE* b = &Bases[i];
            if (b->faction_id == faction_id && i != base_id
            && map_range(base->x, base->y, b->x, b->y) <= 3
            && (has_facility(FAC_GEOSYNC_SURVEY_POD, i)
            || has_facility(FAC_FLECHETTE_DEFENSE_SYS, i)
            || b->item() == -FAC_GEOSYNC_SURVEY_POD
            || b->item() == -FAC_FLECHETTE_DEFENSE_SYS)) {
                return false;
            }
        }
    }
    return true;
}

// Thinker src/base.cpp can_build_unit
bool can_build_unit(int base_id, int unit_id) {
    assert(base_id >= 0 && base_id < *BaseCount && unit_id >= -1);
    BASE* b = &Bases[base_id];
    if (unit_id >= 0 && !(unit_id < MaxProtoFactionNum
    || (unit_id / MaxProtoFactionNum) == b->faction_id)) {
        return false;
    }
    if (unit_id >= 0 && Units[unit_id].is_colony() && b->nutrient_surplus <= 0
    && b->pop_size == 1 && Factions[b->faction_id].diff_level > 1
    && (*DiffLevel <= 3 || is_human(b->faction_id))) {
        return false;
    }
    if (unit_id >= 0 && Units[unit_id].triad() == TRIAD_SEA
    && !adjacent_region(b->x, b->y, -1, 10, TRIAD_SEA)) {
        return false;
    }
    return (*VehCount + 32 < conf.max_veh_num) || (*VehCount + random(32) < conf.max_veh_num);
}

// Thinker src/base.cpp can_link_artifact
bool can_link_artifact(int base_id) {
    return base_id >= 0 && ((has_fac_built(FAC_NETWORK_NODE, base_id)
        && !(Bases[base_id].state_flags & BSTATE_ARTIFACT_LINKED))
        || project_base(FAC_UNIVERSAL_TRANSLATOR) == base_id);
}

// Thinker src/base.cpp can_staple
bool can_staple(int base_id) {
    return base_id >= 0 && conf.nerve_staple_turns > 0
        && Bases[base_id].SE_police(SE_Current) >= 0;
}

// Thinker src/base.cpp can_use_teleport
bool can_use_teleport(int base_id) {
    return has_fac_built(FAC_PSI_GATE, base_id)
        && !(Bases[base_id].state_flags & BSTATE_PSI_GATE_USED);
}

// Thinker src/base.cpp unit_support_plan
int unit_support_plan() {
    return (conf.modify_unit_support == 1 ? PLAN_SUPPLY :
        (conf.modify_unit_support == 2 ? PLAN_PROBE : PLAN_TERRAFORM));
}

// Thinker src/base.cpp prod_name
char* prod_name(int item_id) {
    assert(item_id >= -SP_ID_Last && item_id < MaxProtoNum);
    if (item_id >= 0) {
        return Units[item_id].name;
    } else {
        return Facility[-item_id].name;
    }
}

// Thinker src/base.cpp psych_check
/*
Calculate the base count threshold for possible bureaucracy notifications in Console::new_base.
*/
void __cdecl psych_check(int faction_id, int32_t* content_pop, int32_t* base_limit) {
    size_t diff_level = *DiffLevel;
    if (faction_id >= 0 && faction_id < MaxPlayerNum && diff_level < MaxDiffNum) {
        *content_pop = (is_human(faction_id) ?
            conf.content_pop_player[diff_level] : conf.content_pop_computer[diff_level]);
        *base_limit = (((*content_pop + 2) * max(4, 4 + Factions[faction_id].SE_effic_pending)
            * *MapAreaSqRoot) / 56) / 2;
    } else {
        *content_pop = 0;
        *base_limit = 0;
        assert(0);
    }
}

// Thinker src/base.cpp mineral_factor
int __cdecl mineral_factor(int faction_id, int SE_industry) {
    int value;
    int multiplier = Rules->mineral_cost_multi;
    if (is_human(faction_id)) {
        value = multiplier;
    } else {
        value = conf.cost_factor[*DiffLevel];
        if (!conf.simple_cost_factor) {
            value -= (great_satan(FactionRankings[7], 0) != 0);
            value -= (!*MultiplayerActive && is_human(FactionRankings[7]));
        }
        value = multiplier * value / 10;
    }
    if (*MapSizePlanet == 0) {
        value = 8 * value / 10;
    } else if (*MapSizePlanet == 1) {
        value = 9 * value / 10;
    }
    switch (max(SE_industry, -3)) {
        case -3:
            value = (13 * value + 9) / 10;
            break;
        case -2:
            value = (6 * value + 4) / 5;
            break;
        case -1:
            value = (11 * value + 9) / 10;
            break;
        case 0:
            break;
        case 1:
            value = (9 * value + 9) / 10;
            break;
        case 2:
            value = (4 * value + 4) / 5;
            break;
        case 3:
            value = (7 * value + 9) / 10;
            break;
        case 4:
            value = (3 * value + 4) / 5;
            break;
        default: // +5 Industry or better
            value = (value + 1) / 2;
    }
    return max(value, 1);
}

// Thinker src/base.cpp mineral_output_modifier
int mineral_output_modifier(int base_id) {
    int value = 0;
    if (has_facility(FAC_QUANTUM_CONVERTER, base_id)) {
        value++; // The Singularity Inductor also possible
    }
    if (has_fac_built(FAC_ROBOTIC_ASSEMBLY_PLANT, base_id)) {
        value++;
    }
    if (has_fac_built(FAC_GENEJACK_FACTORY, base_id)) {
        value++;
    }
    if (has_fac_built(FAC_NANOREPLICATOR, base_id)) {
        value++;
    }
    if (has_project(FAC_BULK_MATTER_TRANSMITTER, Bases[base_id].faction_id)) {
        value++;
    }
    return value;
}

// Thinker src/base.cpp mod_bases_reset
void __cdecl mod_bases_reset(int region, int faction_id, int defend_only) {
    if (is_human(faction_id)) {
        return;
    }
    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].faction_id != faction_id) {
            continue;
        }
        MAP* sq = mapsq(Bases[i].x, Bases[i].y);
        if (!sq || (region >= 0 && sq->region != region)) {
            continue;
        }
        if (!defend_only || sq->veh_who() < 0) {
            mod_base_reset(i, 0);
        }
    }
}

// Thinker src/veh.cpp best_armor
VehArmor best_armor(int faction_id, int max_cost) {
    int best_id = ARM_NO_ARMOR;
    for (int i = ARM_NO_ARMOR; i <= ARM_RESONANCE_8_ARMOR; i++) {
        if (has_tech(Armor[i].preq_tech, faction_id)) {
            int cost = Armor[i].cost;
            if (max_cost > 0 && (cost > max_cost || cost > Armor[i].defense_value + 3)) {
                continue;
            }
            int diff = Armor[i].defense_value - Armor[best_id].defense_value;
            if (diff > 0 || (diff == 0 && Armor[best_id].cost >= cost)) {
                best_id = i;
            }
        }
    }
    return (VehArmor)best_id;
}

// Thinker src/veh.cpp best_weapon
VehWeapon best_weapon(int faction_id) {
    int best_id = WPN_HAND_WEAPONS;
    for (int i = 0; i < MaxWeaponNum; i++) {
        if (has_tech(Weapon[i].preq_tech, faction_id)
        && Weapon[i].mode <= WMODE_MISSILE
        && Weapon[i].offense_value < 99
        && i != WPN_CONVENTIONAL_PAYLOAD) {
            int diff = Weapon[i].offense_value - Weapon[best_id].offense_value;
            if (diff > 0 || (diff == 0 && Weapon[best_id].cost >= Weapon[i].cost)) {
                best_id = i;
            }
        }
    }
    return (VehWeapon)best_id;
}

// Thinker src/veh.cpp best_reactor
VehReactor best_reactor(int faction_id) {
    for (VehReactor r : {REC_SINGULARITY, REC_QUANTUM, REC_FUSION}) {
        if (has_tech(Reactor[r - 1].preq_tech, faction_id)) {
            return r;
        }
    }
    return REC_FISSION;
}

// Thinker src/veh.cpp proto_offense
int proto_offense(int unit_id) {
    assert(unit_id >= 0 && unit_id < MaxProtoNum);
    UNIT* u = &Units[unit_id];
    int atk_val = Weapon[u->weapon_id].offense_value;
    if (u->is_planet_buster()) {
        return atk_val * u->reactor_id;
    }
    return (conf.ignore_reactor_power || atk_val < 0 ?
        atk_val * REC_FISSION : atk_val * u->reactor_id);
}

// Thinker src/veh.cpp proto_defense
int proto_defense(int unit_id) {
    assert(unit_id >= 0 && unit_id < MaxProtoNum);
    UNIT* u = &Units[unit_id];
    int def_val = Armor[u->armor_id].defense_value;
    return (conf.ignore_reactor_power || def_val < 0 ?
        def_val * REC_FISSION : def_val * u->reactor_id);
}

// Thinker src/veh.cpp prototype_factor
/*
Determine the extra percent cost for building a prototype. Includes a check if the faction
has the free prototype flag set or if the player is using one of the easier difficulties.
*/
int __cdecl prototype_factor(int unit_id) {
    int faction_id = unit_id / MaxProtoFactionNum;
    if (MFactions[faction_id].rule_flags & RFLAG_FREEPROTO
    || Factions[faction_id].diff_level <= DIFF_SPECIALIST) {
        return 0;
    }
    int triad = Units[unit_id].triad();
    switch (triad) {
    case TRIAD_SEA:
        return Rules->extra_cost_prototype_sea;
    case TRIAD_AIR:
        return Rules->extra_cost_prototype_air;
    case TRIAD_LAND:
    default:
        return Rules->extra_cost_prototype_land;
    }
}

// Thinker src/veh.cpp mod_upgrade_cost
/*
Calculate upgrade cost between two prototypes in mineral rows.
*/
int __cdecl mod_upgrade_cost(int faction_id, int new_unit_id, int old_unit_id) {
    UNIT* old_unit = &Units[old_unit_id];
    UNIT* new_unit = &Units[new_unit_id];
    int cost;

    if (conf.modify_upgrade_cost) {
        if (new_unit->is_supply()) {
            cost = max((int)new_unit->cost, 4*max(1, new_unit->cost - old_unit->cost));
        } else {
            cost = max((int)new_unit->cost, 2*max(1, new_unit->cost - old_unit->cost));
        }
        if (new_unit_id >= MaxProtoFactionNum && !new_unit->is_prototyped()) {
            cost *= 2;
        }
        if (has_project(FAC_NANO_FACTORY, faction_id)) {
            cost /= 2;
        }
    } else {
        int modifier = new_unit->cost;
        if (new_unit_id >= MaxProtoFactionNum && !new_unit->is_prototyped()) {
            modifier = ((new_unit->cost + 1) / 2 + new_unit->cost)
                * ((new_unit->cost + 1) / 2 + new_unit->cost + 1);
        }
        cost = max(0, new_unit->speed() - old_unit->speed())
            + max(0, new_unit->armor_cost() - old_unit->armor_cost())
            + max(0, new_unit->weapon_cost() - old_unit->weapon_cost())
            + modifier;
        if (has_project(FAC_NANO_FACTORY, faction_id)) {
            cost /= 2;
        }
        // Original: assert against the executable upgrade_cost removed.
    }
    return cost;
}

// Thinker src/veh.cpp veh_upgrade
static void veh_upgrade(VEH* veh, int new_unit_id, int old_unit_id, int cost) {
    Faction* f = &Factions[veh->faction_id];
    f->energy_credits -= cost;
    f->units_active[new_unit_id]++;
    f->units_active[old_unit_id]--;
    veh->unit_id = new_unit_id;
    int morale_diff = (has_abil(new_unit_id, ABL_TRAINED) != 0)
        - (has_abil(old_unit_id, ABL_TRAINED) != 0);
    veh->morale = clamp(veh->morale + morale_diff, 0, 6);
    assert(f->energy_credits >= 0);
    assert(new_unit_id / MaxProtoFactionNum == veh->faction_id);
    debug("upgrade_unit %d %d %2d %2d cost: %d %s / %s\n",
    *CurrentTurn, veh->faction_id, veh->x, veh->y, cost, Units[old_unit_id].name, veh->name());
}

// Thinker src/veh.cpp part_upgrade
/*
Partial upgrades until energy reserves run out, MP mode not implemented.
*/
void __cdecl part_upgrade(int faction_id, int new_unit_id, int old_unit_id) {
    Faction* f = &Factions[faction_id];
    if (!*MultiplayerActive && faction_id > 0
    && old_unit_id >= 0 && new_unit_id != old_unit_id
    && new_unit_id / MaxProtoFactionNum == faction_id) {
        int cost = 10 * mod_upgrade_cost(faction_id, new_unit_id, old_unit_id);
        bool active = false;
        for (int i = 0; i < *VehCount; i++) {
            VEH* veh = &Vehs[i];
            if (veh->faction_id == faction_id && veh->unit_id == old_unit_id
            && !veh->moves_spent && !veh->damage_taken && base_at(veh->x, veh->y) >= 0) {
                if (f->energy_credits < cost + 20) {
                    break;
                }
                veh_upgrade(veh, new_unit_id, old_unit_id, cost);
                active = true;
            }
        }
        if (active) {
            draw_map(1);
        }
    }
}

// Thinker src/veh.cpp full_upgrade
/*
Full upgrade to new prototype, retire old prototype and update base queues.
Total cost must be checked by the caller function.
*/
void __cdecl full_upgrade(int faction_id, int new_unit_id, int old_unit_id) {
    int cost = 10 * mod_upgrade_cost(faction_id, new_unit_id, old_unit_id);
    bool active = false;
    for (int i = 0; i < *VehCount; i++) {
        VEH* veh = &Vehs[i];
        if (veh->faction_id == faction_id && veh->unit_id == old_unit_id) {
            veh_upgrade(veh, new_unit_id, old_unit_id, cost);
            active = true;
        }
    }
    for (int i = *BaseCount - 1; i >= 0; --i) {
        BASE* base = &Bases[i];
        if (base->faction_id == faction_id) {
            for (int j = 0; j < 10 && j <= base->queue_size; j++) {
                if (base->queue_items[j] == old_unit_id) {
                    base->queue_items[j] = new_unit_id;
                }
            }
        }
    }
    if (new_unit_id >= MaxProtoFactionNum && active) {
        Units[new_unit_id].unit_flags |= UNIT_PROTOTYPED;
    }
    if (old_unit_id >= MaxProtoFactionNum) {
        Units[old_unit_id].unit_flags &= ~UNIT_ACTIVE;
    }
    if (is_human(faction_id)) {
        draw_map(1);
    }
    // Original: Console_update_data(MapWin, 0) for the active player removed (no UI here).
}

// Thinker src/veh.cpp create_proto
int __cdecl create_proto(int faction_id, VehChassis chs, VehWeapon wpn, VehArmor arm,
VehAblFlag abls, VehReactor rec, VehPlan ai_plan) {
    char name[256];
    name[0] = '\0';
    name_proto(name, -1, faction_id, chs, wpn, arm, abls, rec); // WP5 (mod_name_proto)
    int unit_id = propose_proto(faction_id, chs, wpn, arm, abls, rec, ai_plan, (strlen(name) ? name : NULL));
    debug("create_proto  %d %d chs: %d rec: %d wpn: %2d arm: %2d pln: %2d %08X %d %s\n",
        *CurrentTurn, faction_id, chs, rec, wpn, arm, ai_plan, abls, unit_id, name);
    return unit_id;
}

// Thinker src/veh.cpp mod_veh_skip
int __cdecl mod_veh_skip(int veh_id) {
    veh_skip(veh_id);
    return VEH_SKIP;
}

// Thinker src/veh.cpp set_move_to
int set_move_to(int veh_id, int x, int y) {
    VEH* veh = &Vehs[veh_id];
    debug("set_move_to %2d %2d -> %2d %2d %s\n", veh->x, veh->y, x, y, veh->name());
    if (!mapsq(x, y)) {
        // Fix: trans_move can still find a naval start node after the naval plan
        // was dropped (naval_end -1, -1); skip instead of a go-to order off the map.
        return mod_veh_skip(veh_id);
    }
    veh->waypoint_x[0] = x;
    veh->waypoint_y[0] = y;
    veh->order = ORDER_MOVE_TO;
    veh->status_icon = 'G';
    if (veh->is_former()) {
        veh->movement_turns = 0;
    }
    mapnodes.erase({x, y, NODE_PATROL});
    mapnodes.erase({x, y, NODE_COMBAT_PATROL});
    if (veh->x == x && veh->y == y) {
        return mod_veh_skip(veh_id);
    }
    mapdata[{x, y}].target++;
    return VEH_SYNC;
}

// Thinker src/veh.cpp set_move_next
int set_move_next(int veh_id, int offset) {
    VEH* veh = &Vehs[veh_id];
    int x = wrap(veh->x + BaseOffsetX[offset]);
    int y = veh->y + BaseOffsetY[offset];
    return set_move_to(veh_id, x, y);
}

// Thinker src/veh.cpp set_road_to
int set_road_to(int veh_id, int x, int y) {
    VEH* veh = &Vehs[veh_id];
    veh->waypoint_x[0] = x;
    veh->waypoint_y[0] = y;
    veh->order = ORDER_ROAD_TO;
    veh->status_icon = 'R';
    return VEH_SYNC;
}

// Thinker src/veh.cpp set_action
int set_action(int veh_id, int act, char icon) {
    VEH* veh = &Vehs[veh_id];
    if (act == ORDER_THERMAL_BOREHOLE) {
        mapnodes.insert({veh->x, veh->y, NODE_BOREHOLE});
    } else if (act == ORDER_TERRAFORM_UP) {
        mapnodes.insert({veh->x, veh->y, NODE_RAISE_LAND});
    } else if (act == ORDER_SENSOR_ARRAY) {
        mapnodes.insert({veh->x, veh->y, NODE_SENSOR_ARRAY});
    }
    veh->order = act;
    veh->status_icon = icon;
    veh->state &= ~VSTATE_UNK_10000;
    return VEH_SYNC;
}

// Thinker src/veh.cpp set_order_none
int set_order_none(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    veh->order = ORDER_NONE;
    veh->order_auto_type = ORDERA_TERRA_AUTO_FULL;
    veh->waypoint_x[0] = -1;
    veh->waypoint_y[0] = -1;
    veh->status_icon = '-';
    return mod_veh_skip(veh_id);
}

// Thinker src/veh.cpp set_convoy
int set_convoy(int veh_id, ResType res) {
    VEH* veh = &Vehs[veh_id];
    veh->order_auto_type = res-1;
    veh->order = ORDER_CONVOY;
    veh->status_icon = 'C';
    return mod_veh_skip(veh_id);
}

// Thinker src/veh.cpp set_board_to
int set_board_to(int veh_id, int trans_veh_id) {
    VEH* veh = &Vehs[veh_id];
    VEH* v2 = &Vehs[trans_veh_id];
    assert(veh_id != trans_veh_id);
    assert(veh->x == v2->x && veh->y == v2->y);
    assert(veh_cargo(trans_veh_id) > 0);
    veh->order = ORDER_SENTRY_BOARD;
    veh->waypoint_x[0] = trans_veh_id;
    veh->waypoint_y[0] = 0;
    veh->status_icon = 'L';
    debug("set_board_to %2d %2d %s -> %s\n", veh->x, veh->y, veh->name(), v2->name());
    return VEH_SYNC;
}

// Thinker src/map.cpp bonus_yield
int bonus_yield(int res_type) {
    switch (res_type) {
    case RES_NUTRIENT: return ResInfo->bonus_sq.nutrient;
    case RES_MINERAL: return ResInfo->bonus_sq.mineral;
    case RES_ENERGY: return ResInfo->bonus_sq.energy;
    default: return 0;
    }
}

// Thinker src/map.cpp fungus_yield
int fungus_yield(int faction_id, ResType res_type) {
    Faction* f = &Factions[faction_id];
    int p = clamp(f->SE_planet_pending, -3, 0);
    int N = clamp(f->tech_fungus_nutrient + p, 0, 99);
    int M = clamp(f->tech_fungus_mineral + p, 0, 99);
    int E = clamp(f->tech_fungus_energy + p + (f->SE_economy_pending >= 2), 0, 99);

    if (has_project(FAC_MANIFOLD_HARMONICS, faction_id)) {
        int m = clamp(f->SE_planet_pending + 1, 0, 4);
        N += ManifoldHarmonicsBonus[m][0];
        M += ManifoldHarmonicsBonus[m][1];
        E += ManifoldHarmonicsBonus[m][2];
    }
    if (res_type == RES_NUTRIENT) {
        return N;
    }
    if (res_type == RES_MINERAL) {
        return M;
    }
    if (res_type == RES_ENERGY) {
        return E;
    }
    return N+M+E;
}

// Thinker src/map.cpp total_yield
int total_yield(int x, int y, int faction_id) {
    return mod_crop_yield(faction_id, -1, x, y, 0)
        + mod_mine_yield(faction_id, -1, x, y, 0)
        + mod_energy_yield(faction_id, -1, x, y, 0);
}

// Thinker src/map.cpp item_yield
int item_yield(int x, int y, int faction_id, int bonus, MapItem item) {
    MAP* sq = mapsq(x, y);
    if (!sq) {
        return 0;
    }
    int N = 0, M = 0, E = 0;
    if (item != BIT_FUNGUS) {
        if (bonus == RES_NUTRIENT) {
            N += ResInfo->bonus_sq.nutrient;
        }
        if (bonus == RES_MINERAL) {
            M += ResInfo->bonus_sq.mineral;
        }
        if (bonus == RES_ENERGY) {
            E += ResInfo->bonus_sq.energy;
        }
        if (sq->items & BIT_RIVER && !is_ocean(sq)) {
            E++;
        }
        if (sq->landmarks & LM_CRATER && sq->code_at() < 9) { M++; }
        if (sq->landmarks & LM_VOLCANO && sq->code_at() < 9) { M++; E++; }
        if (sq->landmarks & LM_JUNGLE) { N++; }
        if (sq->landmarks & LM_URANIUM) { E++; }
        if (sq->landmarks & LM_FRESH && is_ocean(sq)) { N++; }
        if (sq->landmarks & LM_GEOTHERMAL) { E++; }
        if (sq->landmarks & LM_RIDGE) { E++; }
        if (sq->landmarks & LM_CANYON) { M++; }
        if (sq->landmarks & LM_FOSSIL && sq->code_at() < 6) { M++; }
        if (Factions[faction_id].SE_economy_pending >= 2) {
            E++;
        }
    }
    if (item == BIT_FOREST) {
        N += ResInfo->forest_sq.nutrient;
        M += ResInfo->forest_sq.mineral;
        E += ResInfo->forest_sq.energy;
    }
    else if (item == BIT_THERMAL_BORE) {
        N += ResInfo->borehole_sq.nutrient;
        M += ResInfo->borehole_sq.mineral;
        E += ResInfo->borehole_sq.energy;
        N -= (N > 0);
    }
    else if (item == BIT_FUNGUS) {
        N = fungus_yield(faction_id, RES_NUTRIENT);
        M = fungus_yield(faction_id, RES_MINERAL);
        E = fungus_yield(faction_id, RES_ENERGY);
    }
    else if (item == BIT_FARM && is_ocean(sq)) {
        N += ResInfo->ocean_sq.nutrient + ResInfo->improved_sea.nutrient;
        M += ResInfo->ocean_sq.mineral;
        E += ResInfo->ocean_sq.energy;
        if (sq->items & BIT_MINE) {
            M += ResInfo->improved_sea.mineral
                + has_tech(Rules->tech_preq_mining_platform_bonus, faction_id);
            N -= (N > 0);
        }
        if (sq->items & BIT_SOLAR) {
            E += ResInfo->improved_sea.energy;
        }
    }
    else {
        assert(0);
    }
    if (N > conf.tile_output_limit[0] && bonus != RES_NUTRIENT
    && !has_tech(Rules->tech_preq_allow_3_nutrients_sq, faction_id)) {
        N = conf.tile_output_limit[0];
    }
    if (M > conf.tile_output_limit[1] && bonus != RES_MINERAL
    && !has_tech(Rules->tech_preq_allow_3_minerals_sq, faction_id)) {
        M = conf.tile_output_limit[1];
    }
    if (E > conf.tile_output_limit[2] && bonus != RES_ENERGY
    && !has_tech(Rules->tech_preq_allow_3_energy_sq, faction_id)) {
        E = conf.tile_output_limit[2];
    }
    if (conf.debug_verbose) {
        debug("item_yield %2d %2d %08X faction: %d bonus: %d planet: %d N: %d M: %d E: %d prev: %d\n",
        x, y, item, faction_id, bonus, Factions[faction_id].SE_planet_pending,
        N, M, E, total_yield(x, y, faction_id));
    }
    return N+M+E;
}

// Thinker src/veh_action.cpp terrain_avail
int __cdecl terrain_avail(FormerItem frm_id, int ocean, int faction_id) {
    assert(frm_id >= FORMER_FARM && frm_id <= FORMER_MONOLITH);
    int preq_tech = (ocean ? Terraform[frm_id].preq_tech_sea : Terraform[frm_id].preq_tech);
    if (preq_tech < TECH_None || (*GameRules & RULES_SCN_NO_TERRAFORMING
    && (frm_id == FORMER_RAISE_LAND || frm_id == FORMER_LOWER_LAND))) {
        return false;
    }
    if (frm_id >= FORMER_CONDENSER && frm_id <= FORMER_LEVEL_TERRAIN
    && has_project(FAC_WEATHER_PARADIGM, faction_id)) {
        return true;
    }
    return has_tech(preq_tech, faction_id);
}

// Thinker src/veh_combat.cpp use_nerve_gas
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

// Thinker src/veh_combat.cpp mod_battle_fight
int __cdecl mod_battle_fight(int veh_id, int offset, int table_offset, int option, int* def_id)
{
    VEH* veh = &Vehs[veh_id];
    int x2, y2;
    if (table_offset) {
        x2 = wrap(veh->x + TableOffsetX[offset]);
        y2 = veh->y + TableOffsetY[offset];
    } else {
        x2 = wrap(veh->x + BaseOffsetX[offset]);
        y2 = veh->y + BaseOffsetY[offset];
    }
    for (int i = 0; i < *VehCount; i++) {
        if (Vehs[i].x == x2 && Vehs[i].y == y2 && Vehs[i].faction_id == veh->faction_id) {
            // Skip alien_move or action_destroy artillery attack on friendly units
            return 0;
        }
    }
    return mod_battle_fight_2(veh_id, offset, x2, y2, table_offset, option, def_id);
}

// Thinker src/net.cpp net_action_build
int __cdecl net_action_build(int veh_id, const char* name) {
    // Original: the multiplayer branch (message_* and NetDaemon) is removed.
    action_build(veh_id, name);
    return VEH_SKIP;
}

// Thinker src/net.cpp net_action_gate
int __cdecl net_action_gate(int veh_id, int base_id) {
    // Original: the multiplayer branch (message_* and NetDaemon) is removed.
    action_gate(veh_id, base_id);
    return VEH_SKIP;
}

// Thinker src/net.cpp net_action_destroy
int __cdecl net_action_destroy(int veh_id, int flag, int x, int y) {
    // Original: the multiplayer branch (message_* and NetDaemon) is removed.
    action_destroy(veh_id, flag, x, y);
    return VEH_SKIP;
}

} // namespace ai
} // namespace engine
} // namespace alpha
