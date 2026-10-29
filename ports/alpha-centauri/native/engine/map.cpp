/* SPDX-License-Identifier: GPL-3.0-or-later
 * Map accessors ported verbatim from Thinker Mod src/map.cpp (plus
 * iterate_tiles/nearby_items from src/path.cpp and strcpy_n from
 * src/strings.cpp) at dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * Changes: memmove_s -> memmove; namespace. Extracted as whole
 * functions so the bodies match upstream; edit by hand from here on.
 */
#include "engine.h"

namespace alpha {
namespace engine {

MAP* next_tile(int x, int y, size_t offset, int* tx, int* ty) {
    *tx = wrap(x + TableOffsetX[offset]);
    *ty = y + TableOffsetY[offset];
    return mapsq(*tx, *ty);
}

MAP* mapsq(int x, int y) {
    if (x >= 0 && y >= 0 && x < *MapAreaX && y < *MapAreaY && !((x + y)&1)) {
        return &((*MapTiles)[ (x + *MapAreaX * y)/2 ]);
    } else {
        return NULL;
    }
}

int wrap(int x) {
    if (!map_is_flat()) {
        if (x >= 0) {
            if (x >= *MapAreaX) {
                x -= *MapAreaX;
            }
        } else {
            x += *MapAreaX;
        }
    }
    return x;
}

int x_dist(int x1, int x2) {
    int dx = abs(x1 - x2);
    if (!map_is_flat() && dx > *MapHalfX) {
        dx = *MapAreaX - dx;
    }
    return dx;
}

int map_range(int x1, int y1, int x2, int y2) {
    int dx = abs(x1 - x2);
    int dy = abs(y1 - y2);
    if (!map_is_flat() && dx > *MapHalfX) {
        dx = *MapAreaX - dx;
    }
    return (dx + dy)/2;
}

int vector_dist(int x1, int y1, int x2, int y2) {
    int dx = abs(x1 - x2);
    int dy = abs(y1 - y2);
    if (!map_is_flat() && dx > *MapHalfX) {
        dx = *MapAreaX - dx;
    }
    return max(dx, dy) - ((((dx + dy) / 2) - min(dx, dy) + 1) / 2);
}

int min_range(const Points& S, int x, int y) {
    int v = 9999;
    for (auto& p : S) {
        v = min(v, map_range(x, y, p.x, p.y));
    }
    return v;
}

int min_vector(const Points& S, int x, int y) {
    int v = 9999;
    for (auto& p : S) {
        v = min(v, vector_dist(x, y, p.x, p.y));
    }
    return v;
}

int avg_range(const Points& S, int x, int y) {
    size_t n = 0;
    size_t sum = 0;
    for (auto& p : S) {
        sum += map_range(x, y, p.x, p.y);
        n++;
    }
    return (n > 0 ? (sum << 8) / n : 0);
}

bool is_ocean(MAP* sq) {
    return (!sq || (sq->climate >> 5) < ALT_SHORE_LINE);
}

bool is_ocean(BASE* base) {
    return is_ocean(mapsq(base->x, base->y));
}

bool is_ocean(int x, int y) {
    return is_ocean(mapsq(x, y));
}

bool is_ocean_shelf(MAP* sq) {
    return (sq && (sq->climate >> 5) == ALT_OCEAN_SHELF);
}

bool is_shore_level(MAP* sq) {
    return (sq && (sq->climate >> 5) == ALT_SHORE_LINE);
}

bool map_is_flat() {
    return *MapToggleFlat & 1;
}

bool map_is_known(int faction_id) {
    assert(faction_id >= 0 && faction_id < MaxPlayerNum);
    return Factions[faction_id].player_flags & PFLAG_MAP_REVEALED;
}

bool adjacent_region(int x, int y, int owner, int threshold, bool ocean) {
    assert(mapsq(x, y));
    for (auto& m : iterate_tiles(x, y, 1, 9)) {
        if (is_ocean(m.sq) != ocean) {
            continue;
        }
        if ((owner < 0 || m.sq->owner < 0 || m.sq->owner == owner) && !bad_reg(m.sq->region)
        && Continents[m.sq->region].tile_count >= threshold) {
            return true;
        }
    }
    return false;
}

int __cdecl is_sensor(int x, int y) {
    MAP* sq = mapsq(x, y);
    if (sq && sq->items & BIT_SENSOR) {
        return 1;
    }
    int base_dist = 9999;
    int base_id = -1;
    for (int i = 0, cnt = *BaseCount; i < cnt; ++i) {
        int dist = vector_dist(x, y, Bases[i].x, Bases[i].y);
        if (dist <= base_dist) {
            base_dist = dist;
            base_id = i;
        }
    }
    if (base_id < 0 || !has_fac_built(FAC_GEOSYNC_SURVEY_POD, base_id)) {
        return 0;
    }
    BASE* base = &Bases[base_id];
    int dx = x_dist(x, base->x);
    int dy = abs(y - base->y);
    return (dx == 0 || dx == 2) && (dy == 0 || dy == 2) ? 2 : 0;
}

int __cdecl is_known(int x, int y, int faction_id) {
    MAP* sq;
    assert(faction_id >= 0 && faction_id < MaxPlayerNum);
    return faction_id >= 0 && faction_id < MaxPlayerNum
        && (Factions[faction_id].player_flags & PFLAG_MAP_REVEALED
        || ((sq = mapsq(x, y)) && sq->visibility & (1 << faction_id)));
}

int __cdecl is_coast(int x, int y, bool is_base_radius) {
    int radius = is_base_radius ? 21 : 9;
    for (int i = 1; i < radius; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        MAP* sq = mapsq(x2, y2);
        if (sq && is_ocean(sq)) {
            return i; // Must return index to the first ocean tile
        }
    }
    return 0;
}

int __cdecl is_port(int base_id, bool is_base_radius) {
    return is_coast(Bases[base_id].x, Bases[base_id].y, is_base_radius);
}

int __cdecl on_map(int x, int y) {
    assert(!((x + y)&1)); // Original version does not check this condition
    return x >= 0 && x < *MapAreaX && y >= 0 && y < *MapAreaY;
}

/*
Validate region bounds. Bad regions include: 0, 63, 64, 127, 128.
*/
int __cdecl bad_reg(int region) {
    return (region & RegionBounds) == RegionBounds || !(region & RegionBounds);
}

void __cdecl rebuild_base_bits() {
    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y&1; x < *MapAreaX; x += 2) {
            bit_set(x, y, BIT_BASE_IN_TILE, 0);
            for (int base_id = 0; base_id < *BaseCount; base_id++) {
                if (Bases[base_id].x == x && Bases[base_id].y == y) {
                    bit_set(x, y, BIT_BASE_IN_TILE, 1);
                    owner_set(x, y, Bases[base_id].faction_id);
                    break;
                }
            }
        }
    }
}

