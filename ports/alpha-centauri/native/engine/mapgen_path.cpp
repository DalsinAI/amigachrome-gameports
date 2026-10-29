/* SPDX-License-Identifier: GPL-3.0-or-later
 * World generation support routines that Thinker Mod takes from terranx.exe
 * (WP7):
 * - map_wipe: Thinker Mod src/savegame.cpp at
 *   dabe62f2d1abba03de40f3dd3048494843f6a9af (MIT; see engine/THINKER-LICENSE);
 * - Path_continents (Path::continents, Path::continent, Path::merge and
 *   Path::make_abstract): ported from OpenSMACX src/path.cpp at 2f7f4e24
 *   (GPL-3.0), original offsets 0x59C790, 0x59C520, 0x59C340, 0x59C200;
 * - fixup_landmarks and world_site: new code. Neither routine is in any
 *   upstream source; see ENGINE-WP7.md for what they do here.
 */
#include "engine.h"
#include "mapgen.h"
#include "wp7_deps.h"

namespace alpha {
namespace engine {

static int game_halted = 1;
static int world_add_temperature = 0;
int* GameHalted = &game_halted;
int* WorldAddTemperature = &world_add_temperature;

/*
Reset the map to a blank state. Original doesn't wipe unk_1 and owner fields.
This is simplified by zeroing all fields first and then setting specific fields.
*/
void __cdecl map_wipe() {
    *MapSeaLevel = 0;
    *MapSeaLevelCouncil = 0;
    *MapLandmarkCount = 0;
    *MapRandomSeed = random(0x7FFF) + 1;
    memset(*MapTiles, 0, *MapAreaTiles * sizeof(MAP));
    for (int i = 0; i < *MapAreaTiles; ++i) {
        (*MapTiles)[i].climate = 0x20;
        (*MapTiles)[i].contour = 20;
        (*MapTiles)[i].val2 = 0xF;
        (*MapTiles)[i].owner = -1;
    }
    reset_state();
    // Original: mapdata.clear() and mapnodes.clear() are part of reset_state
    // here (WP6 owns those AI tables).
}

/* ---- Path::continents (OpenSMACX) ---- */

static void abstract_set(int x, int y, uint8_t region) {
    (*MapAbstract)[(x >> 1) + y * (*MapAbstractX >> 1)] = region;
}

static void path_merge(int region_old, int region_new) {
    Continents[region_new].tile_count += Continents[region_old].tile_count;
    Continents[region_old].tile_count = 0;
    for (int i = 0; i < *MapAreaTiles; i++) {
        if ((int)(*MapTiles)[i].region == region_old) {
            (*MapTiles)[i].region = (uint8_t)region_new;
        }
    }
}

static void path_continent(std::vector<int16_t>& xt, std::vector<int16_t>& yt,
int x, int y, int region) {
    Continents[region].tile_count = 0;
    size_t index1 = 0;
    size_t index2 = 0;
    int fresh_water_count = 0;
    xt[index1] = (int16_t)x;
    yt[index1++] = (int16_t)y;
    region_set(x, y, (uint8_t)region);
    bool is_ocean_tile = is_ocean(x, y);
    do {
        int x_it = xt[index2];
        int y_it = yt[index2++];
        Continents[region].tile_count++;
        for (int i = 0; i < 8; i++) {
            int x_radius = wrap(x_it + BaseOffsetX[i]);
            int y_radius = y_it + BaseOffsetY[i];
            if (on_map(x_radius, y_radius) && y_it && y_it != *MapAreaY - 1) {
                bool is_ocean_rad = is_ocean(x_radius, y_radius);
                if (is_ocean_rad == is_ocean_tile && !region_at(x_radius, y_radius)) {
                    if (is_ocean_tile && bit2_at(x_radius, y_radius) & LM_FRESH && is_ocean_rad) {
                        fresh_water_count++;
                    }
                    region_set(x_radius, y_radius, (uint8_t)region);
                    xt[index1] = (int16_t)x_radius;
                    yt[index1++] = (int16_t)y_radius;
                }
            }
        }
    } while (index2 && index1 != index2);
    if (fresh_water_count) {
        // land locked?
        bool is_fresh_water = fresh_water_count >= ((Continents[region].tile_count * 3) / 4);
        for (int y_it = 0; y_it < *MapAreaY; y_it++) {
            for (int x_it = y_it & 1; x_it < *MapAreaX; x_it += 2) {
                if (region_at(x_it, y_it) == region) {
                    bit2_set(x_it, y_it, LM_FRESH, is_fresh_water);
                }
            }
        }
    }
}

static void path_make_abstract() {
    for (int y = 0; y < *MapAbstractY; y++) {
        for (int x = y & 1; x < *MapAbstractX; x += 2) {
            uint8_t region = 0;
            for (int i = 0; i < TableRange[1]; i++) {
                int x_radius = wrap(x * 5 + TableOffsetX[i]);
                int y_radius = y * 5 + TableOffsetY[i];
                if (on_map(x_radius, y_radius) && !is_ocean(x_radius, y_radius)) {
                    region = (uint8_t)region_at(x_radius, y_radius);
                    break;
                }
            }
            abstract_set(x, y, region);
        }
    }
}

/*
Populate and set up all the continents and path tables.
Original: Path::continents(Paths), 0x59C790. The flood fill queues are
local vectors sized to the map instead of the Path object tables.
*/
void __cdecl Path_continents() {
    if (!*MapTiles || *MapAreaTiles <= 0) {
        return;
    }
    std::vector<int16_t> xt((size_t)*MapAreaTiles + 1);
    std::vector<int16_t> yt((size_t)*MapAreaTiles + 1);
    for (int i = 0; i < *MapAreaTiles; i++) {
        (*MapTiles)[i].region = 0;
    }
    // Fix: OpenSMACX does not clear tile counts of regions that are not
    // visited again; stale counts from a previous map would leak into the
    // smallest-region search and the pole regions.
    for (int i = 0; i < MaxRegionNum; i++) {
        Continents[i].tile_count = 0;
    }
    int y_south_pole = *MapAreaY - 1;
    int ocean_region = 64;
    int land_region = 0;
    for (int y = 1; y < y_south_pole; y++) {
        for (int x = y & 1; x < *MapAreaX; x += 2) {
            if (!region_at(x, y)) {
                int region_current;
                int region_min;
                int region_max;
                if (is_ocean(x, y)) {
                    ocean_region++;
                    region_min = 64;
                    region_max = 126;
                    region_current = clamp(ocean_region, region_min, region_max);
                } else {
                    land_region++;
                    region_min = 0;
                    region_max = 62;
                    region_current = clamp(land_region, region_min, region_max);
                }
                path_continent(xt, yt, x, y, region_current);
                if (region_current == region_max) {
                    int tiles = Continents[region_current].tile_count;
                    int search_region = -1;
                    for (int i = region_min; i < region_max; i++) {
                        if (Continents[i].tile_count < tiles) {
                            tiles = Continents[i].tile_count;
                            search_region = i;
                        }
                    }
                    if (search_region >= 0) {
                        path_merge(search_region, region_max + 1);
                        path_merge(region_current, search_region);
                    } else {
                        path_merge(region_current, region_max + 1);
                    }
                }
            }
        }
    }
    for (int x = 0; x < *MapAreaX; x += 2) { // north pole
        uint8_t pole_region = is_ocean(x, 0) ? 127 : 63;
        region_set(x, 0, pole_region);
        Continents[pole_region].tile_count++;
    }
    if (y_south_pole > 0) { // Fix: a one-row map has no separate south pole row
        for (int x = y_south_pole & 1; x < *MapAreaX; x += 2) { // south pole
            uint8_t pole_region = is_ocean(x, y_south_pole) ? 127 : 63;
            region_set(x, y_south_pole, pole_region);
            Continents[pole_region].tile_count++;
        }
    }
    int most_tiles = 0;
    int total_tiles = 0;
    for (int i = 1; i < 63; i++) {
        int tiles = Continents[i].tile_count;
        total_tiles += tiles;
        if (tiles > most_tiles) {
            most_tiles = tiles;
        }
    }
    *GameState = (most_tiles >= ((total_tiles * 4) / 5))
        ? *GameState | STATE_UNK_100 : *GameState & ~STATE_UNK_100;
    for (int i = 0; i < MaxRegionLandNum; i++) {
        memset(Continents[i].sea_coasts, 0, sizeof(Continents[i].sea_coasts));
    }
    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y & 1; x < *MapAreaX; x += 2) {
            int region = region_at(x, y);
            if (region < MaxRegionLandNum) {
                for (int i = 0; i < 8; i++) {
                    int x_radius = wrap(x + BaseOffsetX[i]);
                    int y_radius = y + BaseOffsetY[i];
                    if (on_map(x_radius, y_radius)) {
                        int region_rad = region_at(x_radius, y_radius);
                        if (region_rad >= MaxRegionLandNum) {
                            int bit = region_rad - MaxRegionLandNum;
                            Continents[region].sea_coasts[bit / 8] |= (uint8_t)(1 << (bit & 7));
                            i += 2 - (i & 1);
                        }
                    }
                }
            }
        }
    }
    path_make_abstract();
}

