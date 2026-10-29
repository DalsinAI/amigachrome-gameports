/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP8: a minimal Planetary Council.
 *
 * The council of the original game (terranx.exe council, call_council,
 * can_call_council) is in no upstream source, so this is new code built on the
 * Thinker helpers it calls (council_votes, eligible, aah_ooga, un_charter,
 * global_trade_pact; see diplo.cpp) and on the #PROPOSALS rules table
 * (Proposal[].preq_tech). Rules (see ENGINE-WP8.md):
 * - A faction may call the council when it has a comm link with at least half
 *   of the other living factions that vote, and at least
 *   Rules->min_turns_between_councils turns after its last call.
 * - The first session elects the Planetary Governor. A proposal needs its
 *   technology, and the same proposal waits min_turns_between_councils turns.
 * - Elections (Governor, Supreme Leader) have two candidates: the proposer and
 *   the eligible faction with the most other votes. The Governor needs more
 *   votes than the other candidate; the Supreme Leader needs three quarters of
 *   all votes on the council (Diplomatic Victory). Other proposals pass with
 *   more yes than no votes.
 * - Votes are council_votes() (population, Empath Guild, Clinical Immortality,
 *   faction bonus). Computer players vote by their relations with the proposer
 *   or candidates and by the proposal's effect on them; humans are asked
 *   through the event sink.
 */
#include "engine.h"
#include "diplo.h"
#include "turn.h"

namespace alpha {
namespace engine {

static bool council_voter(int faction_id) {
    return faction_id > 0 && faction_id < MaxPlayerNum && is_alive(faction_id) && !is_alien(faction_id);
}

static bool is_election(int proposal) {
    return proposal == PROP_ELECT_PLANETARY_GOVERNOR || proposal == PROP_UNITE_SUPREME_LEADER;
}

int __cdecl can_call_council(int faction_id, int flag) {
    if ((*GameMoreRules & MRULES_NO_PLANETARY_COUNCIL) || (*GameState & STATE_GAME_DONE)) {
        return 0;
    }
    if (!council_voter(faction_id)) {
        return 0;
    }
    int others = 0;
    int contacts = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (i != faction_id && council_voter(i)) {
            others++;
            if (has_treaty(faction_id, i, DIPLO_COMMLINK)) {
                contacts++;
            }
        }
    }
    if (!others || 2 * contacts < others) {
        return 0;
    }
    if (!flag && *CurrentTurn - Factions[faction_id].council_call_turn
    < Rules->min_turns_between_councils) {
        return 0;
    }
    return 1;
}

bool council_proposal_valid(int faction_id, int proposal) {
    if (!council_voter(faction_id) || proposal < 0 || proposal > PROP_REINSTATE_UN_CHARTER) {
        return false;
    }
    if (!has_tech(Proposal[proposal].preq_tech, faction_id)) {
        return false; // None is always met, Disable never
    }
    if (*CurrentTurn - ProposalCallTurn[proposal] < Rules->min_turns_between_councils) {
        return false;
    }
    if (!(*GameState & STATE_COUNCIL_HAS_CONVENED) && proposal != PROP_ELECT_PLANETARY_GOVERNOR) {
        return false; // the first session elects the Governor
    }
    switch (proposal) {
    case PROP_ELECT_PLANETARY_GOVERNOR:
        return true;
    case PROP_UNITE_SUPREME_LEADER:
        return (*GameRules & RULES_VICTORY_DIPLOMATIC) && eligible(faction_id);
    case PROP_SALVAGE_UNITY_CORE:
        return ProposalPassCount[PROP_SALVAGE_UNITY_CORE] == 0;
    case PROP_GLOBAL_TRADE_PACT:
        return !global_trade_pact();
    case PROP_REPEAL_GLOBAL_TRADE_PACT:
        return global_trade_pact();
    case PROP_LAUNCH_SOLAR_SHADE:
        return ProposalPassCount[PROP_LAUNCH_SOLAR_SHADE] == 0;
    case PROP_INCREASE_SOLAR_SHADE:
        return ProposalPassCount[PROP_LAUNCH_SOLAR_SHADE] > 0;
    case PROP_MELT_POLAR_CAPS:
        return true;
    case PROP_REPEAL_UN_CHARTER:
        return un_charter();
    case PROP_REINSTATE_UN_CHARTER:
        return !un_charter();
    }
    return false;
}

/* Election candidates: the proposer and the eligible faction with the most votes
 * after it (lowest id on ties), or 0. */
static void council_candidates(int proposer, int candidate[2]) {
    candidate[0] = proposer;
    candidate[1] = 0;
    int best_votes = -1;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (i != proposer && council_voter(i) && eligible(i)) {
            int votes = council_votes(i);
            if (votes > best_votes) {
                best_votes = votes;
                candidate[1] = i;
            }
        }
    }
}

