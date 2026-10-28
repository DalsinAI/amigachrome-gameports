/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP8: victory, score and faction elimination.
 * is_objective, num_objectives, most_objectives, get_rating and compute_score
 * are Thinker Mod src/score.cpp; generators and end_of_game src/game.cpp;
 * mod_eliminate_player src/faction.cpp; corner_market, vulnerable and
 * mod_mind_control src/probe.cpp, at dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * transcending (exe 0x5AC630) follows the OpenSMACX reconstruction
 * (GPL-3.0-or-later). Deviations are marked "Fix:" or "Original:".
 */
#include "engine.h"
#include "diplo.h"
#include "turn.h"

namespace alpha {
namespace engine {

/* Owned by other packages, not in engine_wp.h; weak fallbacks in wp8_deps.cpp. */
extern int* BaseCommerceImport;                                    // WP2 base_compute output [8]
void __cdecl mon_winning_unify(int faction_id, int winner_id);     // WP2 monument.cpp
void __cdecl mon_killed_faction(int faction_id, int killed_id);    // WP2 monument.cpp

/* UI requests of the original end game (movies, credits, score and replay
 * screens). They reach the event sink as EV_MESSAGE "ui:<what>[:<arg>]". */
static void wp8_ui(const char* what, const char* arg = nullptr) {
    char buf[128];
    snprintf(buf, sizeof(buf), "ui:%s%s%s", what, arg ? ":" : "", arg ? arg : "");
    engine_notify(EV_MESSAGE, 0, 0, 0, 0, -1, -1, buf);
}

static void wp8_interlude(int event_id, const char* text, int a3, int a4) {
    if (turn_hooks().interlude) {
        turn_hooks().interlude(event_id, text, a3, a4);
    }
}

static void say_year(char* buf) {
    size_t len = strnlen(buf, StrBufLen);
    snprintf(buf + len, StrBufLen - len, "%d", *CurrentTurn + *StartingMissionYear);
}

/* ---- score.cpp ---- */

/*
Calculate score percentage scaling for the end game screen.
*/
int __cdecl get_rating(int faction_id, int score) {
    int diff_level = Factions[faction_id].diff_level;
    int value = diff_level;
    for (int level = 3; level <= diff_level; level++) {
        value += (level - 1) / 2;
    }
    if (*GameRules & RULES_INTENSE_RIVALRY) {
        value++;
    }
    return score * (value + 6) / 160;
}

/*
Determine whether the base is an objective for scenario victory conditions.
*/
int __cdecl is_objective(int base_id) {
    if (base_id < 0 || base_id >= *BaseCount) {
        assert(0);
        return false;
    }
    if (*GameRules & RULES_SCN_VICT_ALL_BASE_COUNT_OBJ
    || Bases[base_id].event_flags & BEVENT_OBJECTIVE) {
        return true;
    }
    if (*GameRules & RULES_SCN_VICT_SP_COUNT_OBJ) {
        for (int i = 0; i < MaxSecretProjectNum; i++) {
            if (SecretProjects[i] == base_id) {
                return true;
            }
        }
    }
    if (*GameState & STATE_SCN_VICT_BASE_FACIL_COUNT_OBJ
    && *ScnVictFacilityObj >= 0 && *ScnVictFacilityObj <= 64
    && has_fac_built((FacilityId)(*ScnVictFacilityObj), base_id)) {
        return true;
    }
    return false;
}

int __cdecl num_objectives(int faction_id, int incl_pact) {
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
            // Fix: lower bound on the facility id
            if (Bases[i].faction_id == faction_id
            && *ScnVictFacilityObj >= 0 && *ScnVictFacilityObj <= Fac_ID_Last
            && has_fac_built((FacilityId)*ScnVictFacilityObj, i)) {
                score++;
            }
        }
    }
    if (*GameState & STATE_SCN_VICT_TERRAIN_ENH_COUNT_OBJ) {
        for (int y = 0; y < *MapAreaY; y++) {
            for (int x = y & 1; x < *MapAreaX; x += 2) {
                MAP* sq = mapsq(x, y);
                if (sq->owner == faction_id) {
                    score += bit_count(sq->items & (BIT_SENSOR|BIT_THERMAL_BORE|BIT_ECH_MIRROR|BIT_CONDENSER|
                        BIT_FOREST|BIT_AIRBASE|BIT_FARM|BIT_BUNKER|BIT_SOLAR|BIT_MINE|BIT_MAGTUBE|BIT_ROAD));
                }
            }
        }
    }
    if (*GameState & STATE_SCN_VICT_TERRITORY_COUNT_OBJ) {
        for (int y = 0; y < *MapAreaY; y++) {
            for (int x = y & 1; x < *MapAreaX; x += 2) {
                if (mapsq(x, y)->owner == faction_id) {
                    score++;
                }
            }
        }
    }
    if (*GameRules & RULES_SCN_VICT_SP_COUNT_OBJ) {
        for (int i = 0; i < MaxSecretProjectNum; i++) {
            // Fix: upper bound on the base id
            if (SecretProjects[i] >= 0 && SecretProjects[i] < *BaseCount
            && Bases[SecretProjects[i]].faction_id == faction_id) {
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

int __cdecl most_objectives(int* winner_id, int* tied_rivals) {
    int best_score = 0;
    int best_fc_id = 0;
    if (tied_rivals) {
        *tied_rivals = 0;
    }
    for (int fc_id = 1; fc_id < MaxPlayerNum; fc_id++) {
        int score = num_objectives(fc_id, *GameRules & RULES_VICTORY_COOPERATIVE);
        if (score >= best_score) {
            if (tied_rivals) {
                if (score > best_score) {
                    *tied_rivals = 0;
                } else {
                    if (!(Factions[fc_id].diplo_status[best_fc_id] & DIPLO_PACT)
                    || !(*GameRules & RULES_VICTORY_COOPERATIVE)) {
                        ++(*tied_rivals);
                    }
                }
            }
            best_score = score;
            best_fc_id = fc_id;
        }
    }
    if (winner_id) {
        *winner_id = best_fc_id;
    }
    return best_score;
}

/* The 9-entry score table: the caller's array, or (table_val == null) the faction's
 * own fields unk_110 .. unk_118. Fix: the original pointed into the struct
 * (&plr->unk_110) and indexed past the field; this keeps the same aliasing (the
 * unk_110.. resets below also clear the table) without out-of-object indexing. */
namespace {
struct ScoreTable {
    int* arr;
    Faction* plr;
    int& operator[](int i) {
        if (arr) {
            return arr[i];
        }
        switch (i) {
        case 0: return plr->unk_110;
        case 1: return plr->unk_111;
        case 2: return plr->unk_112;
        case 3: return plr->unk_113;
        case 4: return plr->unk_114;
        case 5: return plr->unk_115;
        case 6: return plr->unk_116;
        case 7: return plr->unk_117;
        default: return plr->unk_118;
        }
    }
};
} // namespace

void __cdecl compute_score(int faction_id, int* score_val, int* table_arg, int flag) {
    if (faction_id <= 0 || faction_id >= MaxPlayerNum) {
        return; // Fix: bounds check
    }
    Faction* const plr = &Factions[faction_id];

    if (*GameState & STATE_FINAL_SCORE_DONE && !flag) {
        return;
    }
    if (!score_val) {
        score_val = &plr->unk_109;
    }
    // Fix: end_of_game passed a single int as the table and 9 entries were written
    // through it; callers here pass a full table or null.
    ScoreTable table_val = {table_arg, plr};
    *score_val = 0;
    if (!flag) {
        plr->unk_107 = 0;
    }
    for (int i = 0; i < 9; i++) { // Original: memset(table_val, 0, 0x24)
        table_val[i] = 0;
    }
    for (int base_id = 0; base_id < *BaseCount; base_id++) {
        BASE* base = &Bases[base_id];
        if (base->faction_id == faction_id) {
            table_val[0] += base->pop_size;
            set_base(base_id);
            base_compute(0);
            for (int i = 1; i < MaxPlayerNum; i++) {
                if (!MFactions[faction_id].is_alien() && !MFactions[i].is_alien()
                && !plr->sanction_turns && !Factions[i].sanction_turns) {
                    table_val[1] += BaseCommerceImport[i];
                }
            }
        } else if (plr->player_flags & (PFLAG_UNK_20000|PFLAG_UNK_40000)) {
            if (plr->diplo_status[base->faction_id] & DIPLO_PACT
            && *GameRules & RULES_VICTORY_COOPERATIVE) {
                table_val[0] += base->pop_size;
            } else {
                table_val[0] += base->pop_size / 2;
            }
        } else {
            int status = Factions[base->faction_id].diplo_status[faction_id];
            if (status & DIPLO_HAVE_SURRENDERED) {
                if (status & DIPLO_PACT) {
                    table_val[0] += base->pop_size;
                }
            } else if (status & DIPLO_PACT && *GameRules & RULES_VICTORY_COOPERATIVE) {
                table_val[0] += base->pop_size / 2;
            }
        }
    }
    for (int i = 0; i < MaxTechnologyNum; i++) {
        if (has_tech(i, faction_id)) {
            ++table_val[2];
        }
    }
    table_val[3] = 10 * plr->tech_count_transcendent;
    table_val[6] = max(1, *ObjectiveAchievePts)
        * num_objectives(faction_id, *GameRules & RULES_VICTORY_COOPERATIVE);
    for (int i = 0; i < MaxSecretProjectNum; i++) {
        if (SecretProjects[i] >= 0 && SecretProjects[i] < *BaseCount // Fix: upper bound
        && Bases[SecretProjects[i]].faction_id == faction_id) {
            table_val[4] += 25;
        }
    }
    *ScoreBonusPts = 0;
    for (int i = 0; i < MaxPlayerNum; i++) { // Original: memset(ScorePopTotal, 0, 0x20)
        ScorePopTotal[i] = 0;
    }
    int asc_base_id = project_base(FAC_ASCENT_TO_TRANSCENDENCE);
    if (asc_base_id >= 0 && asc_base_id < *BaseCount) { // Fix: upper bound
        *ScoreBonusPts = max(300, 2 * (1000 - *CurrentTurn));
        int fc_id = -1;
        int num_plr = 0;
        int num_all = 0;
        if (Bases[asc_base_id].faction_id > 0 && Bases[asc_base_id].faction_id < 8) {
            fc_id = Bases[asc_base_id].faction_id;
            num_all = 2 * Factions[fc_id].pop_total;
            ScorePopTotal[fc_id] = num_all;
            if (fc_id == faction_id) {
                num_plr = num_all;
            }
        }
        // Fix: added separate checks for cases where fc_id is not defined
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (fc_id >= 0 && fc_id != i && Factions[i].diplo_status[fc_id] & DIPLO_PACT) {
                num_all += Factions[i].pop_total;
                ScorePopTotal[i] = Factions[i].pop_total;
                if (i == faction_id) {
                    num_plr += Factions[i].pop_total;
                }
            }
        }
        table_val[6] = (!num_all ? 0 : num_plr * *ScoreBonusPts / num_all);
        ScorePopTotal[0] = num_all;
    }
    table_val[7] = max(0, 2 * (500 - *CurrentTurn));
    if (!(*GameRules & RULES_VICTORY_CONQUEST)) {
        table_val[7] = 0;
    }
    if (*GameState & STATE_VICTORY_CONQUER && *GameRules & RULES_VICTORY_CONQUEST && table_val[7]) {
        int pop_plr = 0;
        int pop_all = 0;
        for (int i = 0; i < *BaseCount; i++) {
            pop_all += Bases[i].pop_size;
            int status = Factions[Bases[i].faction_id].diplo_status[faction_id];
            if (Bases[i].faction_id == faction_id
            || (status & DIPLO_PACT && status & DIPLO_HAVE_SURRENDERED)) {
                pop_plr += Bases[i].pop_size;
            }
        }
        if (pop_all) {
            table_val[7] = pop_plr * plr->unk_117 / pop_all;
        }
    } else if (plr->player_flags & (PFLAG_UNK_20000|PFLAG_UNK_40000)) {
        table_val[7] = max(200, 2 * (600 - *CurrentTurn));
    } else if (plr->player_flags & PFLAG_UNK_100000) {
        table_val[7] = max(300, 2 * (1000 - *CurrentTurn));
    } else {
        table_val[7] = 0;
        for (int i = 1; i < MaxPlayerNum; i++) {
            if ((Factions[i].player_flags & (PFLAG_UNK_20000|PFLAG_UNK_40000))
            && (plr->diplo_status[i] & DIPLO_PACT)
            && (*GameRules & RULES_VICTORY_COOPERATIVE)) {
                table_val[7] = max(100, 600 - *CurrentTurn);
            }
        }
    }
    if (!is_alive(faction_id)) {
        table_val[7] = 0;
        table_val[8] = 0;
    }
    if ((*ObjectiveReqVictory < 9000 || *ObjectivesSuddenDeathVictory < 9000)
    && !(*GameState & STATE_SCN_VICT_HIGHEST_AC_SCORE_WINS)) {
        plr->unk_110 = 0;
        plr->unk_111 = 0;
        plr->unk_112 = 0;
        plr->unk_113 = 0;
        plr->unk_114 = 0;
        plr->unk_115 = 0;
        // skipped unk_116
        plr->unk_117 = 0;
        plr->unk_118 = 0;
    }
    if (!plr->unk_117) {
        int num_obj = num_objectives(faction_id, *GameRules & RULES_VICTORY_COOPERATIVE);
        if (*ObjectiveReqVictory) {
            if (num_obj >= *ObjectiveReqVictory) {
                table_val[7] = *VictoryAchieveBonusPts;
            }
        } else {
            if (num_obj >= max(1, most_objectives(0, 0))) {
                table_val[7] = *VictoryAchieveBonusPts;
            }
        }
    }
    for (int i = 0; i < 9; i++) {
        *score_val += table_val[i];
    }
    if (*MapNativeLifeForms != 1 && !(*GameState & STATE_IS_SCENARIO)) {
        table_val[5] = *score_val * (*MapNativeLifeForms - 1) / 4;
        *score_val += table_val[5];
    }
    if (flag) {
        return;
    }
    if (*GameState & STATE_SCN_VICT_HIGHEST_AC_SCORE_WINS) {
        int score_alt = 0;
        int table_alt[9] = {};
        int score_max = 0;
        for (int i = 1; i < MaxPlayerNum; i++) {
            compute_score(i, &score_alt, &table_alt[0], 1);
            if (score_alt > score_max) {
                score_max = score_alt;
            }
        }
        if (*score_val >= score_max) {
            if (!*VictoryAchieveBonusPts) {
                table_val[7] = 1000;
            } else {
                table_val[7] = *VictoryAchieveBonusPts;
            }
        }
    }
    plr->unk_107 = *score_val;
    if (*GameRules & RULES_IRONMAN && !(*GameState & STATE_SCENARIO_CHEATED_FLAG)) {
        plr->unk_107 = 2 * (*score_val);
    }
}

/* ---- game.cpp ---- */

int __cdecl generators(int faction_id, int* pop_size_req) {
    if (pop_size_req) {
        *pop_size_req = 0;
    }
    if (!MFactions[faction_id].is_alien()) {
        return 0;
    }
    int completed = 0;
    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id == faction_id && base->has_fac_built(FAC_SUBSPACE_GENERATOR)) {
            if (base->pop_size >= Rules->base_size_subspace_gen) {
                ++completed;
            } else if (pop_size_req) {
                ++(*pop_size_req);
            }
        }
    }
    return completed;
}

int __cdecl end_of_game(int flag) {
    const int player_id = *CurrentPlayerFaction;
    if ((*GameState & STATE_GAME_DONE) && (*GameState & STATE_FINAL_SCORE_DONE)) {
        return 0;
    }
    int winner_id = -1;
    if (!(*GameState & STATE_GAME_DONE) && flag) {
        if (*CurrentMissionYear >= *EndingMissionYear) {
            *GameVictoryType = VIC_TIME_LIMIT;
            *GameState |= STATE_GAME_DONE;
        }
        for (int fc_id = 1; fc_id < MaxPlayerNum; fc_id++) {
            if (!is_alive(fc_id)) { continue; }
            // Alien Victory (Subspace Generators)
            if (MFactions[fc_id].is_alien()) {
                int active_generators = generators(fc_id, 0);
                // Fix: a zero requirement (base game rules, no generators) never wins
                if (Rules->subspace_gen_req > 0 && active_generators >= Rules->subspace_gen_req) {
                    Factions[fc_id].player_flags |= PFLAG_UNK_100000;
                    if (fc_id == player_id) {
                        *GameVictoryType = VIC_ALIEN_SOLO;
                    } else if ((Factions[fc_id].diplo_status[player_id] & DIPLO_PACT)
                    && (*GameRules & RULES_VICTORY_COOPERATIVE)) {
                        if (*GameVictoryType != VIC_ALIEN_SOLO) {
                            *GameVictoryType = VIC_ALIEN_COOP;
                        }
                    } else {
                        if (*GameVictoryType != VIC_ALIEN_SOLO && *GameVictoryType != VIC_ALIEN_COOP) {
                            *GameVictoryType = VIC_ALIEN_LOSS;
                        }
                    }
                    *GameState |= STATE_GAME_DONE;
                    if (winner_id < 0 || fc_id == player_id) {
                        winner_id = fc_id;
                    }
                    *GameMoreRules |= MRULES_UNK_80;
                }
            }
            // Economic Victory
            int market_turn = Factions[fc_id].corner_market_turn;
            if (market_turn > 0 && market_turn == *CurrentTurn && Factions[fc_id].corner_market_cost > 0) {
                Factions[fc_id].player_flags |= PFLAG_UNK_40000;
                if (fc_id == player_id) {
                    *GameVictoryType = VIC_ECONOMIC_SOLO;
                } else if ((Factions[fc_id].diplo_status[player_id] & DIPLO_PACT)
                && (*GameRules & RULES_VICTORY_COOPERATIVE)) {
                    if (*GameVictoryType != VIC_ECONOMIC_SOLO) {
                        *GameVictoryType = VIC_ECONOMIC_COOP;
                    }
                } else {
                    if (*GameVictoryType != VIC_ECONOMIC_SOLO && *GameVictoryType != VIC_ECONOMIC_COOP) {
                        *GameVictoryType = VIC_ECONOMIC_LOSS;
                    }
                }
                *GameState |= (STATE_VICTORY_ECONOMIC | STATE_GAME_DONE);
                winner_id = fc_id;
                if (fc_id == player_id) { break; } // skip other checks
            }
            // Sudden Death Scenario Objectives
            else if ((is_human(fc_id) || !(*GameRules & RULES_SCN_VICT_SOLO_MISSION))
            && *ObjectivesSuddenDeathVictory >= 1) {
                if (num_objectives(fc_id, *GameRules & RULES_VICTORY_COOPERATIVE)
                >= *ObjectivesSuddenDeathVictory) {
                    *GameVictoryType = VIC_SUDDEN_DEATH;
                    *GameState |= STATE_GAME_DONE;
                    if (winner_id < 0 || fc_id == player_id) {
                        winner_id = fc_id;
                    }
                }
            }
        }
    }
    if (flag && !(*GameState & STATE_GAME_DONE)) {
        if (!(*GameState & STATE_IS_SCENARIO) && *CurrentMissionYear == *EndingMissionYear - 20) {
            parse_num(0, *EndingMissionYear);
            snprintf(StrBuffer, StrBufLen, "%d", *EndingMissionYear);
            parse_says(0, StrBuffer, -1, -1);
            X_pop("RETIREWARNING", 0);
        } else if ((*GameState & STATE_IS_SCENARIO)) {
            if (*CurrentMissionYear == *EndingMissionYear - 20
            || *CurrentMissionYear == *EndingMissionYear - 10) {
                parse_num(0, *EndingMissionYear);
                snprintf(StrBuffer, StrBufLen, "%d", *EndingMissionYear);
                parse_says(0, StrBuffer, -1, -1);
                X_pop_2("SCENARIO", "SCENARIOWARNING", 0);
            }
        }
    }
    if (!(*GameState & STATE_GAME_DONE) || (*GameState & STATE_FINAL_SCORE_DONE)) {
        return 0;
    }
    int victor_id = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (Factions[i].player_flags & PFLAG_UNK_20000) {
            victor_id = i;
        }
        if ((Factions[i].player_flags & PFLAG_UNK_40000)
        && (!victor_id || victor_id == player_id
        || (Factions[victor_id].diplo_status[player_id] & DIPLO_PACT))) {
            victor_id = i;
        }
    }
    auto setup_parser = [](int id) {
        if (id < 0 || id >= MaxPlayerNum) {
            id = 0; // Fix: winner_id may still be -1
        }
        parse_says(0, get_title(id), -1, -1);
        parse_says(1, get_name(id), -1, -1);
        parse_says(2, get_noun(id), -1, -1);
    };
    bool play_credits = false;

