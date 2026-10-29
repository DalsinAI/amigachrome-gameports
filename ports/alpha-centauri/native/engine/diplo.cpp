/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP8: treaties, atrocities, mood, council votes and the diplomacy helpers of
 * Thinker Mod src/faction.cpp and src/game.cpp at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * Bodies are Thinker's; deviations are marked "Fix:" or "Original:".
 */
#include "engine.h"
#include "diplo.h"

namespace alpha {
namespace engine {

/* ---- Runtime globals (terranx.exe data, not saved) ---- */
namespace {
struct DiploRuntime {
    int friction;
    int friction_faction_id;
    int friction_faction_id_with;
    int second_faction;
    int hostility;
    int prev_hostility;
    int attack_chance;
    int tribute_demand;
    int fear_scale;
    int conquest_fear;
    int exposed_base_count;
    int open_base_id;
    int captured_bases;
    int near_endgame;
    int rival_outranks;
    int great_satan;
    int runaway_leader;
    int reputation;
    int player_pact_count;
    int rival_pact_count;
    int rival_strong_pacts;
    int rival_pact_score;
    int spoke_turn;
    int region_factions[MaxRegionNum];
    int enemy_ally_count;
    int enemy_ally;
    int goodwill;
    int help_faction;
    int unmet_enemy;
    int player_enemy;
    int unmet_faction;
    int mutual_faction;
    int rival_war_count;
    int can_surrender;
    int can_betray;
    int state_a[MaxPlayerNum];
    int state_b[MaxPlayerNum][MaxPlayerNum];
    int council_proposal[MaxPlayerNum];
    int council_vote_state[MaxPlayerNum];
    int game_victory_type;
    int control_turn_b;
    int net_income;
    int ambience_flag; // dword_74DA9C
};
DiploRuntime rt;
char PlrBuf[StrBufLen] = {};
} // namespace

int* DiploFriction = &rt.friction;
int* DiploFrictionFactionID = &rt.friction_faction_id;
int* DiploFrictionFactionIDWith = &rt.friction_faction_id_with;
int* DiploSecondFaction = &rt.second_faction;
int* DiploHostility = &rt.hostility;
int* DiploPrevHostility = &rt.prev_hostility;
int* DiploAttackChance = &rt.attack_chance;
int* DiploTributeDemand = &rt.tribute_demand;
int* DiploFearScale = &rt.fear_scale;
int* DiploConquestFear = &rt.conquest_fear;
int* DiploExposedBaseCount = &rt.exposed_base_count;
int* DiploOpenBaseID = &rt.open_base_id;
int* DiploCapturedBases = &rt.captured_bases;
int* DiploNearEndgame = &rt.near_endgame;
int* DiploRivalOutranks = &rt.rival_outranks;
int* DiploGreatSatan = &rt.great_satan;
int* DiploRunawayLeader = &rt.runaway_leader;
int* DiploReputation = &rt.reputation;
int* DiploPlayerPactCount = &rt.player_pact_count;
int* DiploRivalPactCount = &rt.rival_pact_count;
int* DiploRivalStrongPacts = &rt.rival_strong_pacts;
int* DiploRivalPactScore = &rt.rival_pact_score;
int* DiploSpokeTurn = &rt.spoke_turn;
int* DiploRegionFactions = rt.region_factions;
int* DiploEnemyAllyCount = &rt.enemy_ally_count;
int* DiploEnemyAlly = &rt.enemy_ally;
int* DiploGoodwill = &rt.goodwill;
int* DiploHelpFaction = &rt.help_faction;
int* DiploUnmetEnemy = &rt.unmet_enemy;
int* DiploPlayerEnemy = &rt.player_enemy;
int* DiploUnmetFaction = &rt.unmet_faction;
int* DiploMutualFaction = &rt.mutual_faction;
int* DiploRivalWarCount = &rt.rival_war_count;
int* DiploCanSurrender = &rt.can_surrender;
int* DiploCanBetray = &rt.can_betray;
int* DiploStateA = rt.state_a;
int (*DiploStateB)[8] = rt.state_b;
int* CouncilProposals = rt.council_proposal;
int* CouncilVoteState = rt.council_vote_state;
int* GameVictoryType = &rt.game_victory_type;
int* ControlTurnB = &rt.control_turn_b;
int* FactionCurrentNetIncome = &rt.net_income;

void diplo_runtime_reset() {
    memset(&rt, 0, sizeof(rt));
}

/* ---- game.cpp ---- */

bool un_charter() {
    return ProposalPassCount[PROP_REPEAL_UN_CHARTER]
        <= ProposalPassCount[PROP_REINSTATE_UN_CHARTER];
}

bool global_trade_pact() {
    return ProposalPassCount[PROP_REPEAL_GLOBAL_TRADE_PACT]
        < ProposalPassCount[PROP_GLOBAL_TRADE_PACT];
}

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

/* ---- faction.cpp: treaties ---- */

void __cdecl treaty_off(int faction_id_1, int faction_id_2, uint32_t status) {
    debug("treaty_off %d %d %08X\n", faction_id_1, faction_id_2, status);
    // Fix: bounds check like treaty_on (the original indexed blindly)
    if (!(faction_id_1 >= 0 && faction_id_1 < MaxPlayerNum
    && faction_id_2 >= 0 && faction_id_2 < MaxPlayerNum)) {
        return;
    }
    Faction* plr1 = &Factions[faction_id_1];
    Faction* plr2 = &Factions[faction_id_2];

    if (status & (DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT)) {
        status |= (DIPLO_UNK_1000000|DIPLO_UNK_100000);
    }
    if (status & DIPLO_VENDETTA) {
        status |= (DIPLO_UNK_8000|DIPLO_UNK_40);
    }
    if (status & DIPLO_WANT_REVENGE) {
        plr1->diplo_agenda[faction_id_2] &= ~(AGENDA_UNK_800|AGENDA_UNK_400);
        plr2->diplo_agenda[faction_id_1] &= ~(AGENDA_UNK_800|AGENDA_UNK_400);
    }
    plr1->diplo_status[faction_id_2] &= ~status;
    plr2->diplo_status[faction_id_1] &= ~status;
}

void __cdecl agenda_off(int faction_id_1, int faction_id_2, uint32_t status) {
    debug("agenda_off %d %d %08X\n", faction_id_1, faction_id_2, status);
    if (!(faction_id_1 >= 0 && faction_id_1 < MaxPlayerNum
    && faction_id_2 >= 0 && faction_id_2 < MaxPlayerNum)) {
        return; // Fix: bounds check
    }
    Faction* plr1 = &Factions[faction_id_1];
    Faction* plr2 = &Factions[faction_id_2];
    plr1->diplo_agenda[faction_id_2] &= ~status;
    plr2->diplo_agenda[faction_id_1] &= ~status;
}

void __cdecl treaty_on(int faction_id_1, int faction_id_2, uint32_t status) {
    debug("treaty_on %d %d %08X\n", faction_id_1, faction_id_2, status);
    if (!(faction_id_1 >= 0 && faction_id_1 < MaxPlayerNum
    && faction_id_2 >= 0 && faction_id_2 < MaxPlayerNum)) {
        assert(0);
        return;
    }
    const bool is_player = (faction_id_1 == *CurrentPlayerFaction || faction_id_2 == *CurrentPlayerFaction);
    Faction* plr1 = &Factions[faction_id_1];
    Faction* plr2 = &Factions[faction_id_2];
    // Thinker faction.h DiploExcludeTruce / DiploExcludeFight
    const uint32_t DiploExcludeTruce = (DIPLO_UNK_100000|DIPLO_UNK_10000|DIPLO_UNK_8000|DIPLO_UNK_40|DIPLO_VENDETTA);
    const uint32_t DiploExcludeFight = (DIPLO_UNK_4000000|DIPLO_UNK_1000000|DIPLO_UNK_100000|DIPLO_UNK_10000|
        DIPLO_UNK_800|DIPLO_SHALL_BETRAY|DIPLO_UNK_80|DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT);

    if (status & (DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT)) {
        if (is_player && plr1->diplo_status[faction_id_2] & DIPLO_VENDETTA) {
            rt.ambience_flag &= ~1;
        }
        plr1->diplo_status[faction_id_2] &= ~DiploExcludeTruce;
        plr2->diplo_status[faction_id_1] &= ~DiploExcludeTruce;
    }
    if (status & DIPLO_PACT) {
        status |= DIPLO_TREATY;
    }
    if (status & DIPLO_VENDETTA) {
        if (plr1->diplo_status[faction_id_2] & (DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT)) {
            plr1->diplo_combat_count[faction_id_2] = 0;
            plr2->diplo_combat_count[faction_id_1] = 0;
        }
        status |= (DIPLO_UNK_80000000|DIPLO_UNK_100|DIPLO_COMMLINK);
        plr1->diplo_status[faction_id_2] &= ~DiploExcludeFight;
        plr2->diplo_status[faction_id_1] &= ~DiploExcludeFight;
        if (is_player) {
            rt.ambience_flag |= 1;
            // Original: ambience(204) (war music); no sound in the engine
        }
    }
    if (status & (DIPLO_VENDETTA|DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT)) {
        plr1->diplo_status[faction_id_2] &= ~DIPLO_UNK_800000;
        plr2->diplo_status[faction_id_1] &= ~DIPLO_UNK_800000;
    }
    plr1->diplo_status[faction_id_2] |= status;
    plr2->diplo_status[faction_id_1] |= status;
    // Original: redraw of MultiWin when visible (UI)
}

void __cdecl agenda_on(int faction_id_1, int faction_id_2, uint32_t status) {
    debug("agenda_on %d %d %08X\n", faction_id_1, faction_id_2, status);
    if (!(faction_id_1 >= 0 && faction_id_1 < MaxPlayerNum
    && faction_id_2 >= 0 && faction_id_2 < MaxPlayerNum)) {
        assert(0);
        return;
    }
    Faction* plr1 = &Factions[faction_id_1];
    Faction* plr2 = &Factions[faction_id_2];
    if (status & AGENDA_UNK_20) {
        plr1->diplo_agenda[faction_id_2] &= ~AGENDA_UNK_40;
        plr2->diplo_agenda[faction_id_1] &= ~AGENDA_UNK_40;
    }
    plr1->diplo_agenda[faction_id_2] |= status;
    plr2->diplo_agenda[faction_id_1] |= status;
}

void __cdecl set_treaty(int faction_id_1, int faction_id_2, uint32_t status, bool add) {
    if (faction_id_1 >= 0 && faction_id_1 < MaxPlayerNum
    && faction_id_2 >= 0 && faction_id_2 < MaxPlayerNum) {
        if (add) {
            Factions[faction_id_1].diplo_status[faction_id_2] |= status;
            if (status & DIPLO_UNK_40) {
                Factions[faction_id_1].diplo_merc[faction_id_2] = 50;
            }
            // Original: DIPLO_HAVE_INFILTRATOR with conf.counter_espionage renews the
            // probe timer (probe.cpp). counter_espionage is off in the vanilla
            // config and probe teams are deferred.
        } else {
            Factions[faction_id_1].diplo_status[faction_id_2] &= ~status;
        }
    } else {
        assert(0);
    }
}

void __cdecl set_agenda(int faction_id_1, int faction_id_2, uint32_t status, bool add) {
    if (faction_id_1 >= 0 && faction_id_1 < MaxPlayerNum
    && faction_id_2 >= 0 && faction_id_2 < MaxPlayerNum) {
        if (add) {
            Factions[faction_id_1].diplo_agenda[faction_id_2] |= status;
        } else {
            Factions[faction_id_1].diplo_agenda[faction_id_2] &= ~status;
        }
    } else {
        assert(0);
    }
}

/* ---- faction.cpp: atrocities and treaty breaking ---- */

void __cdecl atrocity(int faction_id, int faction_id_tgt, int skip_init_check, int skip_human_check) {
    // Fix: reject invalid factions instead of indexing out of range
    if (!(faction_id >= 0 && faction_id < MaxPlayerNum
    && faction_id_tgt >= 0 && faction_id_tgt < MaxPlayerNum)) {
        return;
    }
    Faction* plr = &Factions[faction_id];
    MFaction* m_plr = &MFactions[faction_id];
    MFaction* m_tgt = &MFactions[faction_id_tgt];
    bool prev_victim = has_treaty(faction_id_tgt, faction_id, DIPLO_ATROCITY_VICTIM);
    set_treaty(faction_id_tgt, faction_id, DIPLO_ATROCITY_VICTIM|DIPLO_WANT_REVENGE, 1);
    set_agenda(faction_id_tgt, faction_id, AGENDA_UNK_4, 1);

    if (un_charter()) {
        if (*SunspotDuration > 0) {
            if (faction_id == *CurrentPlayerFaction) {
                NetMsg_pop(NetMsg, "ATROCITYSPOTS", 5000, 0, 0);
            }
        } else if (!*ExpansionEnabled || skip_human_check || (!m_plr->is_alien() && !m_tgt->is_alien())) {
            for (int i = 1; i < MaxPlayerNum; i++) {
                if (i != faction_id && i != faction_id_tgt && has_treaty(i, faction_id, DIPLO_COMMLINK)
                && (!*ExpansionEnabled || !skip_human_check || m_plr->is_alien() || !MFactions[i].is_alien())) {
                    if (has_treaty(i, faction_id, DIPLO_UNK_200000)) {
                        cause_friction(i, faction_id, 5);
                        if (!is_human(i) && is_human(faction_id) && !prev_victim
                        && has_treaty(faction_id, i, DIPLO_COMMLINK)
                        && !has_treaty(faction_id, i, DIPLO_VENDETTA|DIPLO_PACT)) {
                            bool toggle;
                            if (has_treaty(i, faction_id, DIPLO_WANT_REVENGE)) {
                                toggle = true;
                            } else {
                                if (great_satan(faction_id, 0)) {
                                    toggle = game_randv(16) <= plr->atrocities + 8;
                                } else {
                                    int val = (*GameRules & RULES_INTENSE_RIVALRY ? 5 : *DiffLevel);
                                    toggle = !game_randv(2 * (8 - val));
                                }
                            }
                            if (toggle) {
                                if (faction_id == *CurrentPlayerFaction) {
                                    parse_says(0, get_title(faction_id), -1, -1);
                                    parse_says(1, get_name(faction_id), -1, -1);
                                    // Original: diplomacy_caption(faction_id, i) (window title)
                                    X_pops("ATROCIOUSITY", FactionPortraits[i], 0);
                                }
                                treaty_on(faction_id, i, DIPLO_VENDETTA);
                                Factions[i].diplo_status[faction_id] |= DIPLO_UNK_40;
                                Factions[i].diplo_merc[faction_id] = 50;
                            }
                        }
                    }
                    if (!has_treaty(i, faction_id, DIPLO_VENDETTA)) {
                        treaty_on(i, faction_id, DIPLO_UNK_400000);
                    }
                }
            }
            if (skip_init_check || !plr->atrocities) {
                plr->atrocities++;
                if (!has_treaty(faction_id, faction_id_tgt, DIPLO_ATROCITY_VICTIM)) {
                    plr->integrity_blemishes++;
                }
            }
            if (plr->atrocities <= 4 * (8 - plr->diff_level)) {
                int turns = 10 * plr->atrocities;
                // Fix: game used wrong variable while checking !is_human(faction_id_tgt)
                if (is_human(faction_id) && !is_human(faction_id_tgt) && !prev_victim && plr->atrocities >= 5) {
                    Factions[faction_id_tgt].player_flags |= PFLAG_COMMIT_ATROCITIES_WANTONLY;
                }
                parse_says(0, get_title(faction_id), -1, -1);
                parse_says(1, get_name(faction_id), -1, -1);
                parse_says(2, get_title(faction_id_tgt), -1, -1);
                parse_says(3, get_name(faction_id_tgt), -1, -1);
                parse_num(0, turns);
                if (!m_plr->is_alien()) {
                    if (faction_id == *CurrentPlayerFaction) {
                        NetMsg_pop(NetMsg, "ATROCITY2", 5000, 0, 0);
                    } else if (faction_id_tgt == faction_id) {
                        NetMsg_pop(NetMsg, "ATROCITY1", 5000, 0, 0);
                    } else {
                        NetMsg_pop(NetMsg, "ATROCITY", 5000, 0, 0);
                    }
                }
                plr->sanction_turns += turns;
            } else {
                major_atrocity(faction_id, faction_id_tgt);
            }
        }
    }
}

void __cdecl major_atrocity(int faction_id, int faction_id_tgt) {
    if (!(faction_id >= 0 && faction_id < MaxPlayerNum && faction_id_tgt < MaxPlayerNum)) {
        return; // Fix: bounds check
    }
    Faction* plr = &Factions[faction_id];
    if (faction_id_tgt >= 0) {
        set_treaty(faction_id_tgt, faction_id, DIPLO_MAJOR_ATROCITY_VICTIM|DIPLO_ATROCITY_VICTIM|DIPLO_WANT_REVENGE, 1);
        set_agenda(faction_id_tgt, faction_id, AGENDA_UNK_4, 1);
    }
    if (un_charter()) {
        if (faction_id_tgt < 0) {
            plr->integrity_blemishes += 2;
        } else if (!has_treaty(faction_id, faction_id_tgt, DIPLO_MAJOR_ATROCITY_VICTIM)) {
            plr->integrity_blemishes = clamp(plr->integrity_blemishes + 2, 6, 99);
        }
        plr->major_atrocities++;
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (i != faction_id && i != faction_id_tgt && !is_human(i) && is_alive(i)) {
                if (!has_treaty(faction_id, i, DIPLO_VENDETTA)) {
                    if (has_treaty(faction_id, i, DIPLO_PACT)) {
                        pact_ends(faction_id, i);
                    }
                    treaty_on(i, faction_id, DIPLO_VENDETTA|DIPLO_COMMLINK);
                    Factions[i].diplo_status[faction_id] |= DIPLO_UNK_40;
                    Factions[i].diplo_merc[faction_id] = 50;
                    plr->diplo_spoke[i] = *CurrentTurn;
                    parse_says(0, get_title(i), -1, -1);
                    parse_says(1, get_name(i), -1, -1);
                    parse_says(2, get_noun(i), -1, -1);
                    parse_says(3, get_title(faction_id), -1, -1);
                    parse_says(4, get_name(faction_id), -1, -1);
                    parse_says(5, get_noun(faction_id), -1, -1);
                    snprintf(StrBuffer, StrBufLen, "MAJORATROCITY%d", faction_id != *CurrentPlayerFaction);
                    popp(ScriptFile, StrBuffer, 0, "council_sm.pcx", 0);
                }
            }
        }
    }
}

int __cdecl break_treaty(int faction_id, int faction_id_tgt, uint32_t status) {
    uint32_t current = Factions[faction_id].diplo_status[faction_id_tgt];
    if ((current & DIPLO_VENDETTA) || !(current & status)) {
        return 0;
    }
    parse_says(0, get_title(faction_id_tgt), -1, -1);
    parse_says(1, get_name(faction_id_tgt), -1, -1);
    parse_says(2, get_noun(faction_id_tgt), -1, -1);
    parse_says(3, get_pact_hood(faction_id, faction_id_tgt), -1, -1);
    parse_says(4, get_adjective(faction_id_tgt), -1, -1);
    if (current & DIPLO_PACT) {
        NetMsg_pop(NetMsg, "BREAKINGPACT", -5000, 0, "break_sm.pcx");
        return 1;
    }
    if (current & DIPLO_TREATY) {
        return popp(ScriptFile, "BREAKINGTREATY", 0, "break_sm.pcx", 0) == 0;
    }
    if ((current & DIPLO_TRUCE) && !(current & DIPLO_UNK_10000)) {
        return popp(ScriptFile, "BREAKINGTRUCE", 0, "break_sm.pcx", 0) == 0;
    }
    if (status & DIPLO_UNK_10000) {
        return 0;
    }
    return popp(ScriptFile, "BEGINVENDETTA", 0, "break_sm.pcx", 0) == 0;
}

void __cdecl intervention(int faction_id_def, int faction_id_atk) {
    for (int i = 1; i < MaxPlayerNum; i++) {
        Faction* plr = &Factions[i];
        if (i != faction_id_def && i != faction_id_atk && is_alive(i)
        && plr->diplo_status[faction_id_def] & DIPLO_PACT
        && !(plr->diplo_status[faction_id_atk] & (DIPLO_VENDETTA|DIPLO_PACT))) {
            parse_says(0, get_noun(i), -1, -1);
            parse_says(1, get_noun(faction_id_def), -1, -1);
            parse_says(2, get_title(i), -1, -1);
            parse_says(3, get_name(i), -1, -1);
            parse_says(4, get_title(faction_id_atk), -1, -1);
            parse_says(5, get_name(faction_id_atk), -1, -1);
            parse_says(6, get_noun(faction_id_atk), -1, -1);
            if (is_human(faction_id_atk) || is_human(faction_id_def)) {
                if (is_human(faction_id_atk)) {
                    Factions[faction_id_atk].diplo_spoke[i] = *CurrentTurn;
                }
                treaty_on(i, faction_id_atk, DIPLO_UNK_80000000|DIPLO_VENDETTA|DIPLO_COMMLINK);
                set_treaty(i, faction_id_atk, DIPLO_UNK_40, 1);
            }
            if (faction_id_atk == *CurrentPlayerFaction) {
                X_pop("AIDSTHEM", 0);
            } else if (faction_id_def == *CurrentPlayerFaction) {
                X_pop("AIDSUS", 0);
            }
        }
    }
}

void __cdecl double_cross(int faction_id_atk, int faction_id_def, int faction_id_other) {
    Faction* plr_atk = &Factions[faction_id_atk];
    Faction* plr_def = &Factions[faction_id_def];

    if (*MultiplayerActive) {
        if (faction_id_def != *CurrentPlayerFaction || faction_id_other <= 0) {
            if (faction_id_def == *CurrentPlayerFaction && is_human(faction_id_atk)) {
                parse_says(0, get_title(faction_id_atk), -1, -1);
                parse_says(1, get_name(faction_id_atk), -1, -1);
                parse_says(2, get_noun(faction_id_atk), -1, -1);
                NetMsg_pop(NetMsg, "VENDETTAWARNING", 5000, 0, 0);
            }
        } else {
            parse_says(0, get_title(faction_id_other), -1, -1);
            parse_says(1, get_name(faction_id_other), -1, -1);
            parse_says(2, get_noun(faction_id_other), -1, -1);
            parse_says(3, get_title(faction_id_atk), -1, -1);
            parse_says(4, get_name(faction_id_atk), -1, -1);
            parse_says(5, get_noun(faction_id_atk), -1, -1);
            NetMsg_pop(NetMsg, "INCITED", 5000, 0, 0);
        }
    }
    plr_def->loan_balance[faction_id_atk] = 0;
    int is_allowed = 0;
    int is_victim = 0;
    if (has_treaty(faction_id_atk, faction_id_def, DIPLO_ATROCITY_VICTIM) || plr_def->major_atrocities) {
        is_victim = 1;
    }
    if (faction_id_other >= 0) {
        if (has_treaty(faction_id_atk, faction_id_other, DIPLO_PACT)) {
            if (!has_treaty(faction_id_atk, faction_id_def, DIPLO_PACT)) {
                is_allowed = 1;
            }
        } else {
            if (!has_treaty(faction_id_atk, faction_id_other, DIPLO_TREATY)) {
                if (has_treaty(faction_id_atk, faction_id_other, DIPLO_TRUCE)
                && !has_treaty(faction_id_atk, faction_id_def, DIPLO_TRUCE)) {
                    is_allowed = 1;
                }
            } else if (!has_treaty(faction_id_atk, faction_id_def, DIPLO_TREATY)) {
                is_allowed = 1;
            }
        }
        plr_atk->diplo_gifts[faction_id_other]++;
    }
    if (has_treaty(faction_id_atk, faction_id_def, DIPLO_PACT)) {
        if (!has_agenda(faction_id_atk, faction_id_def, AGENDA_UNK_20)) {
            plr_atk->diplo_reputation = max(0, plr_atk->diplo_reputation - 1);
            if (plr_atk->diff_level && !is_victim && !is_allowed) {
                plr_atk->integrity_blemishes++;
                plr_def->diplo_betrayed[faction_id_atk]++;
            }
            if (faction_id_other >= 0) {
                cause_friction(faction_id_other, faction_id_atk, -4);
            } else {
                if (!is_victim) {
                    plr_atk->integrity_blemishes++;
                }
                plr_atk->diplo_wrongs[faction_id_def]++;
                plr_def->diplo_betrayed[faction_id_atk]++;
            }
            if (is_human(faction_id_atk)) {
                if (has_agenda(faction_id_def, faction_id_atk, AGENDA_UNK_400)) {
                    set_agenda(faction_id_def, faction_id_atk, AGENDA_UNK_800, 1);
                }
                if (has_treaty(faction_id_def, faction_id_atk, DIPLO_WANT_REVENGE)) {
                    set_agenda(faction_id_def, faction_id_atk, AGENDA_UNK_400, 1);
                }
                set_treaty(faction_id_def, faction_id_atk, DIPLO_WANT_REVENGE, 1);
            }
        }
        pact_ends(faction_id_atk, faction_id_def);
        treaty_on(faction_id_atk, faction_id_def, DIPLO_VENDETTA);
        return; // Skip other cases
    }
    if (!has_agenda(faction_id_atk, faction_id_def, AGENDA_UNK_20)) {
        bool has_truce = has_treaty(faction_id_atk, faction_id_def, DIPLO_TRUCE|DIPLO_TREATY);
        if (has_truce) {
            if (!has_treaty(faction_id_atk, faction_id_def, DIPLO_UNK_10000)) {
                plr_atk->diplo_reputation = max(0, plr_atk->diplo_reputation - 1);
                if ((great_satan(faction_id_atk, 0) || (plr_atk->ranking >= 6 && plr_def->ranking <= 4))
                && faction_id_other < 0 && !is_victim) {
                    plr_atk->integrity_blemishes++;
                    plr_def->diplo_betrayed[faction_id_atk]++;
                }
            }
            if (plr_atk->diff_level && !is_victim && !is_allowed) {
                plr_atk->integrity_blemishes++;
                plr_def->diplo_betrayed[faction_id_atk]++;
            }
            if (faction_id_other < 0) {
                plr_atk->diplo_wrongs[faction_id_def]++;
            }
        }
        if (has_truce || faction_id_other >= 0) {
            if (is_human(faction_id_atk)) {
                if (has_agenda(faction_id_def, faction_id_atk, AGENDA_UNK_400)) {
                    set_agenda(faction_id_def, faction_id_atk, AGENDA_UNK_800, 1);
                }
                if (has_treaty(faction_id_def, faction_id_atk, DIPLO_WANT_REVENGE)) {
                    set_agenda(faction_id_def, faction_id_atk, AGENDA_UNK_400, 1);
                }
                set_treaty(faction_id_def, faction_id_atk, DIPLO_WANT_REVENGE, 1);
            }
            if (faction_id_other >= 0) {
                cause_friction(faction_id_other, faction_id_atk, -2);
            }
        }
    }
    treaty_on(faction_id_atk, faction_id_def, DIPLO_VENDETTA);
    if (!has_agenda(faction_id_atk, faction_id_def, AGENDA_UNK_20)) {
        if (is_human(faction_id_atk) != is_human(faction_id_def)) {
            set_treaty(faction_id_atk, faction_id_def, DIPLO_UNK_40, 1);
            plr_atk->diplo_spoke[faction_id_def] = *CurrentTurn;
        }
        intervention(faction_id_def, faction_id_atk);
    }
}

int __cdecl act_of_aggression(int faction_id_atk, int faction_id_def) {
    int value = 0;
    if (has_treaty(faction_id_def, faction_id_atk, DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT)) {
        double_cross(faction_id_atk, faction_id_def, -1);
        value = 1;
    }
    treaty_on(faction_id_atk, faction_id_def, DIPLO_UNK_8000|DIPLO_VENDETTA);
    return value;
}

int __cdecl spying(int faction_id) {
    return has_treaty(MapWin->cOwner, faction_id, DIPLO_HAVE_INFILTRATOR)
        || has_project(FAC_EMPATH_GUILD, MapWin->cOwner)
        || (MapWin->cOwner == *GovernorFaction && !MFactions[faction_id].is_alien());
}

/* ---- faction.cpp: council votes, threats and mood ---- */

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

/*
Check whether a faction's leader is eligible to be a Planetary Governor candidate.
Return Value: Is the leader eligible (top two vote totals)? true/false
*/
int __cdecl eligible(int faction_id) {
    if (is_alien(faction_id)) {
        return false;
    }
    int votes = council_votes(faction_id);
    int faction_count = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (faction_id != i && is_alive(i) && council_votes(i) > votes) {
            faction_count++;
        }
    }
    return faction_count < 2;
}

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

