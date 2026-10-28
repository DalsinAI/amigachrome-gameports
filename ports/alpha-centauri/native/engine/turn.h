/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP1: turn scheduler and End Turn.
 * The turn functions keep the names and bodies of Thinker Mod (src/gameturn.cpp,
 * src/faction.cpp, src/tech.cpp, src/score.cpp, src/map.cpp, src/savegame.cpp at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af, MIT; see engine/THINKER-LICENSE) and are
 * declared in engine_wp.h. This header adds the headless End Turn API, the hooks for
 * routines owned by packages that are not in engine_wp.h, and WP1-local copies of
 * small Thinker helpers.
 * Include engine.h first.
 */
#pragma once

#include <string>
#include <vector>

namespace alpha {
namespace engine {

/* ---- End Turn API ---- */

/* Why engine_end_turn() stopped. */
enum TurnStatus {
    TURN_HUMAN = 0,      // a human faction's turn begins (TurnResult::faction_id); ControlTurnMove = it
    TURN_ROUND_END = 1,  // no human played this round (all-AI game); next call starts a new turn
    TURN_GAME_OVER = 2,  // end_of_game() reported the end of the game
    TURN_STOPPED = 3,    // ControlTurnA was raised (quit/load request)
};

enum TurnEventKind {
    TE_ENGINE = 0,          // an engine_notify() event: type/a/b/c/d/x/y/text as EngineEvent
    TE_POPUP = 1,           // a dialog request: text = label, a = answer, b = PopupKind
    TE_TURN_BEGIN = 2,      // turn_upkeep advanced the turn: turn, a = mission year
    TE_FACTION_UPKEEP = 3,  // faction_upkeep(faction_id) is about to run
    TE_HUMAN_TURN = 4,      // the human faction's turn begins
    TE_AI_TURN = 5,         // mod_enemy_turn(faction_id) is about to run
    TE_ROUND_END = 6,       // every faction has moved (no human turn this round)
    TE_GAME_OVER = 7,
    TE_HUMAN_TURN_END = 8,  // engine_end_turn ended this human faction's turn
    TE_TERRITORY = 9,       // reset_territory recomputed the borders; a = owned tiles
};

struct TurnEvent {
    int kind;
    int turn;          // *CurrentTurn when the event was recorded
    int faction_id;    // *CurrentFaction when the event was recorded (or the subject faction)
    int type;          // TE_ENGINE: EngineEventType
    int a;
    int b;
    int c;
    int d;
    int x;
    int y;
    std::string text;
};

struct TurnResult {
    int status;        // TurnStatus
    int faction_id;    // TURN_HUMAN: faction whose turn begins, else -1
    int turn;          // *CurrentTurn after the call
    int year;          // *CurrentMissionYear after the call
    std::vector<TurnEvent> events;
};

/* End the pending human turn (if ControlTurnMove names one) and run the ordered turn
 * for every following faction: turn upkeep at the start of a new turn, then for each
 * living faction in id order faction_upkeep (social, repair and movement refill,
 * production, energy) and either the computer player (mod_enemy_turn) or a stop for the
 * next human. On a new game (turn 0) or an all-computer game the first call runs one
 * whole turn. Every event raised meanwhile is forwarded to the installed sink and also
 * returned. */
TurnResult engine_end_turn();

/* Thinker's control_turn loop without its windows; returns the TurnStatus of the stop.
 * control_turn() (engine_wp.h) calls this and discards the result. */
int control_turn_run();

/* Next living faction after faction_id in turn order (0..7), or -1 if none. */
int turn_order_next(int faction_id);

/* ---- Hooks for routines owned by other packages and not declared in engine_wp.h ----
 * Null hooks are skipped (see ENGINE-WP1.md). Integration wires them. */
struct TurnHooks {
    void (*crash_landing)(int faction_id);                 // exe-only, new-game intro
    void (*world_climate)();                               // WP7 mapgen
    void (*interlude)(int event_id, const char* text, int a3, int a4); // UI
    void (*pact_ends)(int faction_id_1, int faction_id_2); // WP8
    int (*can_call_council)(int faction_id, int flag);     // WP8
    void (*call_council)(int faction_id);                  // WP8
    void (*plans_upkeep)(int faction_id);                  // WP6
    int (*probe_upkeep)(int faction_id);                   // WP6
    void (*move_upkeep)(int faction_id, int mode);         // WP6 (UM_Full = 0)
    void (*mon_tech_discovered)(int faction_id, int tech_id); // WP2 monument
    int (*energy_limit)(int faction_id);                   // exe-only, see ENGINE-WP1.md
    void (*random_base_event)(int base_id, int event_value); // the 23 base events
    void (*consider_designs)(int faction_id);              // WP5/WP6, time warp
};
TurnHooks& turn_hooks();

/* ---- WP1-local copies of small Thinker helpers owned by other files ----
 * Kept in their own namespace so they never clash with the owning package. */
namespace wp1 {
bool full_game_turn();
bool victory_done();
bool voice_of_planet();
int climactic_battle();
int council_votes(int faction_id);
int aah_ooga(int faction_id, int pact_faction_id);
int great_beelzebub(int faction_id, int is_aggressive);
int great_satan(int faction_id, int is_aggressive);
int reputation(int faction_id, int faction_id_with);
void set_agenda(int faction_id_1, int faction_id_2, uint32_t status, bool add);
int num_objectives(int faction_id, int incl_pact);
int breed_mod(int base_id, int faction_id);
bool base_can_riot(int base_id, bool allow_staple);
int fac_maint(int facility_id, int faction_id);
int energy_limit(int faction_id);
void init_save_game(int faction_id);
int strcmpi(const char* a, const char* b);
int pop2(const char* label, const char* pcx_filename, int base_id);
} // namespace wp1

} // namespace engine
} // namespace alpha
