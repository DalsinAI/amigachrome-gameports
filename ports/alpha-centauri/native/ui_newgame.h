/* SPDX-License-Identifier: GPL-3.0-or-later
 * New game screens (WP7, new code): faction pick, planet and rules settings,
 * and the planetfall screen. The screens are a small state machine over the
 * engine (engine_new_game); they hold no drawing code. A frontend calls
 * three hooks: newgame_key() for input, newgame_view() for the lines to draw
 * with its own text renderer, and newgame_open()/newgame_done() to enter and
 * leave the flow.
 */
#pragma once

#include "engine/engine.h"
#include "engine/newgame.h"

#include <string>
#include <vector>

namespace alpha {
namespace ui {

enum NewGameScreen {
    NG_FACTION = 0,    // pick the player's faction (MFactions[1..7])
    NG_SETTINGS = 1,   // difficulty, planet size and conditions, rules, seed
    NG_PLANETFALL = 2, // landing announcement after the world is built
    NG_DONE = 3,       // game started; the frontend switches to the map
    NG_CANCELLED = 4,  // left the flow from the faction screen
};

enum NewGameKey {
    NGK_UP = 0,
    NGK_DOWN = 1,
    NGK_LEFT = 2,     // previous value of the selected setting
    NGK_RIGHT = 3,    // next value of the selected setting
    NGK_ENTER = 4,    // choose / start / continue
    NGK_BACK = 5,     // previous screen (cancel on the faction screen)
};

/* Rows of the settings screen. */
enum NewGameSetting {
    NGS_DIFFICULTY = 0,
    NGS_SIZE,
    NGS_OCEAN,
    NGS_EROSION,
    NGS_NATIVE,
    NGS_CLOUD,
    NGS_ORBIT,
    NGS_LOOK_FIRST,
    NGS_BLIND_RESEARCH,
    NGS_SEED,
    NGS_START,
    NGS_COUNT,
};

struct NewGameLine {
    std::string text;
    bool selected;
    bool heading;
};

struct NewGameView {
    NewGameScreen screen;
    std::string title;
    std::vector<NewGameLine> lines;
};

struct NewGameState {
    NewGameScreen screen = NG_FACTION;
    int row = 0;                      // selected row on the current screen
    int difficulty = 3;
    int size = 2;                     // 0..4, 5 = random
    int ocean = 1, erosion = 1, native = 1, cloud = 1, orbit = 1; // 0..2, 3 = random
    bool look_first = false;
    bool blind_research = true;
    uint32_t seed = 1;
    int player_faction = 1;
    int start_status = -1;            // engine_new_game result, -1 before start
    // Planetfall text captured from the engine's PLANETFALL popup.
    std::string planetfall_title, planetfall_leader, planetfall_noun;
    int landing_x = -1, landing_y = -1;
};

/* Enter the flow; seed is the frontend's start value (a timer, for example). */
void newgame_open(NewGameState& state, uint32_t seed);
/* Handle one key. Returns true when the view changed. On the settings
 * screen, ENTER on the Start row builds the world with engine_new_game. */
bool newgame_key(NewGameState& state, NewGameKey key);
/* Lines to draw for the current screen. */
NewGameView newgame_view(const NewGameState& state);
/* Settings passed to engine_new_game for the current choices. */
engine::NewGameSettings newgame_settings(const NewGameState& state);
/* True once the flow has ended (NG_DONE or NG_CANCELLED). */
bool newgame_done(const NewGameState& state);

} // namespace ui
} // namespace alpha