void __cdecl rebuild_vehicle_bits() {
    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y&1; x < *MapAreaX; x += 2) {
            bit_set(x, y, BIT_VEH_IN_TILE, 0);
            for (int veh_id = 0; veh_id < *VehCount; veh_id++) {
                if (Vehs[veh_id].x == x && Vehs[veh_id].y == y) {
                    bit_set(x, y, BIT_VEH_IN_TILE, 1);
                    if (!(bit_at(x, y) & BIT_BASE_IN_TILE)) {
                        owner_set(x, y, Vehs[veh_id].faction_id);
                    }
                    break;
                }
            }
        }
    }
}

void __cdecl owner_set(int x, int y, int faction_id) {
    MAP* sq = mapsq(x, y);
    if (sq) {
        sq->val2 &= 0xF0;
        sq->val2 |= faction_id & 0xF;
    }
}

void __cdecl site_set(int x, int y, uint8_t site) {
    MAP* sq = mapsq(x, y);
    if (sq) {
        sq->val2 &= 0x0F;
        sq->val2 |= site << 4;
    }
}

void __cdecl region_set(int x, int y, uint8_t region) {
    MAP* sq = mapsq(x, y);
    if (sq) {
        sq->region = region;
    }
}

int __cdecl region_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    return sq ? sq->region : 0;
}