/* How much faction_id likes faction_id_2 (positive: friendly). */
static int council_relation(int faction_id, int faction_id_2) {
    if (faction_id == faction_id_2) {
        return 100;
    }
    uint32_t status = Factions[faction_id].diplo_status[faction_id_2];
    int value = 0;
    if (status & DIPLO_PACT) {
        value += 4;
    } else if (status & DIPLO_TREATY) {
        value += 2;
    } else if (status & DIPLO_TRUCE) {
        value += 1;
    }
    if (status & DIPLO_VENDETTA) {
        value -= 4;
    }
    if (status & (DIPLO_WANT_REVENGE|DIPLO_ATROCITY_VICTIM)) {
        value -= 3;
    }
    value -= Factions[faction_id].diplo_friction[faction_id_2] / 5;
    value -= reputation(faction_id_2, faction_id) / 3;
    return value;
}

/* Computer player's vote (also the prediction for humans). No side effects. */
static int council_vote_ai(int faction_id, int proposal, int proposer, const int candidate[2]) {
    if (!council_voter(faction_id) || !council_votes(faction_id)) {
        return 0;
    }
    Faction& plr = Factions[faction_id];
    if (is_election(proposal)) {
        if (faction_id == candidate[0] || faction_id == candidate[1]) {
            return faction_id;
        }
        int best = 0;
        int best_value = INT_MIN;
        for (int k = 0; k < 2; k++) {
            int c = candidate[k];
            if (c <= 0) {
                continue;
            }
            int value = council_relation(faction_id, c);
            if (proposal == PROP_UNITE_SUPREME_LEADER) {
                // Only a pact partner (in a cooperative game, or after surrender)
                // helps another faction win the game.
                uint32_t status = plr.diplo_status[c];
                bool ally = (status & DIPLO_PACT)
                    && ((*GameRules & RULES_VICTORY_COOPERATIVE) || (status & DIPLO_HAVE_SURRENDERED));
                if (!ally) {
                    continue;
                }
            }
            if (value > best_value) {
                best_value = value;
                best = c;
            }
        }
        if (proposal == PROP_ELECT_PLANETARY_GOVERNOR && best_value < -2) {
            return 0; // dislikes both
        }
        return best;
    }
    int interest = 0;
    switch (proposal) {
    case PROP_SALVAGE_UNITY_CORE:
        interest = (plr.ranking >= 7 ? -1 : 1); // the leader gains least
        break;
    case PROP_GLOBAL_TRADE_PACT:
        interest = (plr.sanction_turns ? -1 : 1);
        break;
    case PROP_REPEAL_GLOBAL_TRADE_PACT:
        interest = (plr.sanction_turns ? 1 : -1);
        break;
    case PROP_LAUNCH_SOLAR_SHADE:
    case PROP_INCREASE_SOLAR_SHADE:
        interest = (*ClimateFutureChange > 0 || *MapSeaLevel > 0 ? 1 : -1);
        break;
    case PROP_MELT_POLAR_CAPS:
        interest = (MFactions[faction_id].is_aquatic() ? 1 : -1);
        break;
    case PROP_REPEAL_UN_CHARTER:
        interest = (plr.atrocities || plr.major_atrocities || plr.planet_busters ? 1 : -1);
        break;
    case PROP_REINSTATE_UN_CHARTER:
        interest = (plr.atrocities || plr.major_atrocities || plr.planet_busters ? -1 : 1);
        break;
    }
    int value = 2 * interest;
    if (proposer != faction_id) {
        int relation = council_relation(faction_id, proposer);
        value += (relation > 2 ? 1 : (relation < -2 ? -1 : 0));
    } else {
        value = 1;
    }
    return (value > 0) - (value < 0);
}

int council_vote(int faction_id, int proposal, int proposer, const int candidate[2]) {
    if (!council_voter(faction_id) || !council_votes(faction_id)) {
        return 0;
    }
    if (!is_human(faction_id)) {
        return council_vote_ai(faction_id, proposal, proposer, candidate);
    }
    // Human: popup in the "council" script namespace (not an original label).
    // strings: 0 proposal name, 1 proposer noun, 2/3 candidate nouns.
    parse_says(0, Proposal[proposal].name ? Proposal[proposal].name : "", -1, -1);
    parse_says(1, get_noun(proposer), -1, -1);
    if (is_election(proposal)) {
        parse_says(2, candidate[0] > 0 ? get_noun(candidate[0]) : "", -1, -1);
        parse_says(3, candidate[1] > 0 ? get_noun(candidate[1]) : "", -1, -1);
        int answer = X_pop_2("council", "ELECT", 0); // 0/1 candidate, else abstain
        if (answer == 0 || answer == 1) {
            return candidate[answer] > 0 ? candidate[answer] : 0;
        }
        return 0;
    }
    int answer = X_pop_2("council", "VOTE", 0); // 0 yes, 1 no, else abstain
    return answer == 0 ? 1 : (answer == 1 ? -1 : 0);
}

