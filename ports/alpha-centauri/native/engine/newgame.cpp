/* SPDX-License-Identifier: GPL-3.0-or-later
 * Starting a new game (WP7). alien_start is Thinker Mod src/game.cpp at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af (MIT; see engine/THINKER-LICENSE).
 * engine_new_game, planetfall, crash_landing and landing_site are new code:
 * engine_new_game follows the order of Thinker config_game(1), map_menu and
 * control_game; planetfall and crash_landing are executable routines with no
 * upstream source (see ENGINE-WP7.md).
 */
#include "engine.h"
#include "mapgen.h"
#include "newgame.h"
#include "setup.h"
#include "turn.h"
#include "integration.h"

namespace alpha {
namespace engine {

void __cdecl alien_start() {
    if (!*CurrentTurn && *ExpansionEnabled && !*PbemActive && !*MultiplayerActive) {
        const int player_id = MapWin->cOwner;
        if (!MFactions[player_id].is_alien()) {
            if (_stricmp(MFactions[player_id].filename, "FUNGBOY")) {
                for (int faction_id = 1; faction_id < MaxPlayerNum; ++faction_id) {
                    if (is_human(faction_id)) {
                        continue;
                    }
                    bool alien = MFactions[faction_id].is_alien();
                    bool cult = !_stricmp(MFactions[faction_id].filename, "FUNGBOY");
                    if ((!alien && !cult)
                    || (conf.cult_early_start && cult)
                    || (conf.alien_early_start && alien)) {
                        continue;
                    }
                    FactionStatus[0] &= ~(1 << faction_id);
                    FactionStatus[1] &= ~(1 << faction_id);
                    for (int base_id = *BaseCount - 1; base_id >= 0; --base_id) {
                        if (Bases[base_id].faction_id == faction_id) {
                            mod_base_kill(base_id);
                        }
                    }
                    for (int veh_id = *VehCount - 1; veh_id >= 0; --veh_id) {
                        if (veh_id < *VehCount && Vehs[veh_id].faction_id == faction_id) {
                            veh_kill(veh_id);
                        }
                    }
                    Factions[faction_id].base_count = 0;
                    Factions[faction_id].player_flags = -1;
                    Factions[faction_id].diff_level = -1;
                }
            }
        }
    }
}

bool landing_site(int faction_id, int* x, int* y) {
    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].faction_id == faction_id) {
            *x = Bases[i].x;
            *y = Bases[i].y;
            return true;
        }
    }
    for (int i = 0; i < *VehCount; i++) {
        if (Vehs[i].faction_id == faction_id) {
            *x = Vehs[i].x;
            *y = Vehs[i].y;
            return true;
        }
    }
    return false;
}

static void landing_says(int faction_id, int x, int y) {
    parse_says(0, get_title(faction_id), -1, -1);
    parse_says(1, get_name(faction_id), -1, -1);
    parse_says(2, get_noun(faction_id), -1, -1);
    parse_num(0, x);
    parse_num(1, y);
}

void __cdecl planetfall(int faction_id) {
    int x = -1, y = -1;
    if (faction_id <= 0 || faction_id >= MaxPlayerNum || !landing_site(faction_id, &x, &y)) {
        return;
    }
    landing_says(faction_id, x, y);
    popp(ScriptFile, "PLANETFALL", 0, nullptr, nullptr);
}

void __cdecl crash_landing(int faction_id) {
    int x = -1, y = -1;
    if (faction_id <= 0 || faction_id >= MaxPlayerNum || !landing_site(faction_id, &x, &y)) {
        return;
    }
    MapWin->TileX = x;
    MapWin->TileY = y;
    landing_says(faction_id, x, y);
    popp(ScriptFile, "CRASHLANDING", 0, nullptr, nullptr);
    draw_map(1);
}

static bool valid_setting(int value) {
    return value >= -1 && value <= 2;
}

int engine_new_game(const NewGameSettings& s) {
    if (!engine_state() || s.player_faction < 1 || s.player_faction >= MaxPlayerNum
    || s.difficulty < 0 || s.difficulty > 5 || s.map_size < -1 || s.map_size >= WorldSizeCount
    || !valid_setting(s.ocean) || !valid_setting(s.erosion) || !valid_setting(s.native)
    || !valid_setting(s.cloud) || !valid_setting(s.orbit)
    || (s.human_mask & 1) || s.human_mask >= (1u << MaxPlayerNum)) {
        return 1;
    }
    bool custom = s.map_width > 0 && s.map_height > 0;
    if (custom && (s.map_width < 16 || s.map_width > 512 || s.map_height < 16 || s.map_height > 512)) {
        return 1;
    }
    // Original: the timer sources of the executable; here every generator
    // starts from the settings seed so a game is reproducible.
    game_srand(s.seed);
    random_reseed(s.seed);
    next_rand_seed(s.seed, s.seed ^ 0x9E3779B9u);
    engine_reset_runtime();
    *GameHalted = 1;
    *MultiplayerActive = 0;
    *PbemActive = 0;
    *CurrentTurn = 0;
    *ControlTurnMove = 0;

    // config_game: difficulty, rules, faction seat.
    *DiffLevel = s.difficulty;
    *GameRules = s.rules;
    *GameMoreRules = s.more_rules;
    if (!*DiffLevel) {
        *GameRules |= RULES_BLIND_RESEARCH;
    }
    *GameState &= ~(STATE_RAND_FAC_LEADER_PERSONALITIES | STATE_RAND_FAC_LEADER_SOCIAL_AGENDA);
    if (s.random_personalities) {
        *GameState |= STATE_RAND_FAC_LEADER_PERSONALITIES;
    }
    if (s.random_agendas) {
        *GameState |= STATE_RAND_FAC_LEADER_SOCIAL_AGENDA;
    }
    const int plr_id = s.player_faction;
    if (*ExpansionEnabled && MFactions[plr_id].is_alien()) {
        *GameRules &= ~RULES_BLIND_RESEARCH;
        *GameRules |= RULES_NO_UNITY_SURVEY|RULES_INTENSE_RIVALRY;
    }
    FactionStatus[0] = (uint8_t)(s.human_mask ? s.human_mask | (1u << plr_id) : (1u << plr_id));
    MapWin->cOwner = plr_id;
    *CurrentPlayerFaction = plr_id;

    // map_menu: planet size and settings.
    int width;
    if (custom) {
        *MapSizePlanet = 2;
        width = s.map_width;
        *MapAreaY = s.map_height;
    } else {
        int size = s.map_size >= 0 ? s.map_size : game_rand() % 5;
        *MapSizePlanet = size;
        width = WorldSizes[size][0];
        *MapAreaY = WorldSizes[size][1];
    }
    *MapAreaX = 2 * width;
    *MapOceanCoverage = s.ocean;
    *MapLandCoverage = s.ocean >= 0 ? 2 - s.ocean : -1;
    *MapErosiveForces = s.erosion;
    *MapPlanetaryOrbit = s.orbit;
    *MapCloudCover = s.cloud;
    *MapNativeLifeForms = s.native;
    if (map_init()) {
        return 1;
    }
    engine_notify(EV_MAP_CHANGED);

    // control_game / config_game: build the world and set up the factions.
    *VehCount = 0;
    *BaseCount = 0;
    mod_world_build();
    setup_game(0);
    if (*ExpansionEnabled) {
        alien_start();
    }
    *GameHalted = 0;
    planetfall(MapWin->cOwner);
    return 0;
}

} // namespace engine
} // namespace alpha