    switch (*GameVictoryType) {
    case VIC_TRANSCEND_PLR:
    case VIC_TRANSCEND_UNK:
    case VIC_TRANSCEND_LOSS:
        play_credits = true;
        break;

    case VIC_DIPLOMATIC_SOLO:
        popp(ScriptFile, "DIPLOMATICVICTORY", 0, "dipvic_sm.pcx", 0);
        break;

    case VIC_LOST_CAPTURE:
        if (*GamePreferences & PREF_AV_SECRET_PROJECT_MOVIES) {
            if (!MFactions[player_id].is_alien()) {
                wp8_ui("movie", MFactions[player_id].is_leader_female ? "losewoman" : "loseman");
            } else {
                wp8_ui("movie", "losealien");
            }
        }
        break;

    case VIC_TIME_LIMIT:
        parse_num(0, *CurrentMissionYear);
        StrBuffer[0] = 0;
        say_year(StrBuffer);
        parse_says(0, StrBuffer, -1, -1);
        if (*GameState & STATE_IS_SCENARIO) {
            if (!*ObjectiveReqVictory || (*GameState & STATE_SCN_VICT_HIGHEST_AC_SCORE_WINS)) {
                int best_fc_id = player_id;
                int best_score = -9999;
                for (int i = 1; i < MaxPlayerNum; i++) {
                    if (is_alive(i)) {
                        int current_score;
                        if (*GameState & STATE_SCN_VICT_HIGHEST_AC_SCORE_WINS) {
                            // Fix: pass a full 9-entry table (the original passed one int)
                            int score_val = 0, table_val[9] = {};
                            compute_score(i, &score_val, table_val, 1);
                            current_score = score_val;
                        } else {
                            current_score = num_objectives(i, *GameRules & RULES_VICTORY_COOPERATIVE);
                        }
                        if (current_score > best_score || (current_score == best_score && i == player_id)) {
                            best_fc_id = i;
                            best_score = current_score;
                        }
                    }
                }
                setup_parser(best_fc_id);
                if (best_fc_id == player_id) {
                    X_pop_2("SCENARIO", "SCENTIMEWIN", 0);
                } else {
                    X_pop_2("SCENARIO", "SCENTIMELOSS", 0);
                }
            } else {
                X_pop_2("SCENARIO", "SCENTIMELIMIT", 0);
            }
        } else {
            X_pop("TIMELIMIT", 0);
        }
        break;

    case VIC_SUDDEN_DEATH:
        setup_parser(winner_id);
        if (winner_id == player_id) {
            X_pop_2("SCENARIO", "SUDDENDEATH0", 0);
        } else {
            X_pop_2("SCENARIO", "SUDDENDEATH", 0);
        }
        break;

    case VIC_DIPLOMATIC_COOP:
        setup_parser(victor_id);
        popp(ScriptFile, "DIPLOMATICCOOP", 0, "dipvic_sm.pcx", 0);
        break;

    case VIC_DIPLOMATIC_LOSS:
        setup_parser(victor_id);
        {
            const char* popup_str = "DIPLOMATICLOSE2";
            if (!(Factions[victor_id].diplo_status[player_id] & (DIPLO_WANT_REVENGE | DIPLO_ATROCITY_VICTIM))) {
                popup_str = "DIPLOMATICLOSE";
            }
            popp(ScriptFile, popup_str, 0, "dipvic_sm.pcx", 0);
        }
        break;

    case VIC_ECONOMIC_SOLO:
        popp(ScriptFile, "ECONOMICVICTORY", 0, "econwin_sm.pcx", 0);
        break;

    case VIC_ECONOMIC_COOP:
        setup_parser(victor_id);
        popp(ScriptFile, "ECONOMICCOOP", 0, "econwin_sm.pcx", 0);
        break;

    case VIC_ECONOMIC_LOSS:
        setup_parser(victor_id);
        {
            const char* popup_str = "ECONOMICLOSE2";
            if (!(Factions[victor_id].diplo_status[player_id] & (DIPLO_ATROCITY_VICTIM | DIPLO_WANT_REVENGE))) {
                popup_str = "ECONOMICLOSE";
            }
            popp(ScriptFile, popup_str, 0, "econwin_sm.pcx", 0);
        }
        break;

    case VIC_LOST_REMOVE:
        if (*GameLanguage) {
            X_pop("YOULOSE2", 0);
        }
        break;

    case VIC_ALIEN_SOLO:
        wp8_interlude(32, 0, 4, 0);
        setup_parser(victor_id);
        popp(ScriptFile, "ENDBEACON", 0, "beacon_sm.pcx", 0);

        if (*GamePreferences & PREF_AV_SECRET_PROJECT_MOVIES) {
            if (!_stricmp("CARETAKE", MFactions[player_id].search_key)) {
                wp8_ui("movie", "close_ct");
            } else {
                wp8_ui("movie", "close_us");
            }
        }
        break;

    case VIC_ALIEN_COOP:
        setup_parser(victor_id);
        popp(ScriptFile, "ENDBEACONCOOP", 0, "beacon_sm.pcx", 0);
        break;

    case VIC_ALIEN_LOSS:
        if (!MFactions[player_id].is_alien()) {
            wp8_interlude(29, get_noun(victor_id), 1, 0);
        }
        setup_parser(victor_id);
        popp(ScriptFile, "ENDBEACONLOSE", 0, "beaconlose_sm.pcx", 0);
        break;
    }