int __cdecl base_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    if (sq && sq->is_base()) {
        if (*BaseCount <= 0) {
            // Removed unnecessary debug code from here
            rebuild_base_bits();
            return -1;
        }
        for (int i = 0, cnt = *BaseCount; i < cnt; ++i) {
            if (Bases[i].x == x && Bases[i].y == y) {
                return i;
            }
        }
        assert(0);
    }
    return -1;
}

int __cdecl alt_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    if (sq) {
        return sq->alt_level();
    }
    return 0;
}

int __cdecl alt_detail_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    if (sq) {
        return sq->contour;
    }
    return 0;
}

void __cdecl alt_put_detail(int x, int y, uint8_t detail) {
    MAP* sq = mapsq(x, y);
    if (sq) {
        mapsq(x, y)->contour = detail;
    }
}

/*
Calculate the altitude of the specified tile.
Return Value: altitude level on a scale from 0 (ocean trench) to 6 (mountain tops)
*/
int __cdecl alt_natural(int x, int y) {
    int detail = alt_detail_at(x, y) - *MapSeaLevel;
    int alt = conf.altitude_limit;
    while (alt > 0 && detail < AltNatural[alt]) {
        alt--;
    }
    return alt;
}

/*
Calculate the elevation of the specified tile.
Return Value: elevation meters (bounded to: -3000 to 3500)
*/
int __cdecl elev_at(int x, int y) {
    int contour = alt_detail_at(x, y);
    int elev = 50 * (contour - ElevDetail[3] - *MapSeaLevel);
    elev += (contour <= ElevDetail[alt_at(x, y)]) ? 10
        : (x * 113 + y * 217 + *MapSeaLevel * 301) % 50;
    return clamp(elev, -3000, conf.altitude_limit > ALT_THREE_ABOVE_SEA ? 4500 : 3500);
}

void __cdecl world_alt_put_detail(int x, int y) {
    alt_put_detail(x, y, (uint8_t)AltNatural[3]);
}

int __cdecl temp_at(int x, int y) {
    return mapsq(x, y)->climate & 7;
}

void __cdecl temp_set(int x, int y, uint8_t value) {
    MAP* sq = mapsq(x, y);
    sq->climate &= 0xF8;
    sq->climate |= value & 7;
}

void __cdecl climate_set(int x, int y, uint8_t rainfall) {
    MAP* sq = mapsq(x, y);
    sq->climate &= 0xE7;
    sq->climate |= (rainfall & 3) << 3;
    sq->landmarks |= LM_UNK_400000;
    *GameDrawState |= 1;
}

void __cdecl rocky_set(int x, int y, uint8_t rocky) {
    MAP* sq = mapsq(x, y);
    sq->val3 &= 0x3F;
    sq->val3 |= rocky << 6;
    sq->landmarks |= LM_UNK_400000;
    *GameDrawState |= 1;
}

int __cdecl using_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    return sq->val3 & 7;
}

void __cdecl using_set(int x, int y, int faction_id) {
    MAP* sq = mapsq(x, y);
    sq->val3 &= 0xF8;
    sq->val3 |= faction_id & 7;
}

int __cdecl lock_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    return (sq->val3 >> 3) & 7;
}

void __cdecl lock_set(int x, int y, int faction_id) {
    MAP* sq = mapsq(x, y);
    sq->val3 &= 0xC7;
    sq->val3 |= (faction_id & 7) << 3;
}

/*
Lock the specified tile for faction_id during multiplayer.
Return Value: true if already locked by another faction, otherwise false
*/
int __cdecl lock_map(int x, int y, int faction_id) {
    int lock_id = lock_at(x, y);
    if (lock_id != faction_id) {
        if (lock_id) {
            return true;
        }
        lock_set(x, y, faction_id);
    }
    return false;
}

/*
Unlock the specified tile for faction_id during multiplayer.
*/
void __cdecl unlock_map(int x, int y, int faction_id) {
    if (lock_at(x, y) == faction_id) {
        MAP* sq = mapsq(x, y);
        sq->val3 &= 0xC7;
    }
}

uint32_t __cdecl bit_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    return sq ? sq->items : 0;
}

void __cdecl bit_put(int x, int y, uint32_t items) {
    MAP* sq = mapsq(x, y);
    if (sq) {
        sq->items = items;
    }
    assert(sq);
}