int __cdecl climactic_battle() {
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (is_human(i) && Factions[i].corner_market_turn > *CurrentTurn) {
            return true;
        }
    }
    // Removed following code since this will always return false.
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

int __cdecl at_climax(int faction_id) {
    if (is_human(faction_id) || *GameState & STATE_GAME_DONE
    || *DiffLevel == DIFF_CITIZEN || !climactic_battle()) {
        return false;
    }
    if (aah_ooga(faction_id, faction_id)) {
        return true;
    }
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (i != faction_id && Factions[faction_id].corner_market_turn > *CurrentTurn
        && (!has_treaty(faction_id, i, DIPLO_PACT) || !(*GameRules & RULES_VICTORY_COOPERATIVE))) {
            return true;
        }
    }
    int sp_mins_them = 0;
    int sp_mins_own = 0;
    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].queue_items[0] == -FAC_ASCENT_TO_TRANSCENDENCE) {
            int min_accum = Bases[i].minerals_accumulated;
            if (Bases[i].faction_id == faction_id) {
                if (sp_mins_own <= min_accum) {
                    sp_mins_own = min_accum;
                }
            } else if (sp_mins_them <= min_accum) {
                sp_mins_them = min_accum;
            }
        }
    }
    return sp_mins_them && sp_mins_them > sp_mins_own;
}

