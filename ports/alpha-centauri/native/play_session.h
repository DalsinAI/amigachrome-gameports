/* SPDX-License-Identifier: GPL-3.0-or-later
 * Play command layer between the game screen and the engine (engine/).
 *
 * One Session owns the EngineState of the game being played. The map window,
 * the title screen's New Game flow and the headless play test all drive the
 * game through the same calls: load rules and a save (or start a new game),
 * run Commands (unit orders, attacks, base production, research, social
 * engineering, designs, diplomacy, End Turn), save, and read the panel models
 * the ui_*.cpp files draw. After each command the caller rebuilds its view from
 * view_bytes() (the engine's own SAV bytes, read by OriginalWorld).
 *
 * This header does not include the engine, so the window's translation unit
 * keeps the pure modules' alpha:: names without clashes. Not thread-safe; the
 * engine binds one state at a time and Session::bind() selects this one.
 */
#ifndef ALPHA_PLAY_SESSION_H
#define ALPHA_PLAY_SESSION_H
#include <stddef.h>
#include <stdint.h>
#include <vector>
#include "ui_basewin.h"
#include "ui_battle.h"
#include "ui_design.h"
#include "ui_research.h"
#include "ui_social.h"
#include "ui_diplowin.h"

namespace alpha {
namespace play {

enum PlayStatus {
 PLAY_OK=0,
 PLAY_REFUSED=1,      // the engine refused the order (illegal now)
 PLAY_BAD_ARGS=2,     // ids out of range, not the player's unit/base
 PLAY_NO_GAME=3,      // no game loaded
 PLAY_UNAVAILABLE=4,  // the engine package for this is not in this build
 PLAY_IO=5,           // file or save container error
 PLAY_GAME_OVER=6,    // the game has ended; only save/quit remain
 PLAY_ATTACK=7,       // a move into an enemy tile: use CMD_ATTACK
 PLAY_NEEDS_CONFIRM=8,// research change halves progress: repeat with c=1
};
const char* play_status_name(int status);

enum CommandKind {
 CMD_NONE=0,
 CMD_MOVE,          // a=veh, b=direction 0..7 (BaseOffset order)
 CMD_GOTO,          // a=veh, b=x, c=y
 CMD_ROAD_TO,       // a=veh, b=x, c=y, d=1 for mag tube
 CMD_TERRAFORM,     // a=veh, b=FormerItem
 CMD_BUILD_BASE,    // a=veh, text=name or empty
 CMD_SENTRY,        // a=veh
 CMD_HOLD,          // a=veh
 CMD_SKIP,          // a=veh
 CMD_WAKE,          // a=veh
 CMD_DISBAND,       // a=veh
 CMD_SET_HOME,      // a=veh
 CMD_RETURN_HOME,   // a=veh
 CMD_UPGRADE,       // a=veh, b=new unit id (-1: best available)
 CMD_ODDS,          // a=veh, b=x, c=y: battle preview, nothing changes
 CMD_ATTACK,        // a=veh, b=x, c=y
 CMD_PRODUCE,       // a=base, b=item (unit id >= 0, facility -id)
 CMD_QUEUE_ADD,     // a=base, b=item
 CMD_QUEUE_REMOVE,  // a=base, b=slot (>= 1)
 CMD_QUEUE_MOVE,    // a=base, b=from, c=to
 CMD_HURRY,         // a=base (buys the whole item)
 CMD_RESEARCH,      // a=tech id, c=1 confirms a change that halves progress
 CMD_SOCIAL,        // a=category 0..3, b=model 0..3
 CMD_DESIGN,        // a=chassis, b=weapon, c=armor, d=reactor (1..), e=abilities
 CMD_DIPLO,         // a=other faction, b=proposal (DiploProposal)
 CMD_END_TURN,
 CMD_EDIT_UNIT,     // scenario editor: a=faction, b=unit id, c=x, d=y (tests)
 CMD_COUNT,
};
struct Command { int kind,a,b,c,d,e;char text[32]; };
Command command(int kind,int a=0,int b=0,int c=0,int d=0,int e=0);
/* Text form used by play scripts: "move 3 2", "attack 3 10 12",
 * "produce 0 -3", "research 12", "endturn", ... (see play_session.cpp).
 * Returns false for an unknown word or missing numbers. */
bool parse_command(const char* line,Command& out);
const char* command_name(int kind);

/* One line for the message log. kind is alpha::MessageKind as an integer. */
struct PlayMessage { int32_t turn;uint8_t kind;char text[120]; };

struct UnitInfo {
 bool valid;int id,unit_id,faction,x,y,damage,moves_left,speed,order,home_base;
 bool colony,former,combat,own;char name[32];
};
struct TurnInfo { int status,faction,turn,year,events; };

class SessionImpl;
class Session {
public:
 Session();~Session();
 Session(const Session&)=delete;Session& operator=(const Session&)=delete;
 /* Rules: engine_load_rules(dir, expansion). "" is the current directory. */
 int load_rules(const char* dir,int expansion);
 const char* rules_error()const;
 /* Load an original SAV/SC container (load_daemon_bytes). */
 int load_game(const uint8_t* bytes,size_t length);
 /* Start a new game (WP7 engine_new_game) and run the start of turn 1.
  * Values follow engine::NewGameSettings; PLAY_UNAVAILABLE without WP7. */
 int new_game(uint32_t seed,int faction,int difficulty,int map_size);
 /* The title screen's ui_newgame flow builds the world itself on the bound
  * state; this then starts turn 1 exactly like new_game(). */
 int adopt_new_game();
 static bool new_game_available();
 static bool diplomacy_available();
 /* save_daemon_bytes. */
 int save_game(std::vector<uint8_t>& out);
 /* game_write_bytes: the current state for OriginalWorld (no seed change). */
 int view_bytes(std::vector<uint8_t>& out);
 bool loaded()const;
 bool game_over()const;
 /* Bind this session's state (other code may have bound another). */
 void bind();

 int run(const Command&);
 const TurnInfo& last_turn()const;

 /* Messages since the last take, oldest first; take() clears them. */
 size_t message_count()const;
 const PlayMessage* message(size_t i)const;
 void take_messages();

 int player()const;
 int turn()const;
 int year()const;
 int credits()const;
 int vehicle_count()const;
 int base_count()const;
 bool unit(int veh,UnitInfo&)const;
 /* Next own unit after veh that has moves left and no standing order (-1). */
 int next_unit(int veh)const;
 /* First own unit on (x, y), or -1. */
 int unit_at(int x,int y)const;
 int base_at(int x,int y)const;
 /* Target tile of a step, or false off the map. */
 bool step_target(int veh,int direction,int& x,int& y)const;
 /* Enemy tile (unit or base of a faction without a pact)? */
 bool hostile_at(int x,int y)const;
 /* The best upgrade target for a unit, or -1. */
 int upgrade_target(int veh)const;

 /* Panel models (see the ui_*.h files). */
 bool base_production(int base,BaseProductionModel&)const;
 bool research(ResearchPanelModel&)const;
 bool social(SocialPanelModel&)const;
 bool design(DesignPanelModel&)const;
 /* Name and cost of the design currently selected in the model. */
 void design_preview(DesignPanelModel&)const;
 const BattlePanelModel& battle()const;
 bool diplomacy(int other,DiploPanelModel&)const;
 /* Factions the player has met (diplomacy targets), ascending ids. */
 int contacts(int* ids,int max)const;

 /* Digest of the engine state (save bytes), for tests. */
 uint32_t digest();
private:
 SessionImpl* impl_;
};

/* Session shared by the title screen and the map window. */
Session& session();

}
}
#endif
