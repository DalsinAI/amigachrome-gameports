/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP8: diplomacy, Planetary Council, victory and score.
 * Ported from Thinker Mod src/faction.cpp, src/faction_diplo.cpp,
 * src/faction_event.cpp, src/score.cpp, src/game.cpp and src/probe.cpp at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af (MIT; see engine/THINKER-LICENSE).
 * pact_ends, can_call_council, call_council, communicate and trade_maps only
 * exist in terranx.exe; they are new code here (see ENGINE-WP8.md).
 *
 * The functions listed in the WP8 section of engine_wp.h are defined in
 * diplo.cpp, diplo_event.cpp, victory.cpp and council.cpp. This header adds the
 * other Thinker helpers of those files that other packages call (several of
 * them are declared by WP2/WP3/WP4 as weak placeholders) and the WP8 API.
 * Include engine.h first.
 */
#pragma once

namespace alpha {
namespace engine {

/* ---- Runtime globals of the diplomacy code (terranx.exe data, not saved) ----
 * Thinker names; each points into one static WP8 block. diplomacy_check()
 * fills most of them for the dialog of one faction pair. */
extern int* DiploFriction;
extern int* DiploFrictionFactionID;
extern int* DiploFrictionFactionIDWith;
extern int* DiploSecondFaction;
extern int* DiploHostility;
extern int* DiploPrevHostility;
extern int* DiploAttackChance;
extern int* DiploTributeDemand;
extern int* DiploFearScale;
extern int* DiploConquestFear;
extern int* DiploExposedBaseCount;
extern int* DiploOpenBaseID;
extern int* DiploCapturedBases;
extern int* DiploNearEndgame;
extern int* DiploRivalOutranks;
extern int* DiploGreatSatan;
extern int* DiploRunawayLeader;
extern int* DiploReputation;
extern int* DiploPlayerPactCount;
extern int* DiploRivalPactCount;
extern int* DiploRivalStrongPacts;
extern int* DiploRivalPactScore;
extern int* DiploSpokeTurn;
extern int* DiploRegionFactions; // [MaxRegionNum]
extern int* DiploEnemyAllyCount;
extern int* DiploEnemyAlly;
extern int* DiploGoodwill;
extern int* DiploHelpFaction;
extern int* DiploUnmetEnemy;
extern int* DiploPlayerEnemy;
extern int* DiploUnmetFaction;
extern int* DiploMutualFaction;
extern int* DiploRivalWarCount;
extern int* DiploCanSurrender;
extern int* DiploCanBetray;
extern int* DiploStateA;          // [8] what a computer player wants to tell each faction
extern int (*DiploStateB)[8];     // [8][8] the same for PBEM
extern int* CouncilProposals;     // [8] last proposal each faction voted on (Thinker CouncilProposal)
extern int* CouncilVoteState;     // [8] 1 passed/won, -1 failed/lost, 0 none
extern int* GameVictoryType;      // VictoryType of the finished game
extern int* ControlTurnB;         // end_of_game: return to the main menu
extern int* FactionCurrentNetIncome;

/* Clear the runtime globals above (call after binding a new game). */
void diplo_runtime_reset();

/* ---- faction.cpp ---- */
void __cdecl agenda_off(int faction_id_1, int faction_id_2, uint32_t status);
void __cdecl agenda_on(int faction_id_1, int faction_id_2, uint32_t status);
void __cdecl set_agenda(int faction_id_1, int faction_id_2, uint32_t status, bool add);
void __cdecl major_atrocity(int faction_id, int faction_id_tgt);
int __cdecl break_treaty(int faction_id, int faction_id_tgt, uint32_t status);
void __cdecl intervention(int faction_id_def, int faction_id_atk);
void __cdecl double_cross(int faction_id_atk, int faction_id_def, int faction_id_other);
int __cdecl act_of_aggression(int faction_id_atk, int faction_id_def);
int __cdecl spying(int faction_id);
int __cdecl council_votes(int faction_id);
int __cdecl eligible(int faction_id);
int __cdecl great_beelzebub(int faction_id, int is_aggressive);
int __cdecl great_satan(int faction_id, int is_aggressive);
int __cdecl aah_ooga(int faction_id, int pact_faction_id);
int __cdecl climactic_battle();
int __cdecl at_climax(int faction_id);
void __cdecl cause_friction(int faction_id, int faction_id_with, int friction);
int __cdecl reputation(int faction_id, int faction_id_with);
int __cdecl get_patience(int faction_id_with, int faction_id);
int __cdecl energy_value(int loan_principal);
char* __cdecl get_pact(int faction_id);
char* __cdecl get_pacts(int faction_id);
char* __cdecl get_pacts_2(int faction_id, int faction_id_2);
char* __cdecl get_pact_hood(int faction_id, int faction_id_2);
char* __cdecl get_his_her(int faction_id, int tgl);
char* __cdecl get_him_her(int faction_id, int tgl);
char* __cdecl get_he_she(int faction_id, int tgl);
int __cdecl mod_wants_to_attack(int faction_id, int faction_id_tgt, int faction_id_unk);

/* ---- game.cpp ---- */
bool un_charter();
bool global_trade_pact();
bool victory_done();
bool full_game_turn();
bool voice_of_planet();
int __cdecl generators(int faction_id, int* pop_size_req);

/* ---- faction_event.cpp ---- */
void __cdecl pact_attacks(int faction_id_1, int faction_id_2);
void __cdecl pact_unpact(int faction_id_1, int faction_id_2);
void __cdecl pact_truce(int faction_id_1, int faction_id_2);
void __cdecl pact_treaty(int faction_id_1, int faction_id_2);
void __cdecl enemies_war(int faction_id_1, int faction_id_2);
void __cdecl enemies_unpact(int faction_id_1, int faction_id_2);
void __cdecl enemies_team_up(int faction_id_1, int faction_id_2, int faction_id_3);
void __cdecl enemies_trade_tech(int faction_id_1, int faction_id_2);
void __cdecl enemies_treaty(int faction_id_1, int faction_id_2, int trade_tech);
void __cdecl comm_check(int faction_id_1, int faction_id_2);
int __cdecl territory(int faction_id_1, int faction_id_2, int flags, int* out_base_id, int* out_count);

/* ---- score.cpp ---- */
int __cdecl get_rating(int faction_id, int score);
int __cdecl num_objectives(int faction_id, int incl_pact);
int __cdecl most_objectives(int* winner_id, int* tied_rivals);
void __cdecl compute_score(int faction_id, int* score_val, int* table_val, int flag);

/* ---- probe.cpp (only what corner_market needs; probe teams are deferred) ---- */
int __cdecl vulnerable(int faction_id, int x, int y);
int __cdecl mod_mind_control(int base_id, int faction_id, int is_corner_market);

/* ---- terranx.exe routines, new code ---- */
/* End the Pact of Brotherhood between two factions: the pact (and surrender) bits
 * go, the treaty stays, and units left in the other faction's bases or stacks
 * go home (exe 0x53C650). */
void __cdecl pact_ends(int faction_id_1, int faction_id_2);
/* Voice of Planet has been built (exe 0x5AC680). */
int __cdecl ascending(int faction_id);
/* Exchange map knowledge between two factions (exe trade_maps). */
void __cdecl trade_maps(int faction_id_1, int faction_id_2);
/* Diplomacy contact between the human faction_id and the computer player
 * faction_id_2 (exe communicate). Calls DiploHooks::communicate when the UI
 * installed one; headless it records the contact and clears DiploStateA. */
int __cdecl communicate(int faction_id, int faction_id_2, int flag);

/* ---- Comm-link proposals (used by ui_diplo.cpp and computer players) ---- */
enum DiploProposal {
    DP_TRUCE = 0,        // end the vendetta, sign a truce
    DP_TREATY = 1,       // treaty of friendship
    DP_PACT = 2,         // pact of brotherhood
    DP_END_PACT = 3,     // renounce the pact (keeps the treaty)
    DP_VENDETTA = 4,     // declare a vendetta (breaks any treaty)
    DP_COUNT = 5,
};
/* Can faction_id offer `proposal` to faction_id_2 at all (state allows it)? */
bool diplo_proposal_valid(int faction_id, int faction_id_2, int proposal);
/* Would faction_id_2 accept the proposal from faction_id? Runs
 * diplomacy_check(faction_id, faction_id_2, 1). Unilateral proposals (end pact,
 * vendetta) are always accepted. Deterministic: no random numbers. */
bool diplo_proposal_accepted(int faction_id, int faction_id_2, int proposal);
/* Apply an accepted proposal (treaty_on/pact_ends/double_cross/...). */
void diplo_proposal_apply(int faction_id, int faction_id_2, int proposal);

/* ---- Planetary Council (exe-only council/call_council, new minimal code) ---- */
enum CouncilResult {
    COUNCIL_NONE = 0,      // no session
    COUNCIL_PASSED = 1,    // proposal passed / candidate elected
    COUNCIL_FAILED = -1,   // proposal failed / no candidate had enough votes
};
struct CouncilVote {
    int proposal;          // CouncilProposal
    int proposer;          // faction that called the council
    int candidate[2];      // elections: the two candidates (0 = none)
    int votes[MaxPlayerNum];   // votes each faction holds (council_votes)
    int choice[MaxPlayerNum];  // elections: candidate faction voted for, 0 abstain;
                               // other proposals: 1 yes, -1 no, 0 abstain
    int tally[MaxPlayerNum];   // elections: votes per candidate faction
    int yes;
    int no;
    int total;             // all votes on the council (abstentions included)
    int winner;            // elections: elected faction or 0
    int result;            // CouncilResult
};
/* May faction_id call the council now? flag != 0 skips the interval check. */
int __cdecl can_call_council(int faction_id, int flag);
/* Is `proposal` open to faction_id (tech, state, interval)? */
bool council_proposal_valid(int faction_id, int proposal);
/* The proposal a computer player wants to bring, or -1. */
int council_ai_proposal(int faction_id);
/* How faction_id votes: elections return a candidate faction or 0, other
 * proposals 1 (yes), -1 (no) or 0 (abstain). Humans are asked through the
 * event sink (popup "COUNCILVOTE"/"COUNCILELECT", answer = index). */
int council_vote(int faction_id, int proposal, int proposer, const int candidate[2]);
/* Run one council session: vote, count, apply the result. Returns the record. */
CouncilVote council_session(int faction_id, int proposal);
/* Computer player calls the council when it wants to (exe 0x52C880). */
void __cdecl call_council(int faction_id);

/* ---- Hooks for the UI ---- */
struct DiploHooks {
    /* Human/computer comm-link contact; return value is passed back to the caller. */
    int (*communicate)(int faction_id, int faction_id_2, int flag);
};
DiploHooks& diplo_hooks();

/* Fill WP1's turn_hooks() (engine/turn.h) with pact_ends, can_call_council and
 * call_council. Other hooks are left as they are. */
void install_wp8_turn_hooks();

} // namespace engine
} // namespace alpha
