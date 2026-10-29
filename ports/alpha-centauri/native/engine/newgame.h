/* SPDX-License-Identifier: GPL-3.0-or-later
 * Starting a new game without the original setup windows (WP7, new code).
 * engine_new_game() runs the sequence of Thinker Mod config_game(1) and
 * map_menu at dabe62f2d1abba03de40f3dd3048494843f6a9af (MIT; see
 * engine/THINKER-LICENSE): rules and difficulty, map size and planet
 * settings, map_init, mod_world_build, setup_game(0), alien_start and the
 * planetfall notice, on the bound state with the rules already loaded
 * (engine_load_rules). Include engine.h first.
 */
#pragma once

namespace alpha {
namespace engine {

const uint32_t DefaultRules = RULES_VICTORY_COOPERATIVE | RULES_VICTORY_TRANSCENDENCE
    | RULES_BLIND_RESEARCH | RULES_VICTORY_DIPLOMATIC | RULES_VICTORY_ECONOMIC
    | RULES_VICTORY_CONQUEST;
static_assert(DefaultRules == 0x1A0E, "");

/* #WORLDSIZE rows of the original alpha.txt/alphax.txt: width (MapAreaX/2)
 * and height of Tiny, Small, Standard, Large and Huge planets. */
const int WorldSizeCount = 5;
const int WorldSizes[WorldSizeCount][2] = {{24, 48}, {32, 64}, {40, 80}, {50, 100}, {64, 128}};

struct NewGameSettings {
    uint32_t seed = 1;           // seeds game_rand, random, next_rand and the map
    int player_faction = 1;      // 1..7, MapWin->cOwner
    uint32_t human_mask = 0;     // FactionStatus[0]; 0 = only the player
    int difficulty = 3;          // 0 Citizen .. 5 Transcend
    int map_size = 2;            // 0..4 (WorldSizes), -1 random
    int map_width = 0;           // custom size when both are > 0 (16..512)
    int map_height = 0;
    int ocean = 1;               // MapOceanCoverage 0..2, -1 random
    int erosion = 1;             // MapErosiveForces 0..2, -1 random
    int native = 1;              // MapNativeLifeForms 0..2, -1 random
    int cloud = 1;               // MapCloudCover 0..2, -1 random
    int orbit = 1;               // MapPlanetaryOrbit 0..2, -1 random
    uint32_t rules = DefaultRules; // GameRules
    uint32_t more_rules = 0;     // GameMoreRules
    bool random_personalities = false; // STATE_RAND_FAC_LEADER_PERSONALITIES
    bool random_agendas = false;       // STATE_RAND_FAC_LEADER_SOCIAL_AGENDA
};

/* Start a new game on the bound state. Returns 0, or 1 for invalid settings
 * (nothing changed) or a map that could not be allocated. */
int engine_new_game(const NewGameSettings& settings);

/* Thinker game.cpp: remove alien and cult factions that start late. */
void __cdecl alien_start();

/* New code for executable routines without upstream source:
 * planetfall(faction_id) is the landing announcement shown once after setup
 * (popup "PLANETFALL"); crash_landing(faction_id) runs from control_turn on
 * turn 0 (WP1 TurnHooks::crash_landing) and centres the map on the landing
 * site (popup "CRASHLANDING"). Both report the faction and site through
 * parse_says/parse_num: $STR0 title, $STR1 leader, $STR2 faction noun,
 * $NUM0/$NUM1 x/y. */
void __cdecl planetfall(int faction_id);
void __cdecl crash_landing(int faction_id);
/* The landing site of a faction (its first base, else its first unit). */
bool landing_site(int faction_id, int* x, int* y);

} // namespace engine
} // namespace alpha