void __cdecl cause_friction(int faction_id, int faction_id_with, int friction) {
    if (!(faction_id >= 0 && faction_id < MaxPlayerNum
    && faction_id_with >= 0 && faction_id_with < MaxPlayerNum)) {
        return; // Fix: bounds check
    }
    Faction* f = &Factions[faction_id];
    f->diplo_friction[faction_id_with] = clamp(f->diplo_friction[faction_id_with] + friction, 0, 20);
    if (*DiploFrictionFactionID == faction_id && *DiploFrictionFactionIDWith == faction_id_with) {
        *DiploFriction = clamp(*DiploFriction + friction, 0, 20); // Fix: add variable bounding
    }
}

/*
Normalize the diplomatic friction value into a mood offset.
Return Value: Mood (0-8)
*/
int __cdecl get_mood(int friction) {
    if (friction <= 0) {
        return MOOD_MAGNANIMOUS; // can be also displayed as "Submissive"
    }
    if (friction <= 2) {
        return MOOD_SOLICITOUS;
    }
    if (friction <= 4) {
        return MOOD_COOPERATIVE;
    }
    if (friction <= 8) {
        return MOOD_NONCOMMITTAL;
    }
    if (friction <= 12) {
        return MOOD_AMBIVALENT;
    }
    if (friction <= 15) {
        return MOOD_OBSTINATE;
    }
    if (friction <= 17) {
        return MOOD_QUARRELSOME;
    }
    return (friction > 19) + MOOD_BELLIGERENT; // or MOOD_SEETHING if condition is true
}