    switch (*GameVictoryType) {
    case VIC_UNIFY_SOLO:
    case VIC_UNIFY_COOP:
    case VIC_DIPLOMATIC_SOLO:
    case VIC_ECONOMIC_SOLO:
        mon_winning_unify(player_id, -1);
        break;
    case VIC_DIPLOMATIC_COOP:
    case VIC_ECONOMIC_COOP:
        mon_winning_unify(victor_id, -1);
        mon_winning_unify(player_id, victor_id);
        break;
    case VIC_DIPLOMATIC_LOSS:
    case VIC_ECONOMIC_LOSS:
        mon_winning_unify(victor_id, -1);
        break;
    }

    if (!(*GamePreferences & PREF_AV_INTERLUDES_DISABLED)) {
        switch (*GameVictoryType) {
        case VIC_UNIFY_SOLO:
        case VIC_DIPLOMATIC_SOLO:
        case VIC_ECONOMIC_SOLO:
            if (!MFactions[player_id].is_alien()) {
                wp8_interlude(19, 0, 4, 0);
            } else {
                wp8_interlude(33, 0, 4, 0);
            }
            play_credits = true;
            break;
        case VIC_UNIFY_COOP:
        case VIC_DIPLOMATIC_COOP:
        case VIC_ECONOMIC_COOP:
            wp8_interlude(20, 0, 4, 0);
            play_credits = true;
            break;
        }
    }
    if (play_credits
    || *GameVictoryType == VIC_TRANSCEND_PLR
    || *GameVictoryType == VIC_TRANSCEND_UNK
    || *GameVictoryType == VIC_TRANSCEND_LOSS) {
        wp8_ui("credits");
    }
    // Original: report_score(1) shows the final score screen. The engine computes
    // every faction's score here (unk_107/unk_109/unk_110..118) for that screen.
    for (int i = 1; i < MaxPlayerNum; i++) {
        compute_score(i, nullptr, nullptr, 0);
    }
    wp8_ui("score");
    if (!(*GameState & STATE_IS_SCENARIO)) {
        wp8_ui("hall_of_fame"); // Original: quayle(player_id); hall_of_fame(1)
    }
    wp8_ui("replay"); // Original: show_replay()
    *GameState |= STATE_FINAL_SCORE_DONE;
    bool should_exit = (*MultiplayerActive || *GameVictoryType == VIC_LOST_CAPTURE);
    if (!should_exit) {
        should_exit = !popp(ScriptFile, "GAMEOVERMAN", 0, "stars_sm.pcx", 0);
    }
    if (should_exit) {
        *ControlTurnB = (*MultiplayerActive == 0);
        // Original: net_game_close() and ControlTurnA = 1 leave control_turn; the
        // return value tells WP1's control_turn_run to stop with TURN_GAME_OVER.
        return 1;
    }
    return 0;
}

