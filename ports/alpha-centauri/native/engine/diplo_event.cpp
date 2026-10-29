/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP8: diplomacy events and computer-player diplomacy.
 * encounter, enemy_diplomacy, the pact and enemies events, comm_check and
 * territory are Thinker Mod src/faction_event.cpp, diplomacy_check is
 * src/faction_diplo.cpp and action_sat_attack is src/basewin.cpp at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * Bodies are Thinker's; deviations are marked "Fix:" or "Original:".
 *
 * pact_ends, communicate, trade_maps and the comm-link proposal rules
 * (diplo_proposal_*) exist only in terranx.exe and are new code.
 */
#include "engine.h"
#include "diplo.h"
#include "turn.h"

namespace alpha {
namespace engine {

/* Owned by other packages, not in engine_wp.h; weak fallbacks in wp8_deps.cpp. */
int __cdecl contiguous(int faction_id_1, int faction_id_2, int region, int flag); // WP3 path.cpp
void __cdecl scan_prototypes(int faction_id, int faction_id_2);                   // WP5 (new)
void __cdecl action_sat_attack(int faction_id, int faction_id_tgt, int target_id, int base_id);

namespace {
int s_encounter_x = -1; // dword_68D43C
int s_encounter_y = -1; // dword_68D440
DiploHooks s_diplo_hooks = {};
} // namespace

DiploHooks& diplo_hooks() {
    return s_diplo_hooks;
}

static void wp8_interlude(int event_id, const char* text, int a3, int a4) {
    // Original: interlude() plays an interlude movie/text; the UI installs the hook.
    if (turn_hooks().interlude) {
        turn_hooks().interlude(event_id, text, a3, a4);
    }
}

static bool player_spying(int player_id, int faction_id_1, int faction_id_2) {
    Faction& plr = Factions[player_id];
    return has_project(FAC_EMPATH_GUILD, player_id)
        || (plr.diplo_status[faction_id_1] & DIPLO_HAVE_INFILTRATOR)
        || (player_id == *GovernorFaction && !MFactions[faction_id_1].is_alien())
        || (plr.diplo_status[faction_id_2] & DIPLO_HAVE_INFILTRATOR)
        || (player_id == *GovernorFaction && !MFactions[faction_id_2].is_alien());
}

void __cdecl pact_attacks(int faction_id_1, int faction_id_2) {
    parse_says(0, get_title(faction_id_1), -1, -1);
    parse_says(1, get_name(faction_id_1), -1, -1);
    parse_says(2, get_noun(faction_id_1), -1, -1);
    parse_says(3, get_he_she(faction_id_1, 0), -1, -1);
    parse_says(4, get_noun(faction_id_2), -1, -1);
    NetMsg_pop(NetMsg, "PACTATTACKS", 5000, 0, 0);
}

void __cdecl pact_unpact(int faction_id_1, int faction_id_2) {
    parse_says(0, get_title(faction_id_1), -1, -1);
    parse_says(1, get_name(faction_id_1), -1, -1);
    parse_says(2, get_noun(faction_id_1), -1, -1);
    parse_says(3, get_he_she(faction_id_1, 0), -1, -1);
    parse_says(4, get_his_her(faction_id_1, 0), -1, -1);
    parse_says(5, get_pact_hood(faction_id_1, faction_id_2), -1, -1);
    parse_says(6, get_title(faction_id_2), -1, -1);
    parse_says(7, get_name(faction_id_2), -1, -1);
    parse_says(8, get_noun(faction_id_2), -1, -1);
    NetMsg_pop(NetMsg, "PACTUNPACT", 5000, 0, 0);
}

void __cdecl pact_truce(int faction_id_1, int faction_id_2) {
    parse_says(0, get_title(faction_id_1), -1, -1);
    parse_says(1, get_name(faction_id_1), -1, -1);
    parse_says(2, get_noun(faction_id_1), -1, -1);
    parse_says(3, get_he_she(faction_id_1, 0), -1, -1);
    parse_says(4, get_title(faction_id_2), -1, -1);
    parse_says(5, get_name(faction_id_2), -1, -1);
    parse_says(6, get_noun(faction_id_2), -1, -1);
    NetMsg_pop(NetMsg, "PACTTRUCE", 5000, 0, 0);
}

void __cdecl pact_treaty(int faction_id_1, int faction_id_2) {
    parse_says(0, get_title(faction_id_1), -1, -1);
    parse_says(1, get_name(faction_id_1), -1, -1);
    parse_says(2, get_noun(faction_id_1), -1, -1);
    parse_says(3, get_he_she(faction_id_1, 0), -1, -1);
    parse_says(4, get_noun(faction_id_2), -1, -1);
    NetMsg_pop(NetMsg, "PACTTREATY", 5000, 0, 0);
}

void __cdecl enemies_war(int faction_id_1, int faction_id_2) {
    const int player_id = MapWin->cOwner;
    Faction& plr1 = Factions[faction_id_1];
    Faction& plr2 = Factions[faction_id_2];
    if (plr1.diplo_status[faction_id_2] & DIPLO_VENDETTA) {
        return;
    }
    if (Factions[player_id].diplo_status[faction_id_1] & DIPLO_PACT) {
        pact_attacks(faction_id_1, faction_id_2);
    } else if (Factions[player_id].diplo_status[faction_id_2] & DIPLO_PACT) {
        pact_attacks(faction_id_2, faction_id_1);
    } else if (player_spying(player_id, faction_id_1, faction_id_2)
    || (*GameState & STATE_OMNISCIENT_VIEW)) {
        parse_says(0, get_noun(faction_id_1), -1, -1);
        parse_says(1, get_noun(faction_id_2), -1, -1);
        NetMsg_pop(NetMsg, "ENEMYWAR", 5000, 0, 0);
    }
    treaty_on(faction_id_1, faction_id_2, DIPLO_VENDETTA);
    if (!(plr1.diplo_status[faction_id_2] & DIPLO_UNK_40000000)) {
        trade_maps(faction_id_1, faction_id_2);
    }
    uint32_t status = plr1.diplo_status[faction_id_2];
    if ((status & DIPLO_WANT_REVENGE) && !(status & DIPLO_ATROCITY_VICTIM)) {
        if (*DiffLevel > 1 && game_rand() % *DiffLevel) {
            plr1.diplo_agenda[faction_id_2] &= ~(AGENDA_UNK_400|AGENDA_UNK_800);
            plr2.diplo_agenda[faction_id_1] &= ~(AGENDA_UNK_400|AGENDA_UNK_800);
            plr1.diplo_status[faction_id_2] &= ~DIPLO_WANT_REVENGE;
            plr2.diplo_status[faction_id_1] &= ~DIPLO_WANT_REVENGE;
            plr1.diplo_status[faction_id_2] |= DIPLO_UNK_40;
            plr1.diplo_merc[faction_id_2] = 50;
        }
    }
    status = plr2.diplo_status[faction_id_1];
    if ((status & DIPLO_WANT_REVENGE) && !(status & DIPLO_ATROCITY_VICTIM)) {
        if (*DiffLevel > 1 && game_rand() % *DiffLevel) {
            plr2.diplo_agenda[faction_id_1] &= ~(AGENDA_UNK_400|AGENDA_UNK_800);
            plr1.diplo_agenda[faction_id_2] &= ~(AGENDA_UNK_400|AGENDA_UNK_800);
            plr2.diplo_status[faction_id_1] &= ~DIPLO_WANT_REVENGE;
            plr1.diplo_status[faction_id_2] &= ~DIPLO_WANT_REVENGE;
            set_treaty(faction_id_2, faction_id_1, DIPLO_UNK_40, true);
        }
    }
}

void __cdecl enemies_unpact(int faction_id_1, int faction_id_2) {
    const int player_id = MapWin->cOwner;
    Faction& plr1 = Factions[faction_id_1];
    Faction& plr2 = Factions[faction_id_2];
    if (plr1.diplo_status[faction_id_2] & DIPLO_PACT) {
        if (plr1.diplo_status[player_id] & DIPLO_PACT) {
            pact_unpact(faction_id_1, faction_id_2);
        } else if (plr2.diplo_status[player_id] & DIPLO_PACT) {
            pact_unpact(faction_id_2, faction_id_1);
        }
        if (player_spying(player_id, faction_id_1, faction_id_2)
        || (*GameState & STATE_OMNISCIENT_VIEW)) {
            parse_says(0, get_noun(faction_id_1), -1, -1);
            parse_says(1, get_noun(faction_id_2), -1, -1);
            parse_says(2, get_pact_hood(faction_id_1, faction_id_2), -1, -1);
            NetMsg_pop(NetMsg, "ENEMYPACTENDS", 5000, 0, 0);
        }
        plr1.diplo_status[faction_id_2] &= ~(DIPLO_UNK_1000000|DIPLO_UNK_100000|DIPLO_PACT);
        plr2.diplo_status[faction_id_1] &= ~(DIPLO_UNK_1000000|DIPLO_UNK_100000|DIPLO_PACT);
    }
}

void __cdecl enemies_team_up(int faction_id_1, int faction_id_2, int faction_id_3) {
    const int player_id = MapWin->cOwner;
    Faction& plr1 = Factions[faction_id_1];
    Faction& plr2 = Factions[faction_id_2];
    if (!(plr1.diplo_status[faction_id_3] & DIPLO_VENDETTA) && is_human(faction_id_3)) {
        if (plr2.diplo_status[faction_id_3] & DIPLO_VENDETTA) {
            if (!great_satan(faction_id_3, 0) && !(plr1.diplo_status[faction_id_3] & DIPLO_WANT_REVENGE)) {
                treaty_on(faction_id_1, faction_id_3, DIPLO_WANT_TO_TALK);
                return;
            }
            if (faction_id_3 == player_id) {
                parse_says(0, get_noun(faction_id_1), -1, -1);
                NetMsg_pop(NetMsg, "TEAM", 5000, 0, 0);
            }
            Factions[faction_id_3].diplo_spoke[faction_id_2] = *CurrentTurn;
            Factions[faction_id_3].diplo_spoke[faction_id_1] = *CurrentTurn;
            treaty_on(faction_id_1, faction_id_3, DIPLO_VENDETTA);
        } else if (great_satan(faction_id_3, 0)
        || (plr1.diplo_status[faction_id_3] & DIPLO_WANT_REVENGE)
        || (plr2.diplo_status[faction_id_3] & DIPLO_WANT_REVENGE)) {
            plr1.diplo_status[faction_id_3] |= DIPLO_SHALL_BETRAY;
        } else {
            treaty_on(faction_id_1, faction_id_3, DIPLO_WANT_TO_TALK);
        }
    }
}

void __cdecl enemies_trade_tech(int faction_id_1, int faction_id_2) {
    Faction& plr1 = Factions[faction_id_1];
    Faction& plr2 = Factions[faction_id_2];
    if (*GameRules & RULES_SCN_NO_TECH_TRADING) {
        return;
    }
    int tech_to1 = -1;
    int tech_to2 = -1;
    int score_to1 = -9999;
    int score_to2 = -9999;
    for (int i = 0; i < MaxTechnologyNum; i++) {
        const bool has1 = has_tech(i, faction_id_1);
        const bool has2 = has_tech(i, faction_id_2);
        if (has1 && !has2) {
            // Fix: original version compared plr2.diplo_friction[plr2.tech_trade_source[i]]
            // instead of the symmetric condition plr1.diplo_friction[source]
            const int source = plr1.tech_trade_source[i];
            if ((!source || !(plr1.diplo_status[source] & DIPLO_PACT))
            && (!(plr1.diplo_status[source] & DIPLO_TREATY)
            || plr1.diplo_friction[source]
            >= ((plr1.diplo_status[faction_id_2] & DIPLO_PACT) ? 6 : 16))) {
                int value = mod_tech_val(i, faction_id_2, 0);
                int score = (value > 1 ? game_rand() % value : 0);
                if (score > score_to2) {
                    score_to2 = score;
                    tech_to2 = i;
                }
            }
        } else if (!has1 && has2) {
            const int source = plr2.tech_trade_source[i];
            if (!source || !(plr2.diplo_status[source] & DIPLO_PACT)) {
                if (!(plr2.diplo_status[source] & DIPLO_TREATY)
                || plr2.diplo_friction[source]
                >= ((plr1.diplo_status[faction_id_2] & DIPLO_PACT) ? 6 : 16)) {
                    int value = mod_tech_val(i, faction_id_1, 0);
                    int score = (value > 1 ? game_rand() % value : 0);
                    if (score > score_to1) {
                        score_to1 = score;
                        tech_to1 = i;
                    }
                }
            }
        }
    }
    if (tech_to1 >= 0 && tech_to2 >= 0) {
        tech_achieved(faction_id_1, tech_to1, faction_id_2, 0);
        tech_achieved(faction_id_2, tech_to2, faction_id_1, 0);
    } else if ((plr1.diplo_status[faction_id_2] & DIPLO_PACT) && great_satan(FactionRankings[7], 0)) {
        if (tech_to1 >= 0 && Factions[*RankingFactionIDUnk1].tech_ranking >= plr2.tech_ranking) {
            tech_achieved(faction_id_1, tech_to1, faction_id_2, 0);
        }
        if (tech_to2 >= 0 && Factions[*RankingFactionIDUnk1].tech_ranking >= plr1.tech_ranking) {
            tech_achieved(faction_id_2, tech_to2, faction_id_1, 0);
        }
    }
}

void __cdecl enemies_treaty(int faction_id_1, int faction_id_2, int trade_tech) {
    const int player_id = MapWin->cOwner;
    Faction& plr1 = Factions[faction_id_1];
    Faction& plr2 = Factions[faction_id_2];
    if (trade_tech) {
        enemies_trade_tech(faction_id_1, faction_id_2);
    }
    const uint32_t status12 = plr1.diplo_status[faction_id_2];
    if (!(status12 & DIPLO_TREATY) && (status12 & (DIPLO_VENDETTA|DIPLO_TRUCE))) {
        const int difficulty = ((*GameRules & RULES_INTENSE_RIVALRY) ? DIFF_TRANSCEND : *DiffLevel);
        if ((status12 & (DIPLO_WANT_REVENGE|DIPLO_UNK_40))
        || (plr2.diplo_status[faction_id_1] & (DIPLO_UNK_40|DIPLO_WANT_REVENGE))
        || (plr2.ranking >= 6 && plr1.ranking >= 6)
        || game_rand() % 16 < plr2.AI_fight + plr1.AI_fight - difficulty + 8) {
            if (Factions[player_id].diplo_status[faction_id_1] & DIPLO_PACT) {
                pact_truce(faction_id_1, faction_id_2);
            } else if (Factions[player_id].diplo_status[faction_id_2] & DIPLO_PACT) {
                pact_truce(faction_id_2, faction_id_1);
            } else if (player_spying(player_id, faction_id_1, faction_id_2)
            || (*GameState & STATE_OMNISCIENT_VIEW)) {
                parse_says(0, get_noun(faction_id_1), -1, -1);
                parse_says(1, get_noun(faction_id_2), -1, -1);
                NetMsg_pop(NetMsg, "ENEMYTRUCE", 5000, 0, 0);
            }
            treaty_on(faction_id_1, faction_id_2, DIPLO_TRUCE);
            return;
        }
    }
    if (!(status12 & DIPLO_TREATY)) {
        if (Factions[player_id].diplo_status[faction_id_1] & DIPLO_PACT) {
            pact_treaty(faction_id_1, faction_id_2);
        } else if (Factions[player_id].diplo_status[faction_id_2] & DIPLO_PACT) {
            pact_treaty(faction_id_2, faction_id_1);
        } else if (player_spying(player_id, faction_id_1, faction_id_2)
        || (*GameState & STATE_OMNISCIENT_VIEW)) {
            parse_says(0, get_noun(faction_id_1), -1, -1);
            parse_says(1, get_noun(faction_id_2), -1, -1);
            NetMsg_pop(NetMsg, "ENEMYTREATY", 5000, 0, 0);
        }
        treaty_on(faction_id_1, faction_id_2, DIPLO_TREATY);
        return;
    }
    if ((FactionRankings[7] == faction_id_1 && plr2.ranking >= 5)
    || (FactionRankings[7] == faction_id_2 && plr1.ranking >= 5)
    || (status12 & (DIPLO_ATROCITY_VICTIM|DIPLO_WANT_REVENGE))
    || (plr2.diplo_status[faction_id_1] & (DIPLO_ATROCITY_VICTIM|DIPLO_WANT_REVENGE))
    || plr1.major_atrocities
    || plr2.major_atrocities) {
        enemies_unpact(faction_id_1, faction_id_2);
        return;
    }
    int pact_count = 0;
    int faction_id_3;
    for (faction_id_3 = 1; faction_id_3 < MaxPlayerNum; faction_id_3++) {
        const uint32_t status13 = plr1.diplo_status[faction_id_3];
        const uint32_t status23 = plr2.diplo_status[faction_id_3];
        if (faction_id_3 != faction_id_1
        && faction_id_3 != faction_id_2
        && ((status13 & DIPLO_COMMLINK) || (status23 & DIPLO_COMMLINK))
        && (((status13 & DIPLO_COMMLINK) && (status23 & DIPLO_COMMLINK)) || great_satan(faction_id_3, 0))) {
            if ((status13 & DIPLO_PACT) || (status23 & DIPLO_PACT)) {
                if (Factions[faction_id_3].ranking >= plr1.ranking
                || Factions[faction_id_3].ranking >= plr2.ranking) {
                    pact_count++;
                }
                continue;
            }
            if (Factions[faction_id_3].diff_level
            && (great_satan(faction_id_3, 0)
            || ((status13 & (DIPLO_PACT|DIPLO_TREATY|DIPLO_TRUCE)) == 0
            && (status23 & (DIPLO_PACT|DIPLO_TREATY|DIPLO_TRUCE)) == 0
            && ((status12 & DIPLO_PACT)
            || ((status13 & DIPLO_VENDETTA)
            && (plr1.ranking < Factions[faction_id_3].ranking
            || plr2.ranking < Factions[faction_id_3].ranking)))))) {
                break;
            }
        }
    }
    if (faction_id_3 >= MaxPlayerNum) {
        enemies_unpact(faction_id_1, faction_id_2);
        return;
    }
    if (status12 & DIPLO_PACT) {
        return;
    }
    const int blemishes = (great_satan(faction_id_3, 0) ? Factions[faction_id_3].integrity_blemishes : 0);
    if (pact_count > blemishes) {
        enemies_unpact(faction_id_1, faction_id_2);
        return;
    }
    treaty_on(faction_id_1, faction_id_2, DIPLO_PACT);
    enemies_team_up(faction_id_1, faction_id_2, faction_id_3);
    enemies_team_up(faction_id_2, faction_id_1, faction_id_3);
    if (faction_id_3 == player_id || player_spying(player_id, faction_id_1, faction_id_2)) {
        parse_says(0, get_noun(faction_id_1), -1, -1);
        parse_says(1, get_noun(faction_id_2), -1, -1);
        parse_says(2, get_noun(faction_id_3), -1, -1);
        const char* suffix = "";
        if (faction_id_3 == player_id) {
            if (!(plr1.diplo_status[faction_id_3] & DIPLO_VENDETTA)
            && !(plr2.diplo_status[faction_id_3] & DIPLO_VENDETTA)) {
                suffix = "0";
            }
        } else {
            suffix = "1";
        }
        snprintf(StrBuffer, StrBufLen, "ENEMIESTEAMUP%s", suffix);
        NetMsg_pop(NetMsg, StrBuffer, 5000, 0, 0);
    }
}

void __cdecl comm_check(int faction_id_1, int faction_id_2) {
    const int player_id = MapWin->cOwner;
    if (!is_human(faction_id_1)) {
        return;
    }
    uint32_t status = Factions[faction_id_1].diplo_status[faction_id_2];
    bool has_commlink = status & DIPLO_COMMLINK;
    if (has_commlink && !(status & DIPLO_WANT_TO_TALK)) {
        int spoke = Factions[faction_id_1].diplo_spoke[faction_id_2];
        if (spoke >= 0 && *CurrentTurn - spoke < 16) {
            return;
        }
    }
    if (*MultiplayerActive) {
        if (faction_id_1 != player_id || is_human(faction_id_2)) {
            if (is_human(faction_id_2) && !has_commlink) {
                treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_10000|DIPLO_TRUCE);
            }
            if (faction_id_1 == player_id
            && !(Factions[faction_id_1].diplo_status[faction_id_2] & DIPLO_UNK_8000000)) {
                parse_says(0, get_title(faction_id_2), -1, -1);
                parse_says(1, get_name(faction_id_2), -1, -1);
                parse_says(2, get_noun(faction_id_2), -1, -1);
                NetMsg_pop(NetMsg, "NETMET", 5000, 0, 0);
            }
            if (faction_id_2 == player_id
            && !(Factions[faction_id_1].diplo_status[faction_id_2] & DIPLO_UNK_8000000)) {
                parse_says(0, get_title(faction_id_1), -1, -1);
                parse_says(1, get_name(faction_id_1), -1, -1);
                parse_says(2, get_noun(faction_id_1), -1, -1);
                NetMsg_pop(NetMsg, "NETMET", 5000, 0, 0);
            }
        } else {
            DiploStateA[faction_id_2] |= 1;
            if (!has_commlink) {
                DiploStateA[faction_id_2] |= 2;
                // Original: net_treaty_on(..., 0) (network message, no network play here)
                treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_10000|DIPLO_TRUCE);
            }
        }
        return;
    }
    if (!*PbemActive) {
        communicate(faction_id_1, faction_id_2, 0);
        return;
    }
    if (is_human(faction_id_2)) {
        if (!has_commlink) {
            treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_10000|DIPLO_TRUCE);
        }
        return;
    }
    if (faction_id_1 == player_id) {
        communicate(faction_id_1, faction_id_2, 0);
        return;
    }
    DiploStateB[faction_id_1][faction_id_2] |= 1;
    if (!has_commlink) {
        DiploStateB[faction_id_1][faction_id_2] |= 2;
        treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_10000|DIPLO_TRUCE);
    }
}