void __cdecl bit_set(int x, int y, uint32_t items, bool add) {
    MAP* sq = mapsq(x, y);
    if (sq) {
        if (add) {
            sq->items |= items;
        } else {
            sq->items &= ~items;
        }
    }
    assert(sq);
}

uint32_t __cdecl bit2_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    return (sq ? sq->landmarks : 0);
}

void __cdecl bit2_set(int x, int y, uint32_t items, bool add) {
    MAP* sq = mapsq(x, y);
    if (sq) {
        if (add) {
            sq->landmarks |= items;
        } else {
            sq->landmarks &= ~items;
        }
    }
    assert(sq);
}

uint32_t __cdecl code_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    return (sq ? sq->code_at() : 0);
}

void __cdecl code_set(int x, int y, int code) {
    MAP* sq = mapsq(x, y);
    if (sq) {
        sq->landmarks &= 0xFFFFFF;
        sq->landmarks |= (code << 24);
        *GameDrawState |= 4;
    }
}

void __cdecl synch_bit(int x, int y, int faction_id) {
    if (faction_id > 0 && faction_id < MaxPlayerNum) {
        MAP* sq = mapsq(x, y);
        if (sq) {
            sq->visible_items[faction_id - 1] = sq->items;
        }
        assert(sq);
    }
}

void __cdecl site_radius(int x, int y) {
    for (int i = 0; i < 21; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        MAP* sq = mapsq(x2, y2);
        if (sq) {
            sq->val2 &= 0x0F;
        }
    }
}

/*
First Manifold Nexus tile must also be visible to the owner for the effect to take place.
*/
int __cdecl has_temple(int faction_id) {
    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y&1; x < *MapAreaX; x += 2) {
            MAP* sq = mapsq(x, y);
            if (sq && sq->landmarks & LM_NEXUS
            && sq->code_at() == 0 && sq->owner == faction_id
            && sq->visibility & (1 << faction_id)) {
                return true;
            }
        }
    }
    return false;
}

/*
Search for the first landmark found within the radius range of the specified tile.
Return Value: Landmark offset or -1 if none found
*/
int __cdecl find_landmark(int x, int y, size_t range_offset) {
    int offset = TableRange[range_offset];
    for (int i = 0; i < offset; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        if (mapsq(x2, y2)) {
            for (int lm = 0; lm < *MapLandmarkCount; lm++) {
                if (x2 == Landmarks[lm].x && y2 == Landmarks[lm].y) {
                    return lm;
                }
            }
        }
    }
    return -1;
}

/*
Set up a new landmark with the provided name at the specified tile.
Return Value: Landmark offset or -1 if max landmark count is reached
*/
int __cdecl new_landmark(int x, int y, const char* name) {
    int offset = *MapLandmarkCount;
    if (offset >= MaxLandmarkNum) {
        return -1;
    }
    *MapLandmarkCount += 1;
    Landmarks[offset].x = x;
    Landmarks[offset].y = y;
    strcpy_n(&Landmarks[offset].name[0], 32, name);
    return offset;
}

/*
Check whether the specified faction has permission to name a landmark on the provided tile.
Return Value: Does the faction have control of the tile to set a landmark? true/false
*/
int __cdecl valid_landmark(int x, int y, int faction_id) {
    MAP* sq;
    int owner = *MultiplayerActive ? (sq = mapsq(x, y), sq ? sq->owner : -1)
        : whose_territory(faction_id, x, y, NULL, false);
    if (owner == faction_id) {
        return true;
    }
    if (owner > 0) {
        return false;
    }
    int base_id = base_find(x, y);
    return base_id < 0 || Bases[base_id].faction_id == faction_id;
}

/*
Remove the landmark at the specified tile.
*/
void __cdecl kill_landmark(int x, int y) {
    int remove_id = find_landmark(x, y, 1);
    if (remove_id >= 0) {
        if (remove_id < (*MapLandmarkCount - 1)) {
            memmove(&Landmarks[remove_id],
                &Landmarks[remove_id + 1],
                sizeof(Landmark) * (*MapLandmarkCount - remove_id - 1));
        }
        *MapLandmarkCount -= 1;
    }
}