int __cdecl reputation(int faction_id, int faction_id_with) {
    return clamp(Factions[faction_id].integrity_blemishes
        - Factions[faction_id].diplo_gifts[faction_id_with], 0, 99);
}

int __cdecl get_patience(int faction_id_with, int faction_id) {
    // Original: debug builds returned conf.diplo_patience when set
    if (has_treaty(faction_id, faction_id_with, DIPLO_VENDETTA)) {
        return 1;
    }
    if (has_treaty(faction_id, faction_id_with, DIPLO_PACT)) {
        return has_treaty(faction_id, faction_id_with, DIPLO_HAVE_SURRENDERED) ? 500 : 6;
    }
    return (has_treaty(faction_id, faction_id_with, DIPLO_TREATY) != 0)
        - ((*DiploFriction + 3) / 8) + 3;
}

int __cdecl energy_value(int loan_principal) {
    int goodwill = 0;
    int divisor = 2;
    for (int weight = 10, energy = loan_principal / 5; energy > 0; energy -= weight, weight = 20) {
        goodwill += ((weight >= 0) ? ((energy > weight) ? weight : energy) : 0) / divisor++;
    }
    return (goodwill + 4) / 5;
}

/* ---- faction.cpp: names ---- */

char* __cdecl get_pact(int faction_id) {
    *PluralDefault = 0;
    *GenderDefault = MFactions[faction_id].is_leader_female;
    if (MFactions[faction_id].is_leader_female) {
        return label_get(TL_PactSister);
    }
    return label_get(TL_PactBrother);
}