void __cdecl encounter(int faction_id_1, int faction_id_2, int x, int y, int fungal_tower, int native_unit) {
    // Fix: reject invalid factions instead of indexing out of range
    if (faction_id_1 < 0 || faction_id_1 >= MaxPlayerNum || faction_id_2 < 0
    || faction_id_2 >= MaxPlayerNum || faction_id_1 == faction_id_2) {
        return;
    }
    if (!faction_id_1 || !*CurrentTurn) {
        return;
    }
    if (!faction_id_2) {
        if (faction_id_1 == MapWin->cOwner && MFactions[faction_id_1].is_alien()) {
            if (fungal_tower) {
                wp8_interlude(44, 0, 1, 0);
            }
            if (native_unit) {
                wp8_interlude(43, 0, 1, 0);
            }
        }
        return;
    }
    if (MFactions[faction_id_1].is_alien() && MFactions[faction_id_2].is_alien()) {
        treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_80000000|DIPLO_COMMLINK);
        if (on_map(x, y)) {
            treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_40000000);
        }
        if (!_stricmp(MFactions[faction_id_1].search_key, "CARETAKE")) {
            if (faction_id_1 == MapWin->cOwner) {
                wp8_interlude(39, 0, 1, 0);
            }
            if (faction_id_2 == MapWin->cOwner) {
                wp8_interlude(40, 0, 1, 0);
            }
        } else {
            if (faction_id_1 == MapWin->cOwner) {
                wp8_interlude(40, 0, 1, 0);
            }
            if (faction_id_2 == MapWin->cOwner) {
                wp8_interlude(39, 0, 1, 0);
            }
        }
        return;
    }
    if (MFactions[faction_id_1].is_alien()) {
        if (!has_tech(TECH_PrPsych, faction_id_2) && !has_tech(TECH_Psych, faction_id_1)) {
            treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_80000000|DIPLO_COMMLINK);
            if (on_map(x, y)) {
                treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_40000000);
            }
            if (faction_id_1 == MapWin->cOwner) {
                wp8_interlude(34, 0, 1, 0);
            }
            if (faction_id_2 == MapWin->cOwner) {
                wp8_interlude(22, 0, 1, 0);
            }
            return;
        }
        if (faction_id_1 == MapWin->cOwner) {
            wp8_interlude(35, 0, 1, 0);
        }
        if (faction_id_2 == MapWin->cOwner) {
            wp8_interlude(23, 0, 1, 0);
        }
    }
    if (MFactions[faction_id_2].is_alien()) {
        if (!has_tech(TECH_PrPsych, faction_id_1) && !has_tech(TECH_Psych, faction_id_2)) {
            treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_80000000|DIPLO_COMMLINK);
            if (on_map(x, y)) {
                treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_40000000);
            }
            if (faction_id_1 == MapWin->cOwner) {
                wp8_interlude(22, 0, 1, 0);
            }
            if (faction_id_2 == MapWin->cOwner) {
                wp8_interlude(34, 0, 1, 0);
            }
            return;
        }
        if (faction_id_1 == MapWin->cOwner) {
            wp8_interlude(23, 0, 1, 0);
        }
        if (faction_id_2 == MapWin->cOwner) {
            wp8_interlude(35, 0, 1, 0);
        }
    }
    if (*SunspotDuration <= 0) {
        s_encounter_x = x;
        s_encounter_y = y;
        comm_check(faction_id_1, faction_id_2);
        comm_check(faction_id_2, faction_id_1);
        s_encounter_y = -1;
        s_encounter_x = -1;

        bool had_commlink = Factions[faction_id_1].diplo_status[faction_id_2] & DIPLO_COMMLINK;
        // Original: first-contact tutorial popups (ALMEETSGUY, GUYMEETSAL, ...) are UI only.
        treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_80000000|DIPLO_UNK_8000000|DIPLO_COMMLINK);
        if (on_map(x, y)) {
            treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_40000000);
        }
        if (!is_human(faction_id_1)
        && !is_human(faction_id_2)
        && (((faction_id_1 + faction_id_2 + *CurrentTurn) & 3) == 0 || !had_commlink)
        && (!MFactions[faction_id_1].is_alien() || !MFactions[faction_id_2].is_alien())) {
            if (!(Factions[faction_id_1].diplo_agenda[faction_id_2] & AGENDA_PERMANENT)
            || (MFactions[faction_id_1].is_alien() && MFactions[faction_id_2].is_alien())) {
                if (mod_wants_to_attack(faction_id_1, faction_id_2, 0)
                || mod_wants_to_attack(faction_id_2, faction_id_1, 0)) {
                    enemies_war(faction_id_1, faction_id_2);
                } else {
                    enemies_treaty(faction_id_1, faction_id_2, 1);
                }
            } else {
                enemies_trade_tech(faction_id_1, faction_id_2);
            }
        }
    } else {
        treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_80000000|DIPLO_COMMLINK);
        if (on_map(x, y)) {
            treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_40000000);
        }
    }
}