static void council_climate(int direction) {
    // Same bookkeeping as the global warming trigger in mod_base_production:
    // turn_upkeep moves the sea level by ClimateFutureChange over the next turns.
    const int amount = 3;
    *MapSeaLevelCouncil += direction * amount;
    *ClimateFutureChange += direction * max(1, amount * WorldBuilder->sea_level_rises / 3);
    *ClimateValueA = abs(*ClimateFutureChange);
    *ClimateValueB = 20;
    *ClimateValueC = 0;
}

static void council_apply(const CouncilVote& v) {
    const int player_id = *CurrentPlayerFaction;
    switch (v.proposal) {
    case PROP_ELECT_PLANETARY_GOVERNOR:
        if (v.result == COUNCIL_PASSED) {
            *GovernorFaction = v.winner;
        }
        break;
    case PROP_UNITE_SUPREME_LEADER:
        if (v.result == COUNCIL_PASSED) {
            int w = v.winner;
            Factions[w].player_flags |= PFLAG_UNK_20000;
            *GameState |= STATE_VICTORY_DIPLOMATIC | STATE_GAME_DONE;
            if (w == player_id) {
                *GameVictoryType = VIC_DIPLOMATIC_SOLO;
            } else if (player_id > 0 && player_id < MaxPlayerNum
            && (Factions[w].diplo_status[player_id] & DIPLO_PACT)
            && (*GameRules & RULES_VICTORY_COOPERATIVE)) {
                *GameVictoryType = VIC_DIPLOMATIC_COOP;
            } else {
                *GameVictoryType = VIC_DIPLOMATIC_LOSS;
            }
        }
        break;
    case PROP_SALVAGE_UNITY_CORE:
        if (v.result == COUNCIL_PASSED) {
            for (int i = 1; i < MaxPlayerNum; i++) {
                if (is_alive(i)) {
                    Factions[i].energy_credits += 500;
                }
            }
        }
        break;
    case PROP_LAUNCH_SOLAR_SHADE:
    case PROP_INCREASE_SOLAR_SHADE:
        if (v.result == COUNCIL_PASSED) {
            council_climate(-1);
        }
        break;
    case PROP_MELT_POLAR_CAPS:
        if (v.result == COUNCIL_PASSED) {
            council_climate(1);
        }
        break;
    default:
        break; // trade pact and U.N. charter: ProposalPassCount is the state
    }
    if (v.result == COUNCIL_PASSED) {
        ProposalPassCount[v.proposal]++;
    }
}