/* ---- faction.cpp ---- */

/* Original: veh_find(0, 0, faction_id, -1) (exe), the faction's unit nearest to (0, 0). */
static int veh_find_any(int faction_id) {
    int best_id = -1;
    int best_dist = INT_MAX;
    for (int i = 0; i < *VehCount; i++) {
        if (Vehs[i].faction_id == faction_id) {
            int dist = map_range(0, 0, Vehs[i].x, Vehs[i].y);
            if (dist < best_dist) {
                best_dist = dist;
                best_id = i;
            }
        }
    }
    return best_id;
}

int __cdecl mod_eliminate_player(int faction_id, int setup_id) {
    // Fix: reject invalid factions instead of indexing out of range
    if (faction_id <= 0 || faction_id >= MaxPlayerNum || setup_id >= MaxPlayerNum) {
        return 0;
    }
    Faction* plr = &Factions[faction_id];
    MFaction* m = &MFactions[faction_id];
    const bool is_player = faction_id == *CurrentPlayerFaction;
    const bool plr_alien = m->is_alien();
    for (int i = *BaseCount - 1; i >= 0; --i) {
        if (Bases[i].faction_id == faction_id) {
            return 0;
        }
    }
    for (int i = *VehCount - 1; i >= 0; --i) {
        if (Vehs[i].faction_id == faction_id) {
            veh_kill(i); // does not call kill() instead?
        }
    }
    plr->base_count = 0;
    int setup_val = mod_setup_player(faction_id, setup_id, 0);
    if (!setup_val && setup_id > 0 && !(*GameState & (STATE_FINAL_SCORE_DONE|STATE_GAME_DONE))) {
        Factions[setup_id].eliminated_count++;
    }
    if (setup_val || !is_player) {
        if (setup_id >= 0) {
            if (setup_id > 0) {
                set_treaty(faction_id, setup_id, DIPLO_WANT_TO_TALK|DIPLO_WANT_REVENGE|DIPLO_VENDETTA, 1);
            }
        } else {
            setup_id = 0;
        }
        draw_map(1);
        parse_says(0, get_noun(faction_id), -1, -1);
        parse_says(1, get_noun(setup_id), -1, -1);
        parse_says(2, get_title(faction_id), -1, -1);
        parse_says(3, get_name(faction_id), -1, -1);
        parse_says(4, get_his_her(faction_id, 0), -1, -1);
        char name[StrBufLen] = {};
        char file[StrBufLen] = {};
        snprintf(name, StrBufLen, "WIPEOUT%s%d",
            (setup_id && setup_id == *CurrentPlayerFaction) ? "YOU" : "",
            setup_id ? (setup_val != 0) : 2);
        if (setup_val) { // The faction has escaped in a colony pod
            snprintf(file, StrBufLen, "%s", plr_alien ? "al_pod_sm.pcx" : "escpod_sm.pcx");
        } else { // The faction has been eradicated
            snprintf(file, StrBufLen, "%s2.pcx", m->filename);
        }
        popp(ScriptFile, name, 0, file, 0);
        if (!setup_val) {
            mon_killed_faction(setup_id, faction_id);
        }
        if (!*MultiplayerActive && !setup_val && setup_id == *CurrentPlayerFaction
        && (*GamePreferences & PREF_AV_SECRET_PROJECT_MOVIES)) {
            wp8_ui("movie", m->filename); // Original: mod_amovie_project(m->filename)
        }
        if (!*MultiplayerActive && setup_val && setup_id == *CurrentPlayerFaction) {
            // TRACKED event, escape location may be revealed by random chance
            bool infiltrate = Factions[setup_id].diplo_status[faction_id] & DIPLO_HAVE_INFILTRATOR;
            int veh_id;
            if (!game_randv(infiltrate != 0 ? 2 : 4)
            && (veh_id = veh_find_any(faction_id)) >= 0) {
                int x = Vehs[veh_id].x;
                int y = Vehs[veh_id].y;
                for (auto& p : iterate_tiles(x, y, 0, 9)) {
                    spot_loc(p.x, p.y, setup_id);
                }
                bool found = false;
                veh_id = veh_top(veh_id);
                while (veh_id >= 0) {
                    Vehs[veh_id].visibility = 0;
                    if (!found && Vehs[veh_id].is_colony()) {
                        Vehs[veh_id].visibility = (1 << setup_id);
                        found = true;
                    }
                    veh_id = Vehs[veh_id].next_veh_id_stack;
                }
                draw_radius(x, y, 2, 0); // Original: draw_tiles(x, y, 2); MapWin_set_center
                parse_says(6, get_adjective(faction_id), -1, -1);
                if (!plr_alien) {
                    popp(ScriptFile, "TRACKED", 0, "escpod_sm.pcx", 0);
                } else {
                    popp(ScriptFile, "TRACKED", 0, "al_pod_sm.pcx", 0);
                }
            }
        }
        if (faction_id != *CurrentPlayerFaction && plr_alien
        && *CurrentPlayerFaction >= 0 && *CurrentPlayerFaction < MaxPlayerNum
        && MFactions[*CurrentPlayerFaction].is_alien()) {
            wp8_interlude(42, 0, 1, 0);
        }
    }
    return 1;
}