int __cdecl territory(int faction_id_1, int faction_id_2, int flags, int* out_base_id, int* out_count) {
    int cnt = 0;
    if (out_base_id) {
        *out_base_id = -1;
    }
    if (out_count) {
        *out_count = 0;
    }
    const uint32_t status = Factions[faction_id_1].diplo_status[faction_id_2];
    if (status & (DIPLO_VENDETTA|DIPLO_PACT)) {
        return 0;
    }
    if (!(flags & 2) && (status & DIPLO_UNK_200)) {
        return 0;
    }
    const int want_revenge = status & DIPLO_WANT_REVENGE;
    int skip_region[MaxRegionNum] = {}; // includes all land/ocean regions
    for (int i = 0; i < MaxRegionLandNum; i++) {
        if (!want_revenge && Factions[faction_id_2].integrity_blemishes < 5) {
            for (int j = 1; j < MaxPlayerNum; j++) {
                if (j != faction_id_1
                && j != faction_id_2
                && (Factions[faction_id_1].diplo_status[j] & DIPLO_VENDETTA)
                && (Factions[faction_id_2].diplo_status[j] & DIPLO_VENDETTA)
                && Factions[j].region_total_bases[i]) {
                    skip_region[i] = 1;
                    break;
                }
            }
        }
    }
    int base_id;
    for (int i = 0; i < *VehCount; i++) {
        VEH* veh = &Vehs[i];
        veh->state &= ~VSTATE_UNK_400;
        if (veh->faction_id != faction_id_2) {
            continue;
        }
        if ((uint8_t)veh->plan() <= PLAN_COLONY || (veh->plan() == PLAN_SUPPLY && veh->order == ORDER_CONVOY)) {
            MAP* sq = mapsq(veh->x, veh->y);
            if (sq && sq->alt_level() >= ALT_SHORE_LINE && !skip_region[sq->region]) {
                if (whose_territory(faction_id_2, veh->x, veh->y, &base_id, 1) == faction_id_1 && base_id >= 0) {
                    if (out_base_id) {
                        *out_base_id = base_id;
                    }
                    veh->state |= VSTATE_UNK_400;
                    cnt++;
                    if (sq->items & (BIT_SENSOR|BIT_THERMAL_BORE|BIT_CONDENSER|BIT_FOREST
                    |BIT_FARM|BIT_BUNKER|BIT_SOLAR|BIT_MINE)) {
                        cnt++;
                    }
                    if (flags & 1) {
                        if (map_range(Bases[base_id].x, Bases[base_id].y, veh->x, veh->y) <= 2
                        || (status & (DIPLO_UNK_800|DIPLO_SHALL_BETRAY|DIPLO_WANT_REVENGE))
                        || *DiploFriction > 12) {
                            if (out_count && (veh->flags & VFLAG_UNK_1)) {
                                ++*out_count;
                            }
                            veh->flags |= VFLAG_UNK_1|VFLAG_UNK_2;
                            if (!(veh->state & VSTATE_UNK_800) && *SunspotDuration <= 0) {
                                veh->state |= VSTATE_UNK_800;
                                continue;
                            }
                            cnt += ((status & DIPLO_TREATY) ? 5 : 1);
                        }
                    }
                }
            }
        }
    }
    if (cnt) {
        Factions[faction_id_1].player_flags |= PFLAG_UNK_10000;
    }
    return cnt;
}