char* __cdecl get_pacts(int faction_id) {
    *PluralDefault = 1;
    *GenderDefault = MFactions[faction_id].is_leader_female;
    return label_get(MFactions[faction_id].is_leader_female ? TL_PactSisters : TL_PactBrothers);
}

char* __cdecl get_pacts_2(int faction_id, int faction_id_2) {
    int is_female_1 = MFactions[faction_id].is_leader_female;
    int is_female_2 = MFactions[faction_id_2].is_leader_female;
    *PluralDefault = 1;
    *GenderDefault = is_female_1 && is_female_2;
    return label_get(is_female_1 && is_female_2 ? TL_PactSisters : TL_PactBrothers);
}

char* __cdecl get_pact_hood(int faction_id, int faction_id_2) {
    *PluralDefault = 0;
    *GenderDefault = 0;
    if (MFactions[faction_id].is_leader_female && MFactions[faction_id_2].is_leader_female) {
        return label_get(TL_PactOfSisterhood);
    }
    return label_get(TL_PactOfBrotherhood);
}

static char* plr_case(int tgl) {
    if (tgl == 1) {
        PlrBuf[0] = (char)toupper((unsigned char)PlrBuf[0]);
    } else if (tgl != 0) {
        for (char* p = PlrBuf; *p; p++) { // Original: CharUpperA(PlrBuf)
            *p = (char)toupper((unsigned char)*p);
        }
    }
    return PlrBuf;
}