CouncilVote council_session(int faction_id, int proposal) {
    CouncilVote v = {};
    v.proposal = proposal;
    v.proposer = faction_id;
    v.result = COUNCIL_NONE;
    if (!council_proposal_valid(faction_id, proposal)) {
        return v;
    }
    if (is_election(proposal)) {
        council_candidates(faction_id, v.candidate);
    }
    for (int i = 1; i < MaxPlayerNum; i++) {
        v.votes[i] = council_voter(i) ? council_votes(i) : 0;
        v.total += v.votes[i];
    }
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (!v.votes[i]) {
            continue;
        }
        int choice = council_vote(i, proposal, faction_id, v.candidate);
        v.choice[i] = choice;
        if (is_election(proposal)) {
            if (choice > 0 && choice < MaxPlayerNum) {
                v.tally[choice] += v.votes[i];
            }
        } else if (choice > 0) {
            v.yes += v.votes[i];
        } else if (choice < 0) {
            v.no += v.votes[i];
        }
    }
    if (proposal == PROP_ELECT_PLANETARY_GOVERNOR) {
        int a = v.candidate[0];
        int b = v.candidate[1];
        int ta = (a > 0 ? v.tally[a] : 0);
        int tb = (b > 0 ? v.tally[b] : 0);
        v.yes = max(ta, tb);
        v.no = min(ta, tb);
        if (ta > tb) {
            v.winner = a;
        } else if (tb > ta) {
            v.winner = b;
        }
        v.result = (v.winner > 0 ? COUNCIL_PASSED : COUNCIL_FAILED);
    } else if (proposal == PROP_UNITE_SUPREME_LEADER) {
        for (int k = 0; k < 2; k++) {
            int c = v.candidate[k];
            if (c > 0 && v.total > 0 && 4 * v.tally[c] >= 3 * v.total) {
                v.winner = c;
            }
        }
        v.yes = (v.candidate[0] > 0 ? v.tally[v.candidate[0]] : 0);
        v.no = v.total - v.yes;
        v.result = (v.winner > 0 ? COUNCIL_PASSED : COUNCIL_FAILED);
    } else {
        v.result = (v.yes > v.no ? COUNCIL_PASSED : COUNCIL_FAILED);
    }
    ProposalCallTurn[proposal] = *CurrentTurn;
    Factions[faction_id].council_call_turn = *CurrentTurn;
    *GameState |= STATE_COUNCIL_HAS_CONVENED;
    council_apply(v);

    for (int i = 1; i < MaxPlayerNum; i++) {
        if (!v.votes[i]) {
            continue;
        }
        CouncilProposals[i] = proposal;
        if (is_election(proposal)) {
            CouncilVoteState[i] = (v.winner > 0 && v.winner == v.choice[i]) ? 1 : -1;
        } else {
            CouncilVoteState[i] = v.result;
        }
    }
    const int player_id = *CurrentPlayerFaction;
    if (player_id > 0 && player_id < MaxPlayerNum && v.votes[player_id]) {
        // Original council result labels of script.txt.
        parse_says(0, Proposal[proposal].name ? Proposal[proposal].name : "", -1, -1);
        if (proposal == PROP_ELECT_PLANETARY_GOVERNOR) {
            if (*GovernorFaction > 0) {
                parse_says(1, get_title(*GovernorFaction), -1, -1);
                parse_says(2, get_name(*GovernorFaction), -1, -1);
                parse_says(3, get_noun(*GovernorFaction), -1, -1);
            }
            if (v.winner > 0 && v.winner == player_id) {
                popp(ScriptFile, "COUNCILHOTGOVWIN", 0, "council_sm.pcx", 0);
            } else if (*GovernorFaction > 0) {
                popp(ScriptFile, "COUNCILHOTGOVLOSE", 0, "council_sm.pcx", 0);
            } else {
                popp(ScriptFile, "COUNCILHOTGOVNONE", 0, "council_sm.pcx", 0);
            }
        } else if (proposal != PROP_UNITE_SUPREME_LEADER) {
            popp(ScriptFile, v.result == COUNCIL_PASSED ? "COUNCILHOTPASS" : "COUNCILHOTFAIL",
                0, "council_sm.pcx", 0);
        }
    }
    engine_notify(EV_MESSAGE, proposal, faction_id, v.result, v.winner, -1, -1, "council");
    return v;
}

/* Votes faction_id would get as a candidate, predicted with the computer-player rule. */
static int council_predict(int faction_id, int proposal) {
    int candidate[2];
    council_candidates(faction_id, candidate);
    int tally = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (council_voter(i) && council_vote_ai(i, proposal, faction_id, candidate) == faction_id) {
            tally += council_votes(i);
        }
    }
    return tally;
}

int council_ai_proposal(int faction_id) {
    if (!council_voter(faction_id) || is_human(faction_id) || !can_call_council(faction_id, 0)) {
        return -1;
    }
    int total = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (council_voter(i)) {
            total += council_votes(i);
        }
    }
    if (council_proposal_valid(faction_id, PROP_UNITE_SUPREME_LEADER)
    && 4 * council_predict(faction_id, PROP_UNITE_SUPREME_LEADER) >= 3 * total && total > 0) {
        return PROP_UNITE_SUPREME_LEADER;
    }
    if (council_proposal_valid(faction_id, PROP_ELECT_PLANETARY_GOVERNOR)) {
        if (!(*GameState & STATE_COUNCIL_HAS_CONVENED)) {
            return PROP_ELECT_PLANETARY_GOVERNOR;
        }
        if (*GovernorFaction != faction_id && eligible(faction_id)
        && 2 * council_predict(faction_id, PROP_ELECT_PLANETARY_GOVERNOR) > total) {
            return PROP_ELECT_PLANETARY_GOVERNOR;
        }
    }
    return -1;
}

void __cdecl call_council(int faction_id) {
    if (!council_voter(faction_id) || is_human(faction_id)) {
        return;
    }
    int proposal = council_ai_proposal(faction_id);
    if (proposal >= 0) {
        council_session(faction_id, proposal);
    }
}

void install_wp8_turn_hooks() {
    TurnHooks& hooks = turn_hooks();
    hooks.pact_ends = pact_ends;
    hooks.can_call_council = can_call_council;
    hooks.call_council = call_council;
}

} // namespace engine
} // namespace alpha