void __cdecl enemy_diplomacy(int faction_id) {
    // Fix: reject invalid factions instead of indexing out of range
    if (faction_id <= 0 || faction_id >= MaxPlayerNum) {
        return;
    }
    Faction& plr = Factions[faction_id];
    for (int i = 1; i < MaxPlayerNum; i++) {
        plr.diplo_status[i] &= ~DIPLO_UNK_100;
        uint32_t agenda = plr.diplo_agenda[i];
        if (agenda & AGENDA_UNK_20) {
            if (agenda & AGENDA_UNK_40) {
                plr.diplo_agenda[i] = agenda & ~AGENDA_UNK_20;
                Factions[i].diplo_agenda[faction_id] &= ~AGENDA_UNK_20;
            } else {
                plr.diplo_agenda[i] = agenda | AGENDA_UNK_40;
                Factions[i].diplo_agenda[faction_id] |= AGENDA_UNK_40;
            }
        }
    }
    if (is_human(faction_id)) {
        return;
    }
    int vendetta_count = 0;
    int base_id = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (faction_id == i || !is_alive(i)) {
            continue;
        }
        if (plr.diplo_status[i] & DIPLO_VENDETTA) {
            vendetta_count++;
        }
        if (plr.diplo_merc[i]) {
            plr.diplo_merc[i]--;
            if (!plr.diplo_merc[i]) {
                plr.diplo_status[i] &= ~DIPLO_UNK_40;
            }
        }
        if ((*CurrentTurn & 1) && plr.diplo_patience[i] > 0) {
            plr.diplo_patience[i]--;
        }
        if (plr.diplo_status[i] & DIPLO_SHALL_BETRAY) {
            int betray_chance = clamp(8 - ((plr.diplo_agenda[i] & AGENDA_UNK_400) ? 2 : 0)
                - Factions[i].integrity_blemishes, 1, 8);
            if (betray_chance <= 1 || !(game_rand() % betray_chance) || (plr.diplo_status[i] & DIPLO_PACT)) {
                if (plr.diplo_status[i] & DIPLO_PACT) {
                    cause_friction(faction_id, i, 20);
                    plr.diplo_status[i] &= ~(DIPLO_UNK_20000 | DIPLO_UNK_800 | DIPLO_SHALL_BETRAY);
                    treaty_on(faction_id, i, DIPLO_WANT_TO_TALK);
                } else if (Factions[i].target_x >= 0) {
                    plr.diplo_status[i] |= DIPLO_UNK_20000;
                } else {
                    if (Factions[i].diplo_spoke[faction_id] < *CurrentTurn - 8) {
                        Factions[i].diplo_spoke[faction_id] = *CurrentTurn - 8;
                    }
                    plr.diplo_status[i] = (plr.diplo_status[i] & ~(DIPLO_UNK_20000 | DIPLO_SHALL_BETRAY))
                        | DIPLO_UNK_800 | DIPLO_UNK_40;
                    plr.diplo_merc[i] = 50;
                }
            }
        }
        if ((*CurrentTurn & 0xF) == 2 * faction_id + 1) {
            plr.diplo_status[i] &= ~DIPLO_UNK_8000;
            Factions[i].diplo_status[faction_id] &= ~DIPLO_UNK_8000;
        }
        if (!is_human(i) || *SunspotDuration > 0) {
            if (faction_id > i && *SunspotDuration <= 0
            && (((faction_id + i + *CurrentTurn) & 7) == 0 || (plr.diplo_status[i] & DIPLO_WANT_TO_TALK))) {
                if (!(plr.diplo_status[i] & DIPLO_UNK_80000000) || (plr.diplo_status[i] & DIPLO_WANT_TO_TALK)) {
                    plr.diplo_status[i] &= ~DIPLO_WANT_TO_TALK;
                    Factions[i].diplo_status[faction_id] &= ~DIPLO_WANT_TO_TALK;
                    if (plr.diplo_status[i] & DIPLO_PACT) {
                        enemies_trade_tech(faction_id, i);
                    } else if (((plr.diplo_status[i] & DIPLO_TREATY) && (*GameState & STATE_UNK_200))
                    || has_project(FAC_EMPATH_GUILD, faction_id)
                    || has_project(FAC_EMPATH_GUILD, i)
                    || (plr.diplo_status[i] & DIPLO_UNK_20000000)
                    || ((plr.diplo_status[i] & DIPLO_COMMLINK) && (plr.diplo_status[i] & DIPLO_UNK_10000000))) {
                        encounter(faction_id, i, -1, -1, 0, 0);
                    }
                    treaty_on(faction_id, i, DIPLO_UNK_80000000);
                    plr.diplo_status[i] &= ~DIPLO_UNK_4000000;
                    Factions[i].diplo_status[faction_id] &= ~DIPLO_UNK_4000000;
                } else {
                    plr.diplo_status[i] &= ~DIPLO_UNK_80000000;
                    Factions[i].diplo_status[faction_id] &= ~DIPLO_UNK_80000000;
                    for (int j = 1; j < MaxPlayerNum; j++) {
                        if (is_human(j) && ((plr.diplo_status[j] & DIPLO_VENDETTA)
                        || (Factions[i].diplo_status[j] & DIPLO_VENDETTA))) {
                            plr.diplo_status[i] &= ~DIPLO_UNK_4000000;
                            Factions[i].diplo_status[faction_id] &= ~DIPLO_UNK_4000000;
                        }
                    }
                }
            }
            continue;
        }
        if (i == MapWin->cOwner
        && (plr.diplo_status[i] & (DIPLO_ATROCITY_VICTIM | DIPLO_VENDETTA | DIPLO_COMMLINK)) == DIPLO_COMMLINK) {
            int j;
            for (j = 1; j < 8; j++) {
                if (is_alive(j) && j != faction_id && j != i
                && (plr.diplo_status[j] & (DIPLO_UNK_8000 | DIPLO_VENDETTA | DIPLO_COMMLINK))
                    == (DIPLO_UNK_8000 | DIPLO_VENDETTA | DIPLO_COMMLINK)
                && (Factions[i].diplo_status[j] & (DIPLO_VENDETTA | DIPLO_COMMLINK)) == DIPLO_COMMLINK
                && Factions[j].mil_strength_1 > plr.mil_strength_1) {
                    break;
                }
            }
            if (j < 8 && !(plr.diplo_status[i] & DIPLO_PACT) && reputation(i, faction_id) < 5
            && ((*CurrentTurn - Factions[i].diplo_spoke[faction_id] > 8 && (*CurrentTurn & 0xF) == 2 * faction_id)
            || (plr.diplo_status[i] & DIPLO_WANT_TO_TALK))) {
                DiploStateA[faction_id] |= 0x41;
                DiploStateB[i][faction_id] |= 0x41;
                if (!*MultiplayerActive && !*PbemActive) {
                    communicate(i, faction_id, 0);
                    treaty_on(i, faction_id, DIPLO_UNK_80000000 | DIPLO_UNK_8000000 | DIPLO_COMMLINK);
                }
            } else if (!(plr.diplo_status[i] & DIPLO_WANT_REVENGE)
            && (*CurrentTurn - Factions[i].diplo_spoke[faction_id] > 32
            || ((plr.diplo_status[i] & DIPLO_UNK_20000000) && *CurrentTurn == faction_id + 8)
            || ((plr.diplo_status[i] & DIPLO_PACT) && (plr.diplo_status[i] & DIPLO_WANT_TO_TALK)))
            && (!game_random(0, 16) || (plr.diplo_status[i] & DIPLO_WANT_TO_TALK) || *CurrentTurn == faction_id + 8)) {
                if (!*MultiplayerActive && !*PbemActive) {
                    communicate(i, faction_id, 0);
                    treaty_on(i, faction_id, DIPLO_UNK_80000000 | DIPLO_UNK_8000000 | DIPLO_COMMLINK);
                } else {
                    DiploStateA[faction_id] |= 1;
                    DiploStateB[i][faction_id] |= 1;
                }
            }
        }
        if (*PbemActive ? !DiploStateB[i][faction_id] : !DiploStateA[faction_id]) {
            diplomacy_check(i, faction_id, 0);
            if (*DiploAttackChance) {
                if ((plr.diplo_status[i] & DIPLO_WANT_REVENGE)
                || (plr.player_flags_ext & PFLAG_EXT_SHAMELESS_BETRAY_HUMANS)
                || (Factions[i].diff_level > DIFF_THINKER && (*GameRules & RULES_INTENSE_RIVALRY))) {
                    plr.diplo_status[i] = (plr.diplo_status[i] & ~(DIPLO_UNK_20000 | DIPLO_SHALL_BETRAY))
                        | DIPLO_UNK_800 | DIPLO_UNK_40;
                    plr.diplo_merc[i] = 50;
                }
            }
        }
    }
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (!is_human(i) || !(plr.diplo_status[i] & DIPLO_COMMLINK)) {
            continue;
        }
        int territory_result = 0;
        int territory_aux = 0;
        if (*CurrentTurn - Factions[i].diplo_spoke[faction_id] >= 4
        || *SunspotDuration > 0
        || (plr.diplo_status[i] & DIPLO_WANT_REVENGE)) {
            territory_result = territory(faction_id, i, 1, &base_id, &territory_aux);
            if (territory_result && i == MapWin->cOwner && !(plr.diplo_status[i] & DIPLO_VENDETTA)) {
                if (*SunspotDuration <= 0) {
                    uint32_t agenda = plr.diplo_agenda[i];
                    if (plr.diplo_status[i] & DIPLO_TREATY) {
                        if (territory_result >= 5) {
                            DiploStateA[faction_id] |= 4;
                            DiploStateB[i][faction_id] |= 4;
                        } else if (((faction_id + *CurrentTurn) & 3) == 0) {
                            DiploStateA[faction_id] |= 32;
                            DiploStateB[i][faction_id] |= 32;
                        }
                    } else if ((agenda & AGENDA_UNK_800) || ((agenda & AGENDA_UNK_400) && territory_aux)) {
                        DiploStateA[faction_id] |= 24;
                        DiploStateB[i][faction_id] |= 24;
                    } else {
                        uint32_t want_revenge = plr.diplo_status[i] & DIPLO_WANT_REVENGE;
                        bool escalate = false;
                        if (!want_revenge || territory_aux) {
                            if (((faction_id + *CurrentTurn) & 3) == 0) {
                                DiploStateA[faction_id] |= 8;
                                DiploStateB[i][faction_id] |= 8;
                                escalate = game_random(0, territory_aux + Factions[i].integrity_blemishes
                                    + territory_result + 4 * (territory_aux + Factions[i].integrity_blemishes)
                                    + (want_revenge ? 5 : 0)) > 10;
                            }
                        } else {
                            DiploStateA[faction_id] |= 8;
                            DiploStateB[i][faction_id] |= 8;
                            escalate = game_random(0, territory_result
                                + 4 * (Factions[i].integrity_blemishes + 1)
                                + Factions[i].integrity_blemishes + 1) > 10;
                        }
                        if (escalate) {
                            if (i == MapWin->cOwner) {
                                DiploStateA[faction_id] |= 0x10;
                            }
                            DiploStateB[i][faction_id] |= 0x10;
                        }
                    }
                    if (DiploStateA[faction_id] && !*MultiplayerActive && !*PbemActive) {
                        communicate(i, faction_id, 0);
                        treaty_on(i, faction_id, DIPLO_UNK_80000000 | DIPLO_UNK_8000000 | DIPLO_COMMLINK);
                    }
                } else {
                    diplomacy_check(i, faction_id, 0);
                    if (*DiploHostility
                    || (plr.diplo_status[i] & DIPLO_WANT_REVENGE)
                    || territory_result + *DiploFriction + 2 * Factions[i].integrity_blemishes
                    >= 2 * (9 - plr.AI_fight)) {
                        if (plr.diplo_status[i] & DIPLO_TREATY) {
                            plr.diplo_status[i] |= DIPLO_UNK_800;
                        } else if (!(plr.diplo_status[i] & (DIPLO_SHALL_BETRAY | DIPLO_UNK_800))) {
                            if (plr.diplo_status[i] & DIPLO_WANT_REVENGE) {
                                plr.diplo_status[i] |= DIPLO_UNK_800;
                            } else {
                                plr.diplo_status[i] |= DIPLO_SHALL_BETRAY;
                            }
                        }
                    }
                }
            }
        }
        int friction = ((*CurrentTurn & 1) + territory_result + 3) / 5;
        if (plr.diplo_status[i] & DIPLO_PACT) {
            if (vendetta_count) {
                friction += 1 - vendetta_count;
            } else {
                int our_tech = plr.tech_ranking / 2;
                int their_tech = Factions[i].tech_ranking / 2;
                if (their_tech > plr.AI_tech - plr.AI_power - plr.AI_fight + our_tech + 5) {
                    friction++;
                }
                if (our_tech > their_tech + 5) {
                    friction--;
                }
                if (Factions[i].ranking > plr.ranking + 1
                && Factions[i].diff_level != DIFF_CITIZEN) {
                    friction++;
                }
            }
            if (!(plr.diplo_status[i] & DIPLO_UNK_1000000)) {
                for (int j = 1; j < MaxPlayerNum; j++) {
                    if ((plr.diplo_status[j] & DIPLO_VENDETTA)
                    && (Factions[i].diplo_status[j] & DIPLO_COMMLINK)
                    && !(Factions[i].diplo_status[j] & DIPLO_VENDETTA)) {
                        friction++;
                    }
                }
            }
            if ((*GameRules & RULES_VICTORY_COOPERATIVE) && plr.ranking < 6
            && *CurrentTurn >= 200 && friction >= 0) {
                friction = -1;
            }
        } else {
            if ((*GameRules & RULES_INTENSE_RIVALRY) && (*CurrentTurn & 1)) {
                friction++;
            }
            int their_strength = Factions[i].mil_strength_1;
            int our_strength = plr.mil_strength_1;
            int their_scale = clamp(3 * their_strength, 1, 99999);
            friction += 2 * our_strength / their_scale;
            int our_scale = clamp(3 * our_strength, 1, 99999);
            friction += -2 * their_strength / our_scale;
            if (great_satan(i, 0)) {
                friction++;
            }
            if (aah_ooga(i, -1)) {
                friction++;
            }
        }
        if (*CurrentTurn & 1) {
            int soc_category = MFactions[faction_id].soc_priority_category;
            // Fix: bound the category, it indexes the four SE_*_pending fields
            if (soc_category >= 0 && soc_category < 4) {
                int soc_model = MFactions[faction_id].soc_priority_model;
                if (soc_model) {
                    int their_model = (&Factions[i].SE_Politics_pending)[soc_category];
                    if (their_model == soc_model) {
                        friction--;
                    } else if (their_model) {
                        friction++;
                    }
                }
            }
        }
        friction += plr.AI_fight;
        if ((*CurrentTurn & 3) == 0) {
            if (plr.diplo_status[i] & DIPLO_TREATY) {
                friction--;
            }
            int our_rank = plr.ranking;
            int their_rank = Factions[i].ranking;
            if (our_rank >= 6 && their_rank >= 6) {
                friction += 3;
            } else if (our_rank < 5 || their_rank < 5) {
                int rank_diff = their_rank - our_rank;
                int rank_sign = (rank_diff > 0) - (rank_diff < 0);
                friction += 3 * rank_sign - (rank_sign + rank_diff) / 2;
            } else if (*CurrentTurn >= 100) {
                friction++;
            }
            if (Factions[i].energy_credits > plr.energy_credits
            && !strcmp(MFactions[faction_id].filename, "MORGAN")) {
                friction++;
            }
            if (Factions[i].tech_ranking > plr.tech_ranking) {
                friction += plr.AI_power - plr.AI_tech + 1;
            }
        }
        if ((plr.diplo_status[i] & DIPLO_PACT) && (*GameRules & RULES_VICTORY_COOPERATIVE)
        && Factions[i].ranking > plr.ranking && *CurrentTurn >= 200) {
            if (friction > 0) {
                friction = 0;
            }
        }
        plr.diplo_status[i] &= ~DIPLO_UNK_200;
        Factions[i].diplo_status[faction_id] &= ~DIPLO_UNK_200;
        if (!(plr.diplo_status[i] & DIPLO_VENDETTA) || friction >= 0) {
            cause_friction(faction_id, i, friction);
        }
    }
    int max_busters = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (i == faction_id) {
            continue;
        }
        uint32_t status = plr.diplo_status[i];
        if (status & DIPLO_PACT) {
            continue;
        }
        int busters = Factions[i].planet_busters;
        if ((status & DIPLO_TREATY) && !(status & DIPLO_WANT_REVENGE)) {
            busters /= 2;
        }
        if (max_busters <= busters) {
            max_busters = busters;
        }
    }
    while (plr.satellites_ODP - plr.ODP_deployed > max_busters) {
        int sat_type = -1;
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (!(plr.diplo_status[i] & (DIPLO_VENDETTA | DIPLO_UNK_800))) {
                continue;
            }
            int32_t their_sats = Factions[i].satellites_ODP;
            if (plr.satellites_ODP <= their_sats) {
                continue;
            }
            if (their_sats && their_sats >= plr.planet_busters) {
                sat_type = 3;
            } else {
                int geosync_count = 0;
                for (base_id = 0; base_id < *BaseCount; base_id++) {
                    if (Bases[base_id].faction_id == i && Bases[base_id].has_fac_built(FAC_GEOSYNC_SURVEY_POD)) {
                        geosync_count++;
                    }
                }
                if (Factions[i].satellites_mineral) {
                    sat_type = 1;
                } else if (Factions[i].satellites_energy) {
                    sat_type = 2;
                } else if (Factions[i].satellites_nutrient) {
                    sat_type = 0;
                } else if (geosync_count) {
                    sat_type = 4;
                    for (base_id = 0; base_id < *BaseCount; base_id++) {
                        if (Bases[base_id].faction_id == i && Bases[base_id].has_fac_built(FAC_GEOSYNC_SURVEY_POD)) {
                            break;
                        }
                    }
                } else if (their_sats) {
                    sat_type = 3;
                } else {
                    continue;
                }
            }
            action_sat_attack(faction_id, i, sat_type, base_id);
            break;
        }
        if (sat_type < 0) {
            break;
        }
    }
}