/*
New code: fixup_landmarks (0x592940) is an executable routine with no
upstream source. It runs after a map is read and after world generation.
This version keeps the landmark name table consistent with the map: entries
outside the map are removed, names are terminated, and the count is bounded
to the 64 table slots. A table that is already consistent is not changed,
so loading and saving a game stays byte-identical.
*/
void __cdecl fixup_landmarks() {
    int count = clamp(*MapLandmarkCount, 0, 64);
    int n = 0;
    for (int i = 0; i < count; i++) {
        Landmark lm = Landmarks[i];
        lm.name[sizeof(lm.name) - 1] = '\0';
        if (lm.x < 0 || lm.x >= *MapAreaX || lm.y < 0 || lm.y >= *MapAreaY) {
            continue;
        }
        if (n != i || memcmp(&Landmarks[n], &lm, sizeof(lm))) {
            Landmarks[n] = lm;
        }
        n++;
    }
    for (int i = n; i < count; i++) {
        memset(&Landmarks[i], 0, sizeof(Landmark));
    }
    if (*MapLandmarkCount != n) {
        *MapLandmarkCount = n;
    }
}

/*
New code: world_site (exe-only, AI colonization priority 0..15 stored in
the tile site bits). The original weighting is unknown; this rates the
21-tile radius of a land tile by flat or rolling dry land, rainfall, rivers
and resource bonuses, and returns 0 for ocean, rocky and fungus tiles.
toggle is accepted for the original signature and ignored.
*/
int __cdecl world_site(int x, int y, int UNUSED(toggle)) {
    MAP* sq = mapsq(x, y);
    if (!sq || is_ocean(sq) || sq->is_rocky() || sq->is_fungus()) {
        return 0;
    }
    int score = 0;
    for (auto& m : iterate_tiles(x, y, 0, 21)) {
        if (is_ocean(m.sq)) {
            score += is_ocean_shelf(m.sq) ? 1 : 0;
            continue;
        }
        if (m.sq->is_rocky() || m.sq->is_fungus()) {
            continue;
        }
        score += 1 + (m.sq->is_rainy() ? 2 : (m.sq->is_moist() ? 1 : 0));
        if (m.sq->items & BIT_RIVER) {
            score += 2;
        }
        if (m.sq->items & BIT_BONUS_RES) {
            score += 2;
        }
    }
    return clamp(score / 6, 0, 15);
}

} // namespace engine
} // namespace alpha