int __cdecl base_find(int x, int y) {
    int base_dist = 9999;
    int base_id = -1;
    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        int dist = vector_dist(x, y, base->x, base->y);
        if (dist <= base_dist) {
            base_dist = dist;
            base_id = i;
        }
    }
    if (base_id >= 0) {
        *BaseFindDist = base_dist;
    }
    return base_id;
}

int __cdecl base_find_2(int x, int y, int faction_id) {
    int base_dist = 9999;
    int base_id = -1;
    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id == faction_id) {
            int dist = vector_dist(x, y, base->x, base->y);
            if (dist <= base_dist) {
                base_dist = dist;
                base_id = i;
            }
        }
    }
    if (base_id >= 0) {
        *BaseFindDist = base_dist;
    }
    return base_id;
}

/*
This version adds support for modified territory borders (earlier bases claim tiles first).
*/
int __cdecl base_find_3(int x, int y, int faction_id, int region, int faction_id_2, int faction_id_3) {
    int base_dist = 9999;
    int base_id = -1;
    bool border_fix = conf.territory_border_fix && region >= MaxRegionLandNum;

    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (border_fix || region < 0 || region_at(base->x, base->y) == region) {
            if (faction_id < 0 ? (faction_id_2 < 0 || base->faction_id != faction_id_2)
            : (faction_id == base->faction_id
            || (faction_id_2 == -2 ? has_treaty(faction_id, base->faction_id, DIPLO_PACT)
            : (faction_id_2 >= 0 && faction_id_2 == base->faction_id)))) {
                if (faction_id_3 < 0 || base->faction_id == faction_id_3
                || base->visibility & (1 << faction_id_3)) {
                    int dist = vector_dist(x, y, base->x, base->y);
                    if (conf.territory_border_fix ? dist < base_dist : dist <= base_dist) {
                        base_dist = dist;
                        base_id = i;
                    }
                }
            }
        }
    }
    *BaseFindDist = 9999; // Default value is always written here
    if (base_id >= 0) {
        *BaseFindDist = base_dist;
    }
    return base_id;
}

int __cdecl whose_territory(int faction_id, int x, int y, int* base_id, int ignore_comm) {
    MAP* sq = mapsq(x, y);
    if (!sq || sq->owner < 0) { // Fix: invalid coordinates return -1 (no owner)
        return -1;
    }
    if (faction_id != sq->owner) {
        if (!ignore_comm
        && !(*GameState & STATE_OMNISCIENT_VIEW)
        && (!has_treaty(faction_id, sq->owner, DIPLO_COMMLINK)
        || !has_treaty(faction_id, sq->owner, DIPLO_UNK_8000000))) {
            return -1;
        }
        if (base_id) {
            *base_id = base_find_3(x, y, -1, sq->region, -1, -1);
        }
    }
    return sq->owner;
}

std::vector<MapTile> iterate_tiles(int x, int y, size_t start_index, size_t end_index) {
    std::vector<MapTile> tiles;
    assert(start_index < end_index && end_index <= (size_t)TableRange[MaxTableRange]);

    for (size_t i = start_index; i < end_index; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        MAP* sq = mapsq(x2, y2);
        if (sq) {
            tiles.push_back({x2, y2, (int)i, sq});
        }
    }
    assert(tiles.size() > 0);
    return tiles;
}

int nearby_items(int x, int y, size_t start_index, size_t end_index, uint32_t item) {
    assert(start_index < end_index && end_index <= (size_t)TableRange[MaxTableRange]);
    int n = 0;
    for (size_t i = start_index; i < end_index; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        if (bit_at(x2, y2) & item) {
            n++;
        }
    }
    return n;
}

char* strcpy_n(char* dst, size_t count, const char* src) {
    // Same bytes as strncpy(dst, src, count) + dst[count - 1] = 0 (zero padded),
    // written as a loop: gcc -O2 inlines strncpy here and -Wstringop-truncation
    // then fails the build when count equals the destination size.
    size_t i = 0;
    for (; i + 1 < count && src[i]; i++) {
        dst[i] = src[i];
    }
    for (; i < count; i++) {
        dst[i] = '\0';
    }
    return dst;
}

} // namespace engine
} // namespace alpha