/* ---- faction_diplo.cpp ---- */

/* Original: energy_compute(faction_id, 0) (exe 0x5C1890, in no upstream source) fills
 * FactionCurrentNetIncome for the energy report. Fix: use the faction's energy surplus
 * that production_phase totals, which is the same net income without the window. */
static void energy_compute_net(int faction_id) {
    *FactionCurrentNetIncome = Factions[faction_id].energy_surplus_total;
}

void __cdecl diplomacy_check(int faction_id, int faction_id_2, int full_check) {
    // Fix: reject invalid factions instead of indexing out of range
    if (faction_id <= 0 || faction_id >= MaxPlayerNum || faction_id_2 <= 0 || faction_id_2 >= MaxPlayerNum) {
        return;
    }
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];

    *DiploFrictionFactionIDWith = faction_id;
    *DiploFrictionFactionID = faction_id_2;
    *DiploSecondFaction = faction_id_2;
    *DiploHostility = 0;
    *DiploAttackChance = 0;
    *DiploTributeDemand = 0;
    *DiploFearScale = 0;
    *DiploConquestFear = 0;
    *DiploExposedBaseCount = 0;
    *DiploOpenBaseID = 0; // default value excludes the first base
    *DiploCapturedBases = 0;
    *DiploNearEndgame = at_climax(faction_id);
    *DiploRivalOutranks = cmp.ranking > plr.ranking;
    *DiploGreatSatan = great_satan(faction_id, 0)
        && cmp.base_count > 1
        && (!(plr.diplo_status[faction_id_2] & DIPLO_PACT)
        || (cmp.ranking >= 5 && (!(*GameRules & RULES_VICTORY_COOPERATIVE) || *CurrentTurn < 200)));
    *DiploRunawayLeader = FactionRankingsUnk[FactionRankings[7]] > 2 * FactionRankingsUnk[faction_id]
        && cmp.ranking != 7;

    if (cmp.player_flags & PFLAG_TEAM_UP_VS_HUMAN) {
        *DiploGreatSatan = 1;
        *DiploRunawayLeader = 0;
    }
    if (cmp.player_flags & PFLAG_COOP_WITH_HUMAN) {
        *DiploGreatSatan = 0;
        *DiploRunawayLeader = 1;
    }
    // modified effects are only applied on lines referencing conf options
    *DiploFriction = cmp.diplo_friction[faction_id];
    *DiploFriction += conf.rival_friction_bias;
    *DiploReputation = reputation(faction_id, faction_id_2);

    int soc_conflict = 0;
    int soc_category = MFactions[faction_id_2].soc_priority_category;
    // Fix: bound the category, it indexes the four SE_*_pending fields
    if (soc_category >= 0 && soc_category < 4) {
        int soc_model = MFactions[faction_id_2].soc_priority_model;
        if (soc_model) {
            int current_model = (&plr.SE_Politics_pending)[soc_category];
            if (current_model && current_model != soc_model) {
                soc_conflict = 1;
                if (cmp.diplo_agenda[faction_id] & AGENDA_UNK_1) {
                    soc_conflict = 2;
                }
            }
        }
    }
    int our_combat = plr.diplo_combat_count[faction_id_2] + *CurrentTurn / 50 + 1;
    int their_combat = cmp.diplo_combat_count[faction_id] + *CurrentTurn / 50 + 1;
    if (!(plr.diplo_status[faction_id_2] & DIPLO_VENDETTA)) {
        our_combat += *CurrentTurn / 25 + plr.diplo_combat_total[faction_id_2] + 2;
        their_combat += *CurrentTurn / 25 + cmp.diplo_combat_total[faction_id] + 2;
    }
    int ai_fight = cmp.AI_fight;
    *DiploPlayerPactCount = 0;
    *DiploRivalPactCount = 0;
    *DiploRivalStrongPacts = 0;
    *DiploRivalPactScore = 0;
    if ((cmp.diplo_status[faction_id] & DIPLO_WANT_REVENGE) && ai_fight < 0) {
        ai_fight = -ai_fight;
    }
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (is_alive(i)) {
            if ((plr.diplo_status[i] & DIPLO_PACT) && !(Factions[i].diplo_status[faction_id] & DIPLO_HAVE_SURRENDERED)) {
                ++*DiploPlayerPactCount;
            }
            if (cmp.diplo_status[i] & DIPLO_PACT) {
                if (Factions[i].ranking >= 5 && cmp.ranking < 5 && i != faction_id && !*DiploRunawayLeader) {
                    ++*DiploRivalStrongPacts;
                }
                ++*DiploRivalPactCount;
                *DiploRivalPactScore += FactionRankingsUnk[i];
            }
        }
    }
    if (full_check) {
        *DiploSpokeTurn = plr.diplo_spoke[faction_id_2];
    }
    int our_morale = MFactions[faction_id].rule_morale + clamp(plr.SE_morale_pending, -4, 4) + 16;
    int their_morale = MFactions[faction_id_2].rule_morale + clamp(cmp.SE_morale_pending, -4, 4) + 16;

    int max_region_bases[7];
    for (int i = 0; i < 7; i++) {
        max_region_bases[i] = 1;
    }
    memset(&DiploRegionFactions[1], 0, 127 * sizeof(int32_t));
    for (int region = 1; region < MaxRegionNum; region++) {
        for (int i = 1; i < 8; i++) {
            int bases = Factions[i].region_total_bases[region];
            if (bases > max_region_bases[i - 1]) {
                max_region_bases[i - 1] = bases;
            }
        }
    }
    for (int base_id = 0; base_id < *BaseCount; base_id++) {
        BASE& base = Bases[base_id];
        if (has_fac_built(FAC_HEADQUARTERS, base_id)) {
            DiploRegionFactions[region_at(base.x, base.y)] |= 1 << base.faction_id;
        }
        if (base.faction_id == faction_id) {
            if (base.faction_id_former == faction_id_2) {
                ++*DiploCapturedBases;
            }
        } else if (base.faction_id != faction_id_2) {
            continue;
        }
        if (!(plr.diplo_status[faction_id_2] & DIPLO_PACT)) {
            MAP* sq = mapsq(base.x, base.y);
            if (!sq) {
                continue; // Fix: base off the map
            }
            if (sq->veh_who() < 0) {
                for (int i = 0; i < 21; i++) {
                    int x = wrap(base.x + TableOffsetX[i]);
                    int y = base.y + TableOffsetY[i];
                    if (on_map(x, y)) {
                        MAP* adj = mapsq(x, y);
                        if (adj->region == sq->region) {
                            if (base.faction_id == faction_id) {
                                if (adj->veh_who() == faction_id_2) {
                                    ++*DiploExposedBaseCount;
                                    break;
                                }
                            } else if (adj->veh_who() == faction_id) {
                                *DiploOpenBaseID = base_id;
                                *DiploFearScale += 8 * base.pop_size;
                                break;
                            }
                        }
                    }
                }
            } else if (base.faction_id == faction_id) {
                int stack_total = stack_check(veh_at(base.x, base.y), 14, -1, -1, -1);
                int adjacent_total = 0;
                for (int i = 0; i < 21; i++) {
                    int x = wrap(base.x + TableOffsetX[i]);
                    int y = base.y + TableOffsetY[i];
                    if (on_map(x, y)) {
                        int veh_id = stack_fix(veh_at(x, y));
                        // Fix: added bounds checking
                        if (veh_id >= 0 && Vehs[veh_id].faction_id == faction_id_2) {
                            int combat_total = stack_check(veh_id, 2, PLAN_COMBAT, -1, -1);
                            int offense_total = stack_check(veh_id, 2, PLAN_OFFENSE, -1, -1);
                            adjacent_total += combat_total + offense_total;
                        }
                    }
                }
                if (adjacent_total > stack_total) {
                    ++*DiploAttackChance;
                    ++*DiploTributeDemand;
                }
            }
        }
    }
    for (int region = 1; region < MaxRegionNum; region++) {
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (Factions[i].region_total_bases[region] >= max_region_bases[i - 1] - max_region_bases[i - 1] / 4) {
                DiploRegionFactions[region] |= 1 << i;
            }
        }
        int our_bases = plr.region_total_bases[region];
        int their_bases = cmp.region_total_bases[region];
        int our_units = plr.region_total_combat_units[region];
        int their_units = cmp.region_total_combat_units[region];
        if ((our_units || our_bases) && (their_units || their_bases)) {
            int our_weapon = plr.best_weapon_value + 1;
            int their_weapon = cmp.best_weapon_value + 1;
            int our_force = our_weapon * (our_morale * plr.region_force_rating[region] / their_morale) / their_weapon;
            int their_force = their_weapon * (their_morale * cmp.region_force_rating[region] / our_morale) / our_weapon;
            int reg_factions = DiploRegionFactions[region];
            int reg_has_other = reg_factions & (1 << faction_id_2);
            int their_total = their_units;
            if (reg_has_other && our_bases) {
                // Fix: guard the divisor (AI_fight 2 with this faction absent gives 0)
                int divisor = ((reg_factions & (1 << faction_id)) == 0) - cmp.AI_fight + 2;
                their_total += their_force / (divisor ? divisor : 1);
            }
            if (reg_has_other && our_force > their_total) {
                *DiploFearScale += 32 * (our_force + 1) / clamp(their_total, 1, 99999);
            } else {
                *DiploHostility = 1;
                if (our_bases) {
                    *DiploTributeDemand += their_force;
                    if ((reg_factions & (1 << faction_id)) && their_force > our_units) {
                        *DiploTributeDemand += their_force;
                        if (their_force > 3 * (our_units + our_force) / 2) {
                            ++*DiploAttackChance;
                        }
                    }
                    if (their_bases || (plr.diplo_status[faction_id_2] & DIPLO_PACT)) {
                        *DiploTributeDemand -= our_force;
                    }
                } else if (their_bases) {
                    *DiploTributeDemand -= (our_force - their_units) / (2 - (reg_has_other != 0));
                }
            }
        }
    }
    *DiploTributeDemand *= *DiploExposedBaseCount + 1;
    if (*DiploExposedBaseCount && !(plr.diplo_status[faction_id_2] & (DIPLO_TRUCE | DIPLO_TREATY | DIPLO_PACT))) {
        *DiploHostility = 1;
    }
    if (*DiploNearEndgame && *DiploRivalOutranks) {
        *DiploHostility = 3 - (*DiploGreatSatan != 0);
    }
    uint32_t& their_status = cmp.diplo_status[faction_id];
    uint32_t& their_agenda = cmp.diplo_agenda[faction_id];
    if (plr.base_count && cmp.base_count) {
        if ((their_status & DIPLO_WANT_REVENGE) || (cmp.diplo_agenda[faction_id] & AGENDA_UNK_200)) {
            int bonus = clamp(ai_fight + plr.tech_commerce_bonus - cmp.tech_commerce_bonus, 0, 99);
            *DiploTributeDemand = 2 * (*DiploTributeDemand + bonus * cmp.energy_surplus_total / cmp.base_count);
        } else {
            int bonus = clamp(cmp.tech_commerce_bonus + cmp.AI_wealth - plr.tech_commerce_bonus - ai_fight, 0, 99);
            *DiploTributeDemand -= bonus * (plr.energy_surplus_total / plr.base_count);
            if (their_status & DIPLO_WANT_TO_TALK) {
                *DiploTributeDemand /= 2;
            }
        }
    }
    if (plr.best_weapon_value >= 3 * cmp.best_weapon_value / 2 || plr.best_weapon_value > 2 * cmp.best_armor_value) {
        *DiploFearScale *= 2;
    }
    if (our_combat >= 2 * their_combat) {
        *DiploFearScale *= 2;
    }
    if (our_combat >= 4 * their_combat && !(their_status & DIPLO_WANT_REVENGE) && *DiploFearScale <= 8) {
        *DiploFearScale = 8;
    }
    if (2 * our_combat < their_combat) {
        *DiploFearScale /= 2;
    }
    if (4 * our_combat < their_combat) {
        *DiploFearScale /= 2;
    }
    if (*DiploFearScale
        || (plr.diff_level <= ((their_status & DIPLO_WANT_REVENGE) ? 2 : 4) && !(*GameRules & RULES_INTENSE_RIVALRY))) {
        *DiploAttackChance = 0;
    }
    *DiploTributeDemand = *DiploTributeDemand * (*CurrentTurn / 10 + their_combat) / (*CurrentTurn / 10 + our_combat);

    int turn_threshold = *MultiplayerActive ? 0 : 25 * (5 - plr.diff_level);
    if (*CurrentTurn >= turn_threshold + 50 && !*DiploRunawayLeader && plr.ranking >= (*DiffLevel < 3) + 5) {
        if (cmp.ranking >= (plr.ranking < 6) + 5) {
            *DiploGreatSatan = 1;
        } else if (*DiploRivalStrongPacts && (plr.diplo_status[faction_id_2] & DIPLO_UNK_8000000) && plr.ranking >= 6) {
            *DiploGreatSatan = 1;
        }
    }
    if (*DiploGreatSatan
    || !((!(their_status & DIPLO_WANT_REVENGE) && !(cmp.diplo_agenda[faction_id] & AGENDA_UNK_200))
    || *DiploOpenBaseID
    || (*DiploFearScale && 2 * cmp.mil_strength_1 < plr.mil_strength_1))) {
        *DiploFearScale = 0;
        *DiploHostility = (*DiploGreatSatan != 0) + 1;
        if (*DiploTributeDemand < 2 * cmp.ranking) {
            *DiploTributeDemand = 2 * cmp.ranking;
        }
    }
    if (cmp.planet_busters && cmp.major_atrocities) {
        *DiploFearScale = 0;
    }
    if (plr.planet_busters && plr.major_atrocities) {
        if (!cmp.planet_busters) {
            *DiploFearScale = 32;
        }
        *DiploTributeDemand = *DiploTributeDemand * cmp.planet_busters / plr.planet_busters;
    }
    *DiploFearScale -= *DiploFearScale * conf.rival_defiance_bias / 20;
    *DiploConquestFear = *DiploFearScale;
    if ((their_status & DIPLO_PACT) || !*DiploCapturedBases) {
        *DiploConquestFear = 0;
    }
    int diff_level = (*GameRules & RULES_INTENSE_RIVALRY) ? 5 : plr.diff_level;
    int blemishes = clamp(plr.integrity_blemishes / 2, 1, 99);
    *DiploFearScale /= (blemishes + 1) * (diff_level + 3) / 2;
    *DiploEnemyAllyCount = 0;
    *DiploEnemyAlly = 0;
    if (!(their_status & DIPLO_PACT) || !(their_status & DIPLO_HAVE_SURRENDERED)) {
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (is_alive(i)
            && i != faction_id
            && i != faction_id_2
            && (cmp.diplo_status[i] & DIPLO_VENDETTA)
            && (!(Factions[i].diplo_status[faction_id] & DIPLO_PACT)
            || !(Factions[i].diplo_status[faction_id] & DIPLO_HAVE_SURRENDERED))
            && ((plr.diplo_status[i] & DIPLO_PACT)
            || ((plr.diplo_status[i] & DIPLO_TREATY) && ai_fight > 0))) {
                *DiploEnemyAlly = i;
                ++*DiploEnemyAllyCount;
            }
        }
    }
    *DiploFriction += 3 * *DiploEnemyAllyCount;
    if (their_status & DIPLO_PACT) {
        *DiploFriction -= 5;
    }
    if (their_status & (DIPLO_WANT_REVENGE | DIPLO_VENDETTA)) {
        *DiploFriction += 5;
    }
    if (their_agenda & AGENDA_UNK_400) {
        *DiploFriction += 3;
    }
    if (their_agenda & AGENDA_UNK_800) {
        *DiploFriction += 3;
    }
    if (*DiploReputation > 1) {
        *DiploFriction += *DiploReputation - 1;
    }
    if (plr.tech_commerce_bonus < cmp.tech_commerce_bonus) {
        *DiploFriction += 2 * (plr.tech_commerce_bonus - cmp.tech_commerce_bonus);
    } else if (plr.tech_commerce_bonus > cmp.tech_commerce_bonus) {
        *DiploFriction += plr.tech_commerce_bonus - cmp.tech_commerce_bonus;
    }
    if (soc_conflict) {
        *DiploFriction += 5;
        if (*DiploFriction < 5) {
            *DiploFriction = 5;
        }
    }
    if (soc_conflict > 1 && !*DiploFearScale) {
        *DiploFriction += 5;
    }
    if (their_agenda & AGENDA_UNK_200) {
        *DiploHostility = 1;
    }
    if (*DiploFriction <= 4) {
        *DiploHostility = 0;
    }
    if (*DiploFriction >= 15) {
        *DiploHostility = 4;
    }
    if (their_status & DIPLO_UNK_40) {
        *DiploHostility = 1;
    }
    if (!(*GameRules & RULES_VICTORY_TRANSCENDENCE) && !cmp.SE_alloc_labs) {
        *DiploHostility = 1;
    }
    if (*DiploFearScale || plr.mil_strength_1 > 3 * cmp.mil_strength_1) {
        *DiploHostility = 0;
    }
    if (plr.best_weapon_value >= 2 * cmp.best_weapon_value) {
        *DiploHostility = 0;
    }
    int value = (*DiploTributeDemand * (soc_conflict + 1)
        * (*DiploFriction + 10) * (*DiploReputation + 2) * (diff_level + 1)) >> 6;
    if (their_status & DIPLO_UNK_40) {
        value *= 2;
    }
    int energy_credits = plr.energy_credits;
    if (energy_credits >= 2 * cmp.energy_credits) {
        if (energy_credits >= 2000) {
            value = 3 * value / 2;
        }
        if (energy_credits >= 4000) {
            value = 3 * value / 2;
        }
    }
    if (*DiploGreatSatan
    || (((their_status & DIPLO_WANT_REVENGE) || (their_agenda & AGENDA_UNK_200))
    && !*DiploOpenBaseID
    && (!*DiploFearScale || 2 * cmp.mil_strength_1 >= plr.mil_strength_1))) {
        if (plr.ranking >= 6) {
            int min_value = energy_credits * (plr.ranking + cmp.ranking) / 21;
            if (value < min_value) {
                value = min_value;
            }
        }
    }
    *DiploTributeDemand = 25 * ((value + 24) / 25);
    if (*DiploTributeDemand == 25) {
        *DiploTributeDemand = 50;
    }
    if (*DiploTributeDemand > energy_credits
    && *DiploTributeDemand < 2 * energy_credits
    && energy_credits >= ((their_status & DIPLO_UNK_40) ? 100 : 50)
    && !(plr.diplo_status[faction_id_2] & (DIPLO_ATROCITY_VICTIM | DIPLO_WANT_REVENGE))) {
        *DiploTributeDemand = 25 * ((energy_credits + 24) / 25);
    }
    if (!(their_status & (DIPLO_UNK_40 | DIPLO_WANT_REVENGE))
    && (!*DiploGreatSatan || (their_status & DIPLO_UNK_4000000))) {
        energy_compute_net(faction_id);
        if (*DiploTributeDemand > 5 * *FactionCurrentNetIncome) {
            // Fix: guard plr.ranking == 0 (eliminated or unranked faction)
            if (cmp.energy_credits >= cmp.ranking * (2 * plr.energy_credits / 3) / max(1, plr.ranking)) {
                *DiploTributeDemand = 25 * (5 * *FactionCurrentNetIncome / 25);
                if (*DiploTributeDemand < 50) {
                    *DiploTributeDemand = 50;
                }
            }
        }
    }
    if ((((their_status & (DIPLO_TRUCE | DIPLO_TREATY | DIPLO_PACT)) && !(their_status & DIPLO_UNK_10000))
    || *DiploFearScale)
    && !*DiploGreatSatan
    && !(their_status & DIPLO_WANT_REVENGE)) {
        *DiploAttackChance = 0;
    }
    if (!*DiploTributeDemand && !*DiploAttackChance && !(cmp.diplo_agenda[faction_id] & AGENDA_UNK_200)) {
        *DiploHostility = 0;
    }
    *DiploPrevHostility = *DiploHostility;
    *DiploGoodwill = 0;
    if (full_check) {
        *DiploHelpFaction = 0;
        *DiploUnmetEnemy = 0;
        *DiploPlayerEnemy = 0;
        *DiploUnmetFaction = 0;
        *DiploMutualFaction = 0;
    }
    *DiploRivalWarCount = 0;

    for (int i = 1; i < MaxPlayerNum; i++) {
        if (!is_alive(i) || i == faction_id || i == faction_id_2) {
            continue;
        }
        if (!(cmp.diplo_status[i] & DIPLO_COMMLINK)) {
            continue;
        }
        if (full_check
        && (plr.diplo_status[i] & DIPLO_VENDETTA)
        && !(cmp.diplo_status[i] & DIPLO_VENDETTA)
        && !(plr.diplo_status[i] & DIPLO_UNK_10000000)) {
            *DiploPlayerEnemy = i;
        }
        if (cmp.diplo_status[i] & DIPLO_PACT) {
            if (Factions[i].mil_strength_1 > plr.mil_strength_1) {
                --*DiploGoodwill;
            }
            if ((great_satan(faction_id, 0) || *DiploGreatSatan)
            && (Factions[i].diplo_status[faction_id] & DIPLO_VENDETTA)) {
                int reduced = *DiploFearScale - 3;
                if (reduced > 9999) {
                    *DiploFearScale = 9999;
                } else if (reduced <= 0) {
                    *DiploFearScale = 0;
                    *DiploConquestFear = 0;
                    *DiploPrevHostility = 2;
                    *DiploHostility = 2;
                } else {
                    *DiploFearScale = reduced;
                }
            }
        } else if (cmp.diplo_status[i] & DIPLO_VENDETTA) {
            int cmp_mil = cmp.mil_strength_1;
            int alt_mil = Factions[i].mil_strength_1;
            ++*DiploRivalWarCount;
            *DiploGoodwill += 2 * (alt_mil + cmp_mil / 2) / (cmp_mil + 1);
            if (DiploStateA[faction_id_2] & 0x40) {
                if ((plr.diplo_status[i] & (DIPLO_COMMLINK | DIPLO_VENDETTA)) == DIPLO_COMMLINK && alt_mil > cmp_mil) {
                    *DiploHelpFaction = i;
                }
                continue;
            }
            if (!(plr.diplo_status[i] & DIPLO_VENDETTA)
            && ((plr.diplo_status[i] & DIPLO_COMMLINK) || (plr.diplo_status[faction_id_2] & DIPLO_PACT))
            && (!(Factions[i].diplo_status[faction_id] & DIPLO_PACT)
            || !(Factions[i].diplo_status[faction_id] & DIPLO_HAVE_SURRENDERED))) {
                *DiploHelpFaction = i;
                continue;
            }
            if (*DiploHelpFaction || !full_check || (plr.diplo_status[i] & DIPLO_COMMLINK) || is_human(i)) {
                continue;
            }
            if (alt_mil > cmp_mil || !mod_wants_to_attack(faction_id_2, i, 0) || *DiploFearScale) {
                *DiploUnmetEnemy = i;
            }
        } else if (full_check && !is_human(i)) {
            if (plr.diplo_status[i] & DIPLO_COMMLINK) {
                if (!(plr.diplo_status[i] & DIPLO_UNK_10000000)) {
                    *DiploMutualFaction = i;
                }
            } else if ((!*DiploUnmetFaction || contiguous(faction_id, i, 0, 0))
            && ((cmp.diplo_status[i] & DIPLO_TREATY) || !*DiploUnmetFaction)) {
                *DiploUnmetFaction = i;
            }
        }
    }
    if (their_agenda & AGENDA_UNK_800) {
        --*DiploGoodwill;
    }
    if (their_agenda & AGENDA_UNK_400) {
        --*DiploGoodwill;
    }
    if (their_status & DIPLO_WANT_REVENGE) {
        --*DiploGoodwill;
    }
    if (their_agenda & AGENDA_UNK_200) {
        --*DiploGoodwill;
    }
    *DiploGoodwill += -*DiploExposedBaseCount - (*DiploReputation - 1) / 3 - ai_fight;
    if (cmp.tech_commerce_bonus > plr.tech_commerce_bonus && ai_fight <= 0) {
        ++*DiploGoodwill;
    }
    if (!(*GameRules & RULES_VICTORY_TRANSCENDENCE) && !cmp.SE_alloc_labs) {
        --*DiploGoodwill;
    }
    if (cmp.mil_strength_1 > plr.mil_strength_1) {
        --*DiploGoodwill;
    }
    if (cmp.best_weapon_value > 2 * plr.best_armor_value) {
        --*DiploGoodwill;
    }
    if (*DiploGoodwill > ((plr.diplo_status[faction_id_2] & DIPLO_VENDETTA) != 0)
    && !*DiploGreatSatan
    && !(their_status & (DIPLO_UNK_40 | DIPLO_PACT))
    && !*DiploAttackChance) {
        *DiploHostility = 0;
        *DiploTributeDemand = 0;
    }
    if ((their_status & DIPLO_PACT) && (their_status & DIPLO_HAVE_SURRENDERED)) {
        *DiploAttackChance = 0;
        *DiploExposedBaseCount = 0;
        *DiploPrevHostility = 0;
        *DiploHostility = 0;
    } else if ((*DiploExposedBaseCount || *DiploAttackChance) && (their_status & DIPLO_WANT_REVENGE)) {
        *DiploPrevHostility = 1;
        *DiploHostility = 1;
    }
    if (great_satan(faction_id, 1)
    && *DiploRivalPactCount < 2
    && FactionRankingsUnk[faction_id] >= *DiploRivalPactScore + 3 * FactionRankingsUnk[faction_id_2]
    && cmp.ranking < 6
    && cmp.mil_strength_1 < plr.mil_strength_1) {
        *DiploCanSurrender = 1;
        if ((cmp.diplo_status[faction_id] & DIPLO_VENDETTA)
        && !(cmp.diplo_agenda[faction_id] & AGENDA_FIGHT_TO_DEATH)
        && !(cmp.diplo_status[faction_id] & DIPLO_ATROCITY_VICTIM)) {
            *DiploHostility = 0;
        }
    } else {
        *DiploCanSurrender = 0;
    }
    if (full_check) {
        scan_prototypes(faction_id, faction_id_2);
        bool betray = (cmp.player_flags_ext & PFLAG_EXT_SHAMELESS_BETRAY_HUMANS) != 0;
        *DiploCanBetray = (plr.diff_level > 1 || betray)
            && !*DiploExposedBaseCount
            && !(their_status & DIPLO_PACT)
            && !(cmp.diplo_agenda[faction_id] & AGENDA_FIGHT_TO_DEATH)
            && (!(their_status & DIPLO_VENDETTA) || *DiploCapturedBases > 1 || betray)
            && (*DiploNearEndgame
                || *DiploHostility
                || soc_conflict
                || (betray && !*DiploFearScale)
                || (*DiploEnemyAllyCount && (their_status & DIPLO_UNK_800000) && *DiploFriction >= 12))
            && ((their_status & DIPLO_WANT_REVENGE)
                || (betray && *DiploHostility)
                || *DiploReputation > 5
                || ((*DiploHostility || soc_conflict > 1) && (plr.mil_strength_1 > cmp.mil_strength_1 || betray)));
    }
    if ((cmp.player_flags & PFLAG_COOP_WITH_HUMAN) && !(their_status & DIPLO_WANT_REVENGE)) {
        *DiploHostility = 0;
    }
    if (cmp.diplo_agenda[faction_id] & AGENDA_FIGHT_TO_DEATH) {
        *DiploHostility = 1;
    } else if (*DiploCanBetray) {
        *DiploHostility = 0;
        if (*DiploFriction > 6) {
            *DiploFriction = 6;
        }
    }
    uint32_t& our_status = plr.diplo_status[faction_id_2];
    if ((our_status & DIPLO_PACT)
    && (!(our_status & DIPLO_WANT_REVENGE) || *DiploRivalWarCount > 1)
    && (!(our_status & DIPLO_UNK_200) || *DiploRivalWarCount)
    && cmp.ranking < plr.ranking
    && ((*GameRules & RULES_VICTORY_COOPERATIVE) || *DiploRivalWarCount)) {
        *DiploHostility = 0;
    }
    if (cmp.diplo_agenda[faction_id] & AGENDA_PERMANENT) {
        *DiploCanBetray = 0;
        *DiploHostility = (their_status & DIPLO_VENDETTA) != 0;
    }
    if (!*DiploPrevHostility && (*DiploFearScale || (our_status & DIPLO_PACT)) && *DiploFriction > 14) {
        *DiploFriction = 14;
    }
    if ((*DiploHostility || (our_status & DIPLO_VENDETTA) || (their_status & DIPLO_WANT_REVENGE))
    && *DiploFriction < 6) {
        *DiploFriction = 6;
    }
}

