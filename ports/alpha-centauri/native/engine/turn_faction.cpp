/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP1: faction-level turn routines and helpers.
 * social_upkeep/social_upheaval (src/faction.cpp), tech_research (src/tech.cpp),
 * rankings (src/score.cpp), mod_time_warp (src/map.cpp) and mod_auto_save
 * (src/savegame.cpp) are ported from Thinker Mod at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af (MIT; see engine/THINKER-LICENSE).
 * The wp1:: helpers are copies of small Thinker functions whose files belong to
 * other packages (game.cpp, faction.cpp, score.cpp, base.cpp); they keep the
 * Thinker bodies. Deviations are marked "Fix:" or "Original:".
 */
#include "engine.h"
#include "turn.h"

#include <ctype.h>

namespace alpha {
namespace engine {

/* ---- wp1:: local helpers ---- */
namespace wp1 {

bool victory_done() {
    return (*GameState & STATE_GAME_DONE);
}

bool full_game_turn() {
    return !(*GameState & STATE_GAME_DONE) || (*GameState & STATE_FINAL_SCORE_DONE);
}

bool voice_of_planet() {
    // Even destroyed Voice of Planet allows building Ascent to Transcendence
    return project_base(FAC_VOICE_OF_PLANET) != SP_Unbuilt;
}

int council_votes(int faction_id) {
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

int aah_ooga(int faction_id, int pact_faction_id) {
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

int climactic_battle() {
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (is_human(i) && Factions[i].corner_market_turn > *CurrentTurn) {
            return true;
        }
    }
    // Removed following code since this will always return false.
    // This may have been disabled as a design decision rather than a bug.
    // if (aah_ooga(0, -1)) { return true; }
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

int great_beelzebub(int faction_id, int is_aggressive) {
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

int great_satan(int faction_id, int is_aggressive) {
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

int reputation(int faction_id, int faction_id_with) {
    return clamp(Factions[faction_id].integrity_blemishes
        - Factions[faction_id].diplo_gifts[faction_id_with], 0, 99);
}

void set_agenda(int faction_id_1, int faction_id_2, uint32_t status, bool add) {
    if (faction_id_1 >= 0 && faction_id_1 < MaxPlayerNum
    && faction_id_2 >= 0 && faction_id_2 < MaxPlayerNum) {
        if (add) {
            Factions[faction_id_1].diplo_agenda[faction_id_2] |= status;
        } else {
            Factions[faction_id_1].diplo_agenda[faction_id_2] &= ~status;
        }
    }
}

int num_objectives(int faction_id, int incl_pact) {
    if (!is_alive(faction_id)) {
        return 0;
    }
    int score = Factions[faction_id].unk_101;
    for (int i = 0; i < *VehCount; i++) {
        if (Vehs[i].faction_id != faction_id || !(Vehs[i].flags & VFLAG_IS_OBJECTIVE)) {
            continue;
        }
        if (*GameRules & RULES_SCN_VICT_OBJ_UNITS_REACH_FRIEND_OBJ_BASE) {
            int base_id = base_at(Vehs[i].x, Vehs[i].y);
            if (base_id < 0 || !is_objective(base_id)) {
                continue;
            }
            int owner = Bases[base_id].faction_id;
            if (owner == faction_id || ((Factions[faction_id].diplo_status[owner] & DIPLO_PACT)
            && (*GameRules & RULES_VICTORY_COOPERATIVE))) {
                score++;
            }
        } else if (*GameRules & RULES_SCN_VICT_OBJ_UNITS_REACH_FRIEND_HQ_BASE) {
            int base_id = base_at(Vehs[i].x, Vehs[i].y);
            if (base_id < 0 || !has_fac_built(FAC_HEADQUARTERS, base_id)) {
                continue;
            }
            int owner = Bases[base_id].faction_id;
            if (owner == faction_id || ((Factions[faction_id].diplo_status[owner] & DIPLO_PACT)
            && (*GameRules & RULES_VICTORY_COOPERATIVE))) {
                score++;
            }
        } else {
            score++;
        }
    }
    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].faction_id != faction_id) {
            continue;
        }
        if (*GameState & STATE_SCN_VICT_POPULATION_COUNT_OBJ) {
            score += Bases[i].pop_size;
        }
        if ((*GameRules & RULES_SCN_VICT_ALL_BASE_COUNT_OBJ)
        || (Bases[i].event_flags & BEVENT_OBJECTIVE)) {
            score++;
        }
    }
    if (*GameState & STATE_SCN_VICT_TECH_COUNT_OBJ) {
        for (int tech_id = 0; tech_id < MaxTechnologyNum; tech_id++) {
            if (has_tech(tech_id, faction_id)) {
                score++;
            }
        }
        score += Factions[faction_id].tech_count_transcendent;
    }
    if (*GameState & STATE_SCN_VICT_CREDITS_COUNT_OBJ) {
        score += Factions[faction_id].energy_credits;
    }
    if (*GameState & STATE_SCN_VICT_BASE_FACIL_COUNT_OBJ) {
        for (int i = 0; i < *BaseCount; i++) {
            if (Bases[i].faction_id == faction_id
            && *ScnVictFacilityObj >= 0 // Fix: lower bound check
            && *ScnVictFacilityObj <= Fac_ID_Last
            && has_fac_built((FacilityId)*ScnVictFacilityObj, i)) {
                score++;
            }
        }
    }
    if (*GameState & STATE_SCN_VICT_TERRAIN_ENH_COUNT_OBJ) {
        for (int y = 0; y < *MapAreaY; y++) {
            for (int x = y & 1; x < *MapAreaX; x += 2) {
                MAP* sq = mapsq(x, y);
                if (sq && sq->owner == faction_id) {
                    score += bit_count(sq->items & (BIT_SENSOR|BIT_THERMAL_BORE|BIT_ECH_MIRROR|BIT_CONDENSER|\
                        BIT_FOREST|BIT_AIRBASE|BIT_FARM|BIT_BUNKER|BIT_SOLAR|BIT_MINE|BIT_MAGTUBE|BIT_ROAD));
                }
            }
        }
    }
    if (*GameState & STATE_SCN_VICT_TERRITORY_COUNT_OBJ) {
        for (int y = 0; y < *MapAreaY; y++) {
            for (int x = y & 1; x < *MapAreaX; x += 2) {
                MAP* sq = mapsq(x, y);
                if (sq && sq->owner == faction_id) {
                    score++;
                }
            }
        }
    }
    if (*GameRules & RULES_SCN_VICT_SP_COUNT_OBJ) {
        for (int i = 0; i < MaxSecretProjectNum; i++) {
            if (SecretProjects[i] >= 0 && Bases[SecretProjects[i]].faction_id == faction_id) {
                score++;
            }
        }
    }
    if (incl_pact && (*GameRules & RULES_VICTORY_COOPERATIVE)) {
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (i != faction_id && (Factions[faction_id].diplo_status[i] & DIPLO_PACT)) {
                score += num_objectives(i, 0);
            }
        }
    }
    return score;
}

int breed_mod(int base_id, int faction_id) {
    int value = 0;
    if (has_project(FAC_XENOEMPATHY_DOME, faction_id)) {
        value++;
    }
    if (has_project(FAC_PHOLUS_MUTAGEN, faction_id)) {
        value++;
    }
    if (has_project(FAC_VOICE_OF_PLANET, faction_id)) {
        value++;
    }
    if (has_fac_built(FAC_CENTAURI_PRESERVE, base_id)) {
        value++;
    }
    if (has_fac_built(FAC_TEMPLE_OF_PLANET, base_id)) {
        value++;
    }
    if (has_fac_built(FAC_BIOLOGY_LAB, base_id)) {
        value++;
    }
    return value;
}

bool base_can_riot(int base_id, bool allow_staple) {
    BASE* b = &Bases[base_id];
    return (!allow_staple || !b->nerve_staple_turns_left)
        && !has_project(FAC_TELEPATHIC_MATRIX, b->faction_id)
        && !has_fac_built(FAC_PUNISHMENT_SPHERE, base_id);
}

int fac_maint(int facility_id, int faction_id) {
    CFacility& facility = Facility[facility_id];
    MFaction& m = MFactions[faction_id];

    for (int i = 0; i < m.faction_bonus_count; i++) {
        if (m.faction_bonus_val1[i] == facility_id
        && (m.faction_bonus_id[i] == RULE_FACILITY
        || (m.faction_bonus_id[i] == RULE_FREEFAC
        && has_tech(facility.preq_tech, faction_id)))) {
            return 0;
        }
    }
    return facility.maint;
}

/*
Original: energy_limit is terranx.exe 0x445440 and is in no upstream source.
Without a hook it returns 10 (no limit), so allocate_energy keeps only its
efficiency-based balance limits. See ENGINE-WP1.md.
*/
int energy_limit(int faction_id) {
    if (turn_hooks().energy_limit) {
        return turn_hooks().energy_limit(faction_id);
    }
    return 10;
}

void init_save_game(int faction_id) {
    Faction* plr = &Factions[faction_id];
    MFaction* m = &MFactions[faction_id];
    if (!faction_id) {
        return;
    }
    if (!*CurrentTurn) {
        // Thinker clears 20 bytes from thinker_probe_lost (the probe/unused counters).
        uint8_t* p = (uint8_t*)&m->thinker_probe_lost;
        size_t n = min((size_t)20, sizeof(MFaction) - (size_t)(p - (uint8_t*)m));
        memset(p, 0, n);
    }
    m->thinker_unused[0] = 0;
    m->thinker_unused[1] = 0;
    m->thinker_unused[2] = 0;
    if (plr->base_id_attack_target >= 0) {
        // Fix: bound check before indexing Bases
        if (plr->base_id_attack_target >= *BaseCount
        || !at_war(Bases[plr->base_id_attack_target].faction_id, faction_id)) {
            plr->base_id_attack_target = -1;
        }
    }
}

int strcmpi(const char* a, const char* b) {
    while (*a && *b) {
        int ca = tolower((unsigned char)*a++);
        int cb = tolower((unsigned char)*b++);
        if (ca != cb) {
            return ca - cb;
        }
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

/* Original: POP2 (exe) shows a SCRIPT popup with an image, centred on a base. */
int pop2(const char* label, const char* pcx_filename, int UNUSED(base_id)) {
    return popp(ScriptFile, label, 0, pcx_filename, nullptr);
}

} // namespace wp1

/* ---- social engineering ---- */

/*
Handle the social engineering turn upkeep for the specified faction.
*/
void __cdecl social_upkeep(int faction_id) {
    Faction* f = &Factions[faction_id];
    // Fix: the original casts &SE_Politics_pending to CSocialCategory*; the fields are
    // copied explicitly so the code does not depend on the native struct layout.
    CSocialCategory pending;
    pending.politics = f->SE_Politics_pending;
    pending.economics = f->SE_Economics_pending;
    pending.values = f->SE_Values_pending;
    pending.future = f->SE_Future_pending;
    f->SE_Politics = pending.politics;
    f->SE_Economics = pending.economics;
    f->SE_Values = pending.values;
    f->SE_Future = pending.future;
    social_calc(&pending, (CSocialEffect*)&f->SE_economy_pending, faction_id, false, false);
    social_calc(&pending, (CSocialEffect*)&f->SE_economy, faction_id, false, false);
    social_calc(&pending, (CSocialEffect*)&f->SE_economy_2, faction_id, true, false);
    f->SE_upheaval_cost_paid = 0;
}

/*
Calculate the cost of the social upheaval for the specified faction.
Return Value: Social upheaval cost
*/
int __cdecl social_upheaval(int faction_id, CSocialCategory* choices) {
    Faction* f = &Factions[faction_id];
    const int current[MaxSocialCatNum] = {f->SE_Politics, f->SE_Economics, f->SE_Values, f->SE_Future};
    int changes = 0;
    for (int i = 0; i < MaxSocialCatNum; i++) {
        if (choices->models[i] != current[i]) {
            changes++;
        }
    }
    if (!changes) {
        return 0;
    }
    changes++;
    int diff_lvl = is_human(faction_id) ? f->diff_level : DIFF_LIBRARIAN;
    int cost = changes * changes * changes * diff_lvl;
    if (is_alien(faction_id)) {
        cost += cost / 2;
    }
    return cost;
}

/* ---- research ---- */

static bool revised_tech_cost() {
    // Not supported during multiplayer
    return conf.revised_tech_cost && !*MultiplayerActive;
}

void __cdecl tech_research(int faction_id, int value) {
    if (faction_id < 0 || faction_id >= MaxPlayerNum || value < 0) {
        return; // Fix: the original asserts
    }
    CFacility& fac_psigate = Facility[FAC_PSI_GATE];
    CFacility& fac_ascent = Facility[FAC_ASCENT_TO_TRANSCENDENCE];
    Faction& plr = Factions[faction_id];
    if (value > 0) {
        if (!is_human(faction_id) && wp1::climactic_battle()) {
            // Original also applied this hidden AI bonus for faction_id <= 0.
            // The reason for this is unclear and it may not be useful so it is removed.
            // PSI Gate checks are related to conditions on climactic_battle.
            if ((fac_psigate.preq_tech != TECH_None
            && (fac_psigate.preq_tech < 0 || fac_psigate.preq_tech >= TECH_TranT
            || (Tech[fac_psigate.preq_tech].preq_tech1 < TECH_None)
            || (Tech[fac_psigate.preq_tech].preq_tech2 < TECH_None
            && Tech[fac_psigate.preq_tech].preq_tech1 != TECH_None)
            || !(TechOwners[fac_psigate.preq_tech] & (1 << faction_id))))
            || (fac_ascent.preq_tech != TECH_None
            && (fac_ascent.preq_tech < 0 || fac_ascent.preq_tech >= TECH_TranT
            || (Tech[fac_ascent.preq_tech].preq_tech1 < TECH_None)
            || (Tech[fac_ascent.preq_tech].preq_tech2 < TECH_None
            && Tech[fac_ascent.preq_tech].preq_tech1 != TECH_None)
            || !(TechOwners[fac_ascent.preq_tech] & (1 << faction_id))))) {
                value *= 2;
            }
        }
        plr.tech_accumulated += value;
        if (*MultiplayerActive || plr.tech_research_id >= 0) {
            if (plr.tech_research_id >= 89) { // Possibly unused game mechanic
                plr.tech_accumulated += value;
            }
            if (plr.tech_accumulated >= mod_tech_rate(faction_id)) {
                int tech_id = tech_advance(faction_id);
                debug("tech_advance %d %d %d\n", *CurrentTurn, faction_id, tech_id);
                if (tech_id >= 0 && turn_hooks().mon_tech_discovered) {
                    turn_hooks().mon_tech_discovered(faction_id, tech_id);
                }
                plr.tech_accumulated = 0;
                if (!revised_tech_cost()) {
                    plr.tech_cost = -1; // Must be negative to reset the cost
                    plr.tech_cost = mod_tech_rate(faction_id);
                }
            }
            if (plr.player_flags & PFLAG_FIRST_SECRETS) {
                plr.player_flags &= ~PFLAG_FIRST_SECRETS;
                int tech_id = tech_advance(faction_id);
                debug("tech_secrets %d %d %d\n", *CurrentTurn, faction_id, tech_id);
                if (tech_id >= 0 && turn_hooks().mon_tech_discovered) {
                    turn_hooks().mon_tech_discovered(faction_id, tech_id);
                }
            }
        }
        if (plr.tech_accumulated >= 0 && plr.tech_research_id < 0
        && (!is_human(faction_id) || faction_id == *CurrentPlayerFaction || !*PbemActive)) {
            plr.tech_research_id = mod_tech_selection(faction_id);
        }
    }
}

/* ---- rankings ---- */

void __cdecl rankings(int flag) {
    int max_weapon_value = 1;
    for (int unit_id = MaxProtoFactionNum; unit_id < MaxProtoNum; unit_id++) {
        if (Units[unit_id].is_active() && Units[unit_id].is_prototyped()) {
            int offense_value = Units[unit_id].offense_value();
            if (offense_value < 99) {
                max_weapon_value = max(max_weapon_value, offense_value);
            }
        }
    }
    for (int fc_id = 0; fc_id < MaxPlayerNum; fc_id++) {
        Faction* plr = &Factions[fc_id];
        if (flag) {
            plr->ranking = 0;
        }
        if (fc_id == 0) {
            continue;
        }
        int score;
        if (!is_alive(fc_id)) {
            score = 0;
        } else if ((*ObjectiveReqVictory < 9000 || *ObjectivesSuddenDeathVictory < 9000)
        && !*ObjectiveAchievePts) {
            score = 10 * wp1::num_objectives(fc_id, 0);
        } else {
            score = 4 * (plr->pop_total + plr->tech_count_transcendent);
            for (int tech_id = 0; tech_id < MaxTechnologyNum; tech_id++) {
                if (has_tech(tech_id, fc_id)) {
                    score += Tech[tech_id].AI_growth + Tech[tech_id].AI_tech
                        + Tech[tech_id].AI_wealth + Tech[tech_id].AI_power;
                }
            }
            for (int i = 0; i < MaxSecretProjectNum; i++) {
                if (SecretProjects[i] >= 0 && Bases[SecretProjects[i]].faction_id == fc_id) {
                    score += 10;
                }
            }
            score += *ObjectiveAchievePts * wp1::num_objectives(fc_id, 0);
            // Fix: original version considered plr->units_active[unit_id] when the count
            // is 250 or below and otherwise excludes unit_id from scoring but this does not
            // properly handle plr->units_active wrapping to zero when it reaches 256.
            std::map<int,int> units_active;
            for (int i = *VehCount - 1; i >= 0; --i) {
                if (Vehs[i].faction_id == fc_id) {
                    ++units_active[Vehs[i].unit_id];
                }
            }
            for (int unit_id = 0; unit_id < MaxProtoNum; unit_id++) {
                if (unit_id < MaxProtoFactionNum && !has_tech(Units[unit_id].preq_tech, fc_id)) {
                    continue;
                }
                int unit_value = units_active[unit_id] * Units[unit_id].cost;
                if (!unit_value) {
                    continue;
                }
                // Fix: original version used score += unit_value * (offense_value / max_weapon_value);
                // for non-PB, non-PSI combat units but this is likely a mistake since integer division
                // truncates the value towards zero when offense_value is below current max_weapon_value.
                int offense_value = Units[unit_id].offense_value();
                if (offense_value == 0) {
                    score += unit_value / 4;
                } else if (offense_value < 0) {
                    score += unit_value / 2;
                } else if (offense_value < 99) {
                    score += unit_value * offense_value / max_weapon_value;
                } else {
                    score += unit_value;
                }
            }
        }
        FactionRankingsUnk[fc_id] = score;
        if (*CurrentTurn >= 0 && *CurrentTurn < 1000) { // Fix: lower bound
            FactionTurnMight[*CurrentTurn][fc_id] = (int16_t)score;
        }
    }
    if (!flag) {
        return;
    }
    for (int rnk = 7; rnk > 0; --rnk) {
        int best_score = -1;
        int best_id = 0;
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (FactionRankingsUnk[i] > best_score) {
                best_score = FactionRankingsUnk[i];
                best_id = i;
            }
        }
        FactionRankingsUnk[best_id] = -1;
        Factions[best_id].ranking = rnk;
    }
    for (int i = 0; i < MaxPlayerNum; i++) {
        FactionRankings[i] = 0;
    }
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (is_alive(i)) {
            FactionRankings[Factions[i].ranking] = i;
        }
    }
    *RankingFactionIDUnk1 = 0;
    *RankingFactionIDUnk2 = 0;
    for (int rnk = 7; rnk >= 0; --rnk) {
        if (is_human(FactionRankings[rnk])) {
            *RankingFactionIDUnk1 = FactionRankings[rnk];
            break;
        }
    }
    for (int rnk = 0; rnk < MaxPlayerNum; ++rnk) {
        if (is_human(FactionRankings[rnk])) {
            *RankingFactionIDUnk2 = FactionRankings[rnk];
            break;
        }
    }
    int leader_faction = wp1::great_satan(FactionRankings[7], 0) ? FactionRankings[7] : 0;
    if (wp1::climactic_battle()) {
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (is_human(i) && (Factions[i].diff_level >= 4 || (*GameRules & RULES_INTENSE_RIVALRY))) {
                leader_faction = i;
            }
        }
    }
    if (!leader_faction) {
        *GameState &= ~STATE_UNK_200;
        return;
    }
    *GameState |= STATE_UNK_200;
    int vendetta_count = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (Factions[i].diplo_status[leader_faction] & DIPLO_VENDETTA) {
            vendetta_count++;
        }
    }
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (is_human(i)) {
            continue;
        }
        int status = Factions[i].diplo_status[leader_faction];
        if (!(status & (DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT))) {
            continue;
        }
        if (status & (DIPLO_UNK_800|DIPLO_SHALL_BETRAY|DIPLO_PACT|DIPLO_UNK_4000000)) {
            continue;
        }
        int difficulty = (*GameRules & RULES_INTENSE_RIVALRY) ? 5 : Factions[leader_faction].diff_level;
        bool wants_revenge = (Factions[leader_faction].diplo_status[i] & DIPLO_WANT_REVENGE) != 0;
        int chance = difficulty * ((wants_revenge ? 4 : 0)
            + ((Factions[i].player_flags_ext & PFLAG_EXT_SHAMELESS_BETRAY_HUMANS) ? 4 : 0)
            + (wp1::climactic_battle() ? 4 : 0)
            + wp1::reputation(leader_faction, i));
        int roll_range = (vendetta_count - Factions[i].AI_fight + 2) << 8;
        int roll = roll_range > 1 ? game_rand() % roll_range : 0;
        if (roll < chance) {
            set_treaty(i, leader_faction, DIPLO_SHALL_BETRAY, 1);
        }
    }
}