/* ---- exe 0x5AC630 (OpenSMACX base.cpp) ---- */

/*
Is the faction building the Ascent to Transcendence? The original game does not use
this code. Fix (OpenSMACX): the queue entry of a project is negative.
*/
int __cdecl transcending(int faction_id) {
    if (!ascending(faction_id)) {
        return false;
    }
    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].faction_id == faction_id
        && Bases[i].queue_items[0] == -FAC_ASCENT_TO_TRANSCENDENCE) {
            return true;
        }
    }
    return false;
}

/* ---- probe.cpp ---- */

/*
Calculate how vulnerable the coordinates are for the specified faction based on how far
away this tile is from the faction headquarters.
Return Value: Radial distance between coordinates and faction HQ or 12 if no HQ/bases
*/
int __cdecl vulnerable(int faction_id, int x, int y) {
    int dist = 12; // default value for no bases or no HQ
    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].faction_id == faction_id && has_fac_built(FAC_HEADQUARTERS, i)) {
            dist = vector_dist(x, y, Bases[i].x, Bases[i].y);
            break;
        }
    }
    return dist;
}

/*
Calculate the cost for the faction to be able to mind control the specified base. The 3rd
parameter determines if this cost is for cornering the market (true) or via probe (false).
Return Value: Mind control cost
*/
int __cdecl mod_mind_control(int base_id, int faction_id, int is_corner_market) {
    BASE* base = &Bases[base_id];
    int dist = vulnerable(base->faction_id, base->x, base->y);
    if (dist <= 0) {
        if (!is_corner_market) {
            return -1;
        }
        dist = 1;
    }
    if (has_fac_built(FAC_GENEJACK_FACTORY, base_id)) {
        dist *= 2;
    }
    if (has_fac_built(FAC_CHILDREN_CRECHE, base_id)) {
        dist /= 2;
    }
    if (has_fac_built(FAC_PUNISHMENT_SPHERE, base_id)) {
        dist /= 2;
    }
    if (base->nerve_staple_turns_left) {
        dist /= 2;
    }
    int veh_id = stack_fix(veh_at(base->x, base->y));
    int cost = ((stack_check(veh_id, 2, PLAN_COMBAT, -1, -1)
        + stack_check(veh_id, 2, PLAN_OFFENSE, -1, -1))
        * (stack_check(veh_id, 6, ABL_POLY_ENCRYPTION, -1, -1) + 1)
        + Factions[faction_id].mind_control_total / 4 + base->pop_size)
        * ((Factions[base->faction_id].corner_market_cost
        + Factions[base->faction_id].energy_credits + 1200) / (dist + 4));
    if (!is_human(faction_id) && is_human(base->faction_id)) {
        int diff = Factions[base->faction_id].diff_level;
        if (diff > DIFF_LIBRARIAN) {
            cost = (cost * 3) / diff;
        }
    }
    bool is_pact = has_treaty(faction_id, base->faction_id, DIPLO_PACT);
    if (is_corner_market) {
        if (is_pact) {
            cost /= 2;
        }
        if (has_treaty(faction_id, base->faction_id, DIPLO_TREATY)) {
            cost /= 2;
        }
        int tech_comm_target = Factions[base->faction_id].tech_commerce_bonus;
        tech_comm_target *= tech_comm_target;
        int tech_comm_probe = Factions[faction_id].tech_commerce_bonus;
        tech_comm_probe *= tech_comm_probe;
        cost = (cost * (tech_comm_target + 1)) / (tech_comm_probe + 1);
    } else if (is_pact) {
        cost *= 2;
    }
    if (base->faction_id_former == faction_id) {
        cost /= 2;
    }
    if (base->state_flags & BSTATE_DRONE_RIOTS_ACTIVE) {
        cost /= 2;
    }
    if (base->state_flags & BSTATE_GOLDEN_AGE_ACTIVE) {
        cost *= 2;
    }
    if (has_treaty(base->faction_id, faction_id, DIPLO_ATROCITY_VICTIM)) {
        cost *= 2;
    } else if (has_treaty(base->faction_id, faction_id, DIPLO_WANT_REVENGE)) {
        cost += cost / 2;
    }
    return cost;
}

/*
Calculate the cost for the faction to corner the Global Energy Market (Economic Victory).
Return Value: Cost to corner the Global Energy Market
*/
int __cdecl corner_market(int faction_id) {
    int cost = 0;
    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id != faction_id) {
            if (!has_treaty(base->faction_id, faction_id, DIPLO_PACT)
            || !has_treaty(base->faction_id, faction_id, DIPLO_HAVE_SURRENDERED)) {
                cost += mod_mind_control(i, faction_id, true);
            }
        }
    }
    return max(1000, cost);
}

} // namespace engine
} // namespace alpha