/* ---- basewin.cpp ---- */

/* Weak so a base-window port of the same Thinker body may replace it. */
__attribute__((weak)) void __cdecl action_sat_attack(int faction_id, int faction_id_tgt, int target_id, int base_id) {
    const int player_id = *CurrentPlayerFaction;
    assert(target_id >= 0 && target_id <= 4);
    if (!(Factions[faction_id].diplo_status[faction_id_tgt] & DIPLO_PACT)) {
        double_cross(faction_id, faction_id_tgt, -1);
    }
    if (faction_id == player_id || faction_id_tgt == player_id) {
        parse_says(0, get_title(faction_id), -1, -1);
        parse_says(1, get_name(faction_id), -1, -1);
        parse_says(2, get_noun(faction_id), -1, -1);
        parse_says(3, get_title(faction_id_tgt), -1, -1);
        parse_says(4, get_name(faction_id_tgt), -1, -1);
        parse_says(5, get_noun(faction_id_tgt), -1, -1);
        if (target_id >= 0 && target_id < 4) {
            parse_says(6, Facility[FAC_SKY_HYDRO_LAB + target_id].name, -1, -1);
        } else {
            parse_says(6, Facility[FAC_GEOSYNC_SURVEY_POD].name, -1, -1);
        }
        parse_says(7, MFactions[faction_id_tgt].adj_name_faction, -1, -1);
    }
    if (game_rand() % 100 >= OrbitalAttackChance) {
        --Factions[faction_id].satellites_ODP;
        if (faction_id == player_id) {
            popp(ScriptFile, "WEMUFFEDIT", 0, "satbat_sm.pcx", 0);
        } else if (faction_id_tgt == player_id) {
            popp(ScriptFile, "THEYMUFFEDIT", 0, "satbat_sm.pcx", 0);
        }
    } else {
        switch (target_id) {
        case 0: --Factions[faction_id_tgt].satellites_nutrient; break;
        case 1: --Factions[faction_id_tgt].satellites_mineral; break;
        case 2: --Factions[faction_id_tgt].satellites_energy; break;
        case 3: --Factions[faction_id_tgt].satellites_ODP; break;
        case 4:
            if (base_id >= 0 && base_id < *BaseCount) { // Fix: bounds check
                set_fac(FAC_GEOSYNC_SURVEY_POD, base_id, 0);
            }
            break;
        }
        ++Factions[faction_id].ODP_deployed;
        if (faction_id == player_id) {
            popp(ScriptFile, "WEKILLEDIT", 0, "satbat_sm.pcx", 0);
        } else if (faction_id_tgt == player_id) {
            popp(ScriptFile, "THEYKILLEDIT", 0, "satbat_sm.pcx", 0);
        }
    }
    // Original: ReportIf redraw (UI)
}

