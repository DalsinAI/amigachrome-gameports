/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP7-local copies of the Thinker Mod src/map.cpp terrain routines that world
 * generation and start placement call (dabe62f2d1abba03de40f3dd3048494843f6a9af,
 * MIT; see engine/THINKER-LICENSE): alt_set, alt_set_both, world_alt_set,
 * mod_minerals_at, mod_bonus_at, mod_goody_at and mapgen_rand.
 *
 * The shared copies of these routines belong to WP3 (engine_wp.h). World
 * generation runs before the first turn, where alt_set only stores the
 * altitude, so these copies keep new maps and start positions independent
 * of whether WP3 has landed, and identical once it has. On a running game
 * (CurrentTurn > 0) alt_set and world_alt_set forward to the engine_wp.h
 * versions, which also handle submerged bases and units.
 */
#include "engine.h"
#include "mapgen.h"

namespace alpha {
namespace engine {
namespace wp7 {

static int mapgen_rand(int value) {
    if (*MultiplayerActive) {
        return game_randv(value);
    }
    return (value > 1 ? map_rand.get(value) : 0);
}

void alt_set(int x, int y, int altitude) {
    if (*CurrentTurn) {
        engine::alt_set(x, y, altitude); // in-game changes: WP3
        return;
    }
    MAP* sq = mapsq(x, y);
    assert(sq && altitude >= 0 && altitude <= 7);
    // Thinker alt_set: on turn 0 only the altitude level is stored.
    sq->climate = (altitude * 32) | (sq->climate & 0x1F);
    *GameDrawState |= 4;
}

/*
Set both the altitude level and altitude detail for the specified tile.
*/
void alt_set_both(int x, int y, int altitude) {
    alt_set(x, y, altitude);
    if (alt_natural(x, y) != altitude) {
        int offset = AltNatural[altitude + 1] - AltNatural[altitude];
        alt_put_detail(x, y,
            (uint8_t)(*MapSeaLevel + AltNatural[altitude] + mapgen_rand(offset)));
    }
}

void world_alt_set(int x, int y, int altitude, int toggle) {
    if (*CurrentTurn) {
        engine::world_alt_set(x, y, altitude, toggle); // in-game changes: WP3
        return;
    }
    assert(altitude >= 0 && altitude <= 7);
    memset(MapBaseSubmergedCount, 0, MaxPlayerNum * sizeof(int));
    memset(MapBaseIdClosestSubmergedVeh, 0xFF, MaxPlayerNum * sizeof(int));
    if (on_map(x, y)) {
        if (toggle) {
            alt_set_both(x, y, altitude);
        } else {
            alt_set(x, y, altitude);
        }
    }
    MAP* sq;
    int tx = -1;
    int ty = -1;
    if (altitude > 1) {
        int alt = altitude - 1;
        for (int i = 0; i < altitude - 1; ++i, --alt) {
            bool found = false;
            for (int j = TableRange[i]; j < TableRange[i+1]; ++j) {
                if ((sq = next_tile(x, y, j, &tx, &ty)) != NULL && sq->alt_level() < alt) {
                    if (toggle) {
                        alt_set_both(tx, ty, alt);
                        if (sq->anything_at() < 0 && !bonus_at(tx, ty)) {
                            owner_set(tx, ty, -1);
                        }
                    } else {
                        alt_set(tx, ty, alt);
                    }
                    found = true;
                }
            }
            if (!found) {
                break;
            }
        }
    }
    if (altitude <= conf.altitude_limit) {
        int alt = altitude + 1;
        for (int i = 0; i <= conf.altitude_limit - altitude; ++i, ++alt) {
            bool found = false;
            for (int j = TableRange[i]; j < TableRange[i+1]; ++j) {
                if ((sq = next_tile(x, y, j, &tx, &ty)) != NULL && sq->alt_level() > alt) {
                    if (toggle) {
                        alt_set_both(tx, ty, alt);
                        if (sq->anything_at() < 0 && !bonus_at(tx, ty)) {
                            owner_set(tx, ty, -1);
                        }
                    } else {
                        alt_set(tx, ty, alt);
                    }
                    found = true;
                }
            }
            if (!found) {
                break;
            }
        }
    }
}

int minerals_at(int x, int y) {
    if (!y || y == (*MapAreaY - 1)) {
        return 2;
    }
    MAP* sq = mapsq(x, y);
    int alt = sq->alt_level();
    int avg = (x + y) / 2;
    int val1 = (x - avg) / 2 + 2 * ((x - avg) / 2) + 2 * (avg / 2) + *MapRandomSeed;
    int val2 = ((byte)val1 - 2 * (((byte)x - (byte)avg) & 1) - (avg & 1)) & 3;
    int val3 = (alt - 3 >= 0 ? alt - 3 : 3 - alt) - (alt < 3);
    if (val3 == 0) {
        if (val2 == 0) {
            return 1;
        }
        if (val2 == 1 || val2 == 3) {
            return 0;
        }
        if (val2 == 2) {
            return (val1 & 4 ? 2 : 1);
        }
        return ~val2 & 1;
    } else if (val3 == 1) {
        if (!val2) {
            return 0;
        }
        if (!(val2 - 1)) {
            return 1;
        }
        if ((val2 - 1) != 1) {
            return 2;
        }
        return 1;
    } else if (val3 == 2) {
        if (!val2) {
            return 0;
        }
        if (val2 == 1) {
            return 1;
        }
        return 2;
    } else if (val3 == 3) {
        if (val2 < 0 || val2 > 1) {
            return 2;
        }
        return 1;
    } else {
        return ~val2 & 1;
    }
}

int bonus_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    uint32_t bit = sq->items;
    uint32_t alt = sq->alt_level();
    bool has_rsc_bonus = bit & BIT_BONUS_RES;
    if (!has_rsc_bonus && (!*MapRandomSeed || (alt >= ALT_SHORE_LINE
    && !conf.rare_supply_pods && !(*GameRules & RULES_NO_UNITY_SCATTERING)))) {
        return 0;
    }
    int avg = (x + y) >> 1;
    x -= avg;
    int chk = (avg & 3) + 4 * (x & 3);
    if (!has_rsc_bonus && chk != ((*MapRandomSeed + (-5 * (avg >> 2)) - 3 * (x >> 2)) & 0xF)) {
        return 0;
    }
    if (alt < ALT_OCEAN_SHELF) {
        return 0;
    }
    int ret = (alt < ALT_SHORE_LINE) ? chk % 3 + 1 : (chk % 5) & 3;
    if (!ret || bit & BIT_NUTRIENT_RES) {
        if (bit & BIT_ENERGY_RES) {
            return 3; // energy
        }
        return ((bit & BIT_MINERAL_RES) != 0) + 1; // nutrient or mineral
    }
    return ret;
}

int goody_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    uint32_t bit = sq->items;
    uint32_t alt = sq->alt_level();
    if (bit & (BIT_SUPPLY_REMOVE | BIT_MONOLITH)) {
        return 0; // nothing, supply pod already opened or monolith
    }
    if (*GameRules & RULES_NO_UNITY_SCATTERING) {
        return (bit & (BIT_UNK_4000000 | BIT_UNK_8000000)) ? 2 : 0; // near landing site
    }
    if (bit & BIT_SUPPLY_POD) {
        return 1; // supply pod
    }
    if (!*MapRandomSeed) {
        return 0; // nothing
    }
    int avg = (x + y) >> 1;
    int x_diff = x - avg;
    int cmp = (avg & 3) + 4 * (x_diff & 3);
    if (!conf.rare_supply_pods && alt >= ALT_SHORE_LINE) {
        if (cmp == ((-5 * (avg >> 2) - 3 * (x_diff >> 2) + *MapRandomSeed) & 0xF)) {
            return 2;
        }
    }
    return cmp == ((11 * (avg / 4) + 61 * (x_diff / 4) + *MapRandomSeed + 8) & 0x1F); // 0 or 1
}

} // namespace wp7
} // namespace engine
} // namespace alpha