char* __cdecl get_his_her(int faction_id, int tgl) {
    snprintf(PlrBuf, StrBufLen, "%s",
        MFactions[faction_id].is_leader_female ? label_get(TL_Her2) : label_get(TL_His));
    return plr_case(tgl);
}

char* __cdecl get_him_her(int faction_id, int tgl) {
    snprintf(PlrBuf, StrBufLen, "%s",
        MFactions[faction_id].is_leader_female ? label_get(TL_Her) : label_get(TL_Him));
    return plr_case(tgl);
}

char* __cdecl get_he_she(int faction_id, int tgl) {
    snprintf(PlrBuf, StrBufLen, "%s",
        MFactions[faction_id].is_leader_female ? label_get(TL_She) : label_get(TL_He));
    return plr_case(tgl);
}

/* ---- faction.cpp: wants_to_attack ---- */

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

int __cdecl mod_wants_to_attack(int faction_id, int faction_id_tgt, int faction_id_unk) {
    int value = evaluate_attack(faction_id, faction_id_tgt, faction_id_unk);

    debug("wants_to_attack turn: %d factions: %d %d %d value: %d\n",
        *CurrentTurn, faction_id, faction_id_tgt, faction_id_unk, value);
    return value;
}

} // namespace engine
} // namespace alpha