/* ---- terranx.exe routines, new code ---- */

/* Nearest base of faction_id to (x, y), lowest id on ties, or -1. */
static int nearest_own_base(int faction_id, int x, int y) {
    int best_id = -1;
    int best_dist = INT_MAX;
    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].faction_id == faction_id) {
            int dist = map_range(x, y, Bases[i].x, Bases[i].y);
            if (dist < best_dist) {
                best_dist = dist;
                best_id = i;
            }
        }
    }
    return best_id;
}

/* Units of faction_id standing in a base of faction_id_2 or stacked with its units. */
static void pact_units_home(int faction_id, int faction_id_2) {
    std::vector<int> moving;
    for (int i = 0; i < *VehCount; i++) {
        VEH* veh = &Vehs[i];
        if (veh->faction_id != faction_id) {
            continue;
        }
        int base_id = base_at(veh->x, veh->y);
        bool shared = base_id >= 0 && Bases[base_id].faction_id == faction_id_2;
        if (!shared) {
            for (int k = veh_top(i); k >= 0; k = Vehs[k].next_veh_id_stack) {
                if (Vehs[k].faction_id == faction_id_2) {
                    shared = true;
                    break;
                }
            }
        }
        if (shared) {
            moving.push_back(i);
        }
    }
    // Highest id first, so a veh_kill does not shift the ids still to be handled.
    for (auto it = moving.rbegin(); it != moving.rend(); ++it) {
        int veh_id = *it;
        VEH* veh = &Vehs[veh_id];
        int home_id = veh->home_base_id;
        if (home_id < 0 || home_id >= *BaseCount || Bases[home_id].faction_id != faction_id) {
            home_id = nearest_own_base(faction_id, veh->x, veh->y);
        }
        if (home_id >= 0) {
            veh_put(veh_id, Bases[home_id].x, Bases[home_id].y);
        } else {
            veh_kill(veh_id); // nowhere to go
        }
    }
}