/* ---- time warp and autosave ---- */

static void set_project_owner(FacilityId item_id, int faction_id) {
    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].faction_id == faction_id) {
            SecretProjects[item_id - SP_ID_First] = i;
            break;
        }
    }
}

void __cdecl mod_time_warp() {
    const uint32_t BIT_SKIP_BONUS = (BIT_BASE_IN_TILE | BIT_VEH_IN_TILE
        | BIT_THERMAL_BORE | BIT_MONOLITH | BIT_MINE | BIT_SOLAR);
    const FacilityId projects[] = {
        FAC_CITIZENS_DEFENSE_FORCE,
        FAC_COMMAND_NEXUS,
        FAC_HUMAN_GENOME_PROJECT,
        FAC_MERCHANT_EXCHANGE,
        FAC_PLANETARY_TRANSIT_SYSTEM,
        FAC_VIRTUAL_WORLD,
        FAC_WEATHER_PARADIGM,
    };
    std::set<int32_t> selected = {};
    std::set<int32_t> choices = {};
    const int num = clamp(*MapAreaTiles / 3200 + 2, 2, 5);
    /*
    Aquatic factions always spawn with an extra Unity Gunship.
    Alien factions spawn with an extra Colony Pod and Battle Ogre Mk1.
    These units are added in setup_player regardless of other conditions.
    */
    if (conf.time_warp_mod) {
        for (int i = 1; i < MaxPlayerNum; i++) {
            bool ocean = MFactions[i].is_aquatic();
            bool alien = MFactions[i].is_alien();
            for (int j = 0; j < conf.time_warp_techs; j++) {
                Factions[i].tech_research_id = mod_tech_ai(i);
                tech_advance(i);
            }
            Factions[i].tech_ranking = 2 * Factions[i].tech_achieved;
            Factions[i].tech_research_id = -1;
            Factions[i].energy_credits += num * 50;
            if (turn_hooks().consider_designs) {
                turn_hooks().consider_designs(i);
            }

            int base_id = find_hq(i);
            if (base_id >= 0) {
                BASE* base = &Bases[base_id];
                base->pop_size = 4;
                if (!has_fac_built(FAC_PRESSURE_DOME, base_id)) {
                    set_fac(FAC_RECYCLING_TANKS, base_id, 1);
                }
                set_fac(FAC_RECREATION_COMMONS, base_id, 1);
                set_fac(FAC_PERIMETER_DEFENSE, base_id, 1);
                set_fac(FAC_NETWORK_NODE, base_id, 1);

                for (int j = 0; j < num; j++) {
                    if (ocean) {
                        veh_init_free(j&1 ? BSC_UNITY_GUNSHIP : BSC_TRANSPORT_FOIL, i, base->x, base->y);
                    } else {
                        veh_init_free(j&1 ? BSC_UNITY_ROVER : BSC_SCOUT_PATROL, i, base->x, base->y);
                    }
                }
                for (int j = 0; j < num - ocean - alien; j++) {
                    veh_init_free(ocean ? BSC_SEA_ESCAPE_POD : BSC_COLONY_POD, i, base->x, base->y);
                }
                for (int j = 0; j < num; j++) {
                    veh_init_free(ocean ? BSC_SEA_FORMERS : BSC_FORMERS, i, base->x, base->y);
                }
                int fixed = 0;
                int added = 0;
                for (auto& m : iterate_tiles(base->x, base->y, 0, 81)) {
                    if ((base->x != m.x || base->y != m.y)
                    && (m.sq->owner < 0 || m.sq->owner == i)
                    && !(m.sq->landmarks & LM_VOLCANO)
                    && !(m.sq->items & BIT_SKIP_BONUS)) {
                        // Improve resources/remove fungus near start
                        int bonus = mod_bonus_at(m.x, m.y);
                        if (!mod_goody_at(m.x, m.y) && !m.sq->is_rocky() && added < 5 && !random(4)) {
                            if (!is_ocean(m.sq)) {
                                m.sq->items &= ~TerraformRules[FORMER_FOREST][1];
                                m.sq->items &= ~BIT_FUNGUS;
                                m.sq->items |= BIT_FOREST;
                                added++;
                            } else if (is_ocean_shelf(m.sq)) {
                                m.sq->items &= ~TerraformRules[FORMER_FARM][1];
                                m.sq->items &= ~BIT_FUNGUS;
                                m.sq->items |= BIT_FARM;
                                added++;
                            }
                        }
                        if (m.sq->items & BIT_FUNGUS && bonus != RES_NONE && fixed < 4) {
                            m.sq->items &= ~BIT_FUNGUS;
                            fixed++;
                        }
                        if (bonus == RES_NUTRIENT && m.sq->is_rocky()) {
                            rocky_set(m.x, m.y, LEVEL_ROLLING);
                        }
                    }
                    m.sq->visibility |= (1 << i);
                    synch_bit(m.x, m.y, i);
                }
                // Set initial production, turn advances once before player turn begins
                if (!base->nerve_staple_turns_left) {
                    ++base->nerve_staple_turns_left;
                }
                set_base(base_id);
                base_compute(1);
                mod_base_change(base_id, select_build(base_id));
            }
        }
        *CurrentTurn = conf.time_warp_start_turn;
    } else {
        // Original: time_warp() in terranx.exe (no upstream source).
        not_implemented("time_warp");
    }
    init_world_config();
    if (!conf.time_warp_projects) {
        return;
    }
    for (const FacilityId item_id : projects) {
        choices.clear();
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (selected.count(i)
            || (item_id == FAC_COMMAND_NEXUS && has_free_facility(FAC_COMMAND_CENTER, i))
            || (item_id == FAC_CITIZENS_DEFENSE_FORCE && has_free_facility(FAC_PERIMETER_DEFENSE, i))) {
                // skip faction
            } else {
                choices.insert(i);
            }
        }
        if (!choices.size()) {
            for (int i = 1; i < MaxPlayerNum; i++) {
                if (!selected.count(i)) {
                    choices.insert(i);
                }
            }
        }
        if (choices.size()) {
            int32_t choice = pick_random(choices);
            set_project_owner(item_id, choice);
            selected.insert(choice);
        }
    }
}

void __cdecl mod_auto_save() {
    if ((!*PbemActive || *MultiplayerActive)
    && (!(*GameRules & RULES_IRONMAN) || *GameState & STATE_SCENARIO_EDITOR)) {
        if (conf.autosave_interval > 0 && !(*CurrentTurn % conf.autosave_interval)) {
            char buf[256];
            // Original: "saves/auto/Autosave_%d.sav"; the Amiga build has no saves/ tree.
            snprintf(buf, sizeof(buf), "Autosave_%d.sav", game_year(*CurrentTurn));
            save_daemon(buf);
        }
    }
}

} // namespace engine
} // namespace alpha