void __cdecl pact_ends(int faction_id_1, int faction_id_2) {
    if (faction_id_1 <= 0 || faction_id_1 >= MaxPlayerNum || faction_id_2 <= 0
    || faction_id_2 >= MaxPlayerNum || faction_id_1 == faction_id_2) {
        return;
    }
    debug("pact_ends %d %d\n", faction_id_1, faction_id_2);
    if (!has_treaty(faction_id_1, faction_id_2, DIPLO_PACT)) {
        return;
    }
    // The pact goes, the treaty that came with it stays; a surrender ends with the pact.
    treaty_off(faction_id_1, faction_id_2, DIPLO_PACT|DIPLO_HAVE_SURRENDERED);
    pact_units_home(faction_id_1, faction_id_2);
    pact_units_home(faction_id_2, faction_id_1);
    if (faction_id_1 == *CurrentPlayerFaction || faction_id_2 == *CurrentPlayerFaction) {
        const int other = (faction_id_1 == *CurrentPlayerFaction ? faction_id_2 : faction_id_1);
        parse_says(0, get_title(other), -1, -1);
        parse_says(1, get_name(other), -1, -1);
        parse_says(2, get_noun(other), -1, -1);
        parse_says(3, get_pact_hood(faction_id_1, faction_id_2), -1, -1);
    }
    engine_notify(EV_MESSAGE, faction_id_1, faction_id_2, 0, 0, -1, -1, "pact_ends");
}

int __cdecl ascending(int UNUSED(faction_id)) {
    return voice_of_planet();
}

void __cdecl trade_maps(int faction_id_1, int faction_id_2) {
    if (faction_id_1 <= 0 || faction_id_1 >= MaxPlayerNum || faction_id_2 <= 0
    || faction_id_2 >= MaxPlayerNum || faction_id_1 == faction_id_2 || !*MapTiles) {
        return;
    }
    const uint8_t bit1 = (uint8_t)(1 << faction_id_1);
    const uint8_t bit2 = (uint8_t)(1 << faction_id_2);
    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y & 1; x < *MapAreaX; x += 2) {
            MAP* sq = mapsq(x, y);
            if (!sq) {
                continue;
            }
            if ((sq->visibility & bit1) && !(sq->visibility & bit2)) {
                sq->visibility |= bit2;
                synch_bit(x, y, faction_id_2);
            } else if ((sq->visibility & bit2) && !(sq->visibility & bit1)) {
                sq->visibility |= bit1;
                synch_bit(x, y, faction_id_1);
            }
        }
    }
}

int __cdecl communicate(int faction_id, int faction_id_2, int flag) {
    if (faction_id <= 0 || faction_id >= MaxPlayerNum || faction_id_2 <= 0
    || faction_id_2 >= MaxPlayerNum || faction_id == faction_id_2) {
        return 0;
    }
    if (s_diplo_hooks.communicate) {
        return s_diplo_hooks.communicate(faction_id, faction_id_2, flag);
    }
    // Headless: the contact is recorded without a dialog.
    if (is_human(faction_id)) {
        Factions[faction_id].diplo_spoke[faction_id_2] = *CurrentTurn;
    }
    DiploStateA[faction_id_2] = 0;
    return 0;
}

/* ---- Comm-link proposals ---- */

bool diplo_proposal_valid(int faction_id, int faction_id_2, int proposal) {
    if (faction_id <= 0 || faction_id >= MaxPlayerNum || faction_id_2 <= 0
    || faction_id_2 >= MaxPlayerNum || faction_id == faction_id_2
    || !is_alive(faction_id) || !is_alive(faction_id_2)
    || !has_treaty(faction_id, faction_id_2, DIPLO_COMMLINK)) {
        return false;
    }
    uint32_t status = Factions[faction_id].diplo_status[faction_id_2];
    switch (proposal) {
    case DP_TRUCE:
        return (status & DIPLO_VENDETTA) != 0;
    case DP_TREATY:
        return !(status & (DIPLO_VENDETTA|DIPLO_TREATY|DIPLO_PACT));
    case DP_PACT:
        return (status & DIPLO_TREATY) && !(status & (DIPLO_VENDETTA|DIPLO_PACT));
    case DP_END_PACT:
        return (status & DIPLO_PACT) != 0;
    case DP_VENDETTA:
        return !(status & DIPLO_VENDETTA);
    default:
        return false;
    }
}

bool diplo_proposal_accepted(int faction_id, int faction_id_2, int proposal) {
    if (!diplo_proposal_valid(faction_id, faction_id_2, proposal)) {
        return false;
    }
    if (proposal == DP_END_PACT || proposal == DP_VENDETTA) {
        return true; // unilateral
    }
    Faction& cmp = Factions[faction_id_2];
    // Each proposal uses up some of the rival's patience; it recovers every
    // other turn in enemy_diplomacy.
    diplomacy_check(faction_id, faction_id_2, 1);
    const int patience = get_patience(faction_id, faction_id_2);
    if (cmp.diplo_patience[faction_id] >= max(1, patience)) {
        return false;
    }
    cmp.diplo_patience[faction_id]++;

    const uint32_t their_status = cmp.diplo_status[faction_id];
    const uint32_t their_agenda = cmp.diplo_agenda[faction_id];
    const int mood = get_mood(*DiploFriction);
    const bool revenge = (their_status & DIPLO_WANT_REVENGE) != 0;
    if (their_agenda & AGENDA_FIGHT_TO_DEATH) {
        return false;
    }
    switch (proposal) {
    case DP_TRUCE:
        // A rival that fears the player, or is not hostile and not seething, accepts.
        return (*DiploFearScale > 0 || (!*DiploHostility && !*DiploGreatSatan))
            && (!revenge || *DiploFearScale > 0)
            && mood < MOOD_SEETHING;
    case DP_TREATY:
        return !*DiploHostility && !*DiploGreatSatan && !revenge
            && !(their_status & (DIPLO_SHALL_BETRAY|DIPLO_UNK_800|DIPLO_UNK_40))
            && *DiploReputation < 5
            && mood <= MOOD_NONCOMMITTAL;
    case DP_PACT:
        return !*DiploHostility && !*DiploGreatSatan && !revenge
            && !(their_status & (DIPLO_SHALL_BETRAY|DIPLO_UNK_800|DIPLO_UNK_40|DIPLO_ATROCITY_VICTIM))
            && *DiploReputation <= 2
            && *DiploGoodwill >= 0
            && mood <= MOOD_COOPERATIVE;
    default:
        return false;
    }
}

void diplo_proposal_apply(int faction_id, int faction_id_2, int proposal) {
    if (!diplo_proposal_valid(faction_id, faction_id_2, proposal)) {
        return;
    }
    Factions[faction_id].diplo_spoke[faction_id_2] = *CurrentTurn;
    switch (proposal) {
    case DP_TRUCE:
        treaty_on(faction_id, faction_id_2, DIPLO_TRUCE);
        break;
    case DP_TREATY:
        treaty_on(faction_id, faction_id_2, DIPLO_TREATY);
        break;
    case DP_PACT:
        treaty_on(faction_id, faction_id_2, DIPLO_PACT);
        break;
    case DP_END_PACT:
        pact_ends(faction_id, faction_id_2);
        cause_friction(faction_id_2, faction_id, 2);
        break;
    case DP_VENDETTA:
        if (has_treaty(faction_id_2, faction_id, DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT)) {
            double_cross(faction_id, faction_id_2, -1);
        } else {
            treaty_on(faction_id, faction_id_2, DIPLO_VENDETTA);
        }
        break;
    default:
        break;
    }
}

} // namespace engine
} // namespace alpha
