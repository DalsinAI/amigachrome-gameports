/* SPDX-License-Identifier: GPL-3.0-or-later
 * Map accessors ported from Thinker Mod src/map.h, offset tables from
 * src/path.h and src/mapgen.h at dabe62f2d1abba03de40f3dd3048494843f6a9af
 * (MIT; see engine/THINKER-LICENSE). Yield, movement-cost, terraform and
 * world generation functions belong to later packages (see engine_wp.h).
 * Include engine.h rather than this header directly.
 */
#pragma once

namespace alpha {
namespace engine {

const int NearbyTiles[][2] = {{1,-1}, {2,0}, {1,1}, {0,2}, {-1,1}, {-2,0}, {-1,-1}, {0,-2}};
const int BaseOffsetX[] = { 1, 2, 1, 0, -1, -2, -1,  0, 0}; // Path::find offset
const int BaseOffsetY[] = {-1, 0, 1, 2,  1,  0, -1, -2, 0}; // Path::find offset
const int NearOffsetX[] = {1, 1, -1, -1, 0};
const int NearOffsetY[] = {-1, 1, 1, -1, 0};
const int MaxTableRange = 8;
const int TableRange[] = {1, 9, 25, 49, 81, 121, 169, 225, 289};

const int TableOffsetX[] = {
     0,  1,  2,  1,  0, -1, -2,  -1,   0,   2,   2,  -2,  -2,   1,   3,   3,   1,  -1,  -3,  -3,
    -1,  4, -4,  0,  0,  1,  2,   3,   4,   5,   5,   4,   3,   2,   1,  -1,  -2,  -3,  -4,  -5,
    -5, -4, -3, -2, -1,  0,  6,   0,  -6,   0,   1,   2,   3,   4,   5,   6,   7,   8,   7,   6,
     5,  4,  3,  2,  1,  0, -1,  -2,  -3,  -4,  -5,  -6,  -7,  -8,  -7,  -6,  -5,  -4,  -3,  -2,
    -1,  0,  1,  2,  3,  4,  5,   6,   7,   8,   9,  10,   9,   8,   7,   6,   5,   4,   3,   2,
     1,  0, -1, -2, -3, -4, -5,  -6,  -7,  -8,  -9, -10,  -9,  -8,  -7,  -6,  -5,  -4,  -3,  -2,
    -1,  0,  1,  2,  3,  4,  5,   6,   7,   8,   9,  10,  11,  12,  11,  10,   9,   8,   7,   6,
     5,  4,  3,  2,  1,  0, -1,  -2,  -3,  -4,  -5,  -6,  -7,  -8,  -9, -10, -11, -12, -11, -10,
    -9, -8, -7, -6, -5, -4, -3,  -2,  -1,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,
    11, 12, 13, 14, 13, 12, 11,  10,   9,   8,   7,   6,   5,   4,   3,   2,   1,   0,  -1,  -2,
    -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -13, -12, -11, -10,  -9,  -8,  -7,  -6,
    -5, -4, -3, -2, -1,  0,  1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
    15, 16, 15, 14, 13, 12, 11,  10,   9,   8,   7,   6,   5,   4,   3,   2,   1,   0,  -1,  -2,
    -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15, -16, -15, -14, -13, -12, -11, -10,
    -9, -8, -7, -6, -5, -4, -3,  -2,  -1,
};

const int TableOffsetY[] = {
     0,  -1,   0,   1,   2,   1,   0,  -1,  -2,  -2,   2,   2,  -2,  -3, -1,  1,  3,  3,  1, -1,
    -3,   0,   0,   4,  -4,  -5,  -4,  -3,  -2,  -1,   1,   2,   3,   4,  5,  5,  4,  3,  2,  1,
    -1,  -2,  -3,  -4,  -5,   6,   0,  -6,   0,  -8,  -7,  -6,  -5,  -4, -3, -2, -1,  0,  1,  2,
     3,   4,   5,   6,   7,   8,   7,   6,   5,   4,   3,   2,   1,   0, -1, -2, -3, -4, -5, -6,
    -7, -10,  -9,  -8,  -7,  -6,  -5,  -4,  -3,  -2,  -1,   0,   1,   2,  3,  4,  5,  6,  7,  8,
     9,  10,   9,   8,   7,   6,   5,   4,   3,   2,   1,   0,  -1,  -2, -3, -4, -5, -6, -7, -8,
    -9, -12, -11, -10,  -9,  -8,  -7,  -6,  -5,  -4,  -3,  -2,  -1,   0,  1,  2,  3,  4,  5,  6,
     7,   8,   9,  10,  11,  12,  11,  10,   9,   8,   7,   6,   5,   4,  3,  2,  1,  0, -1, -2,
    -3,  -4,  -5,  -6,  -7,  -8,  -9, -10, -11, -14, -13, -12, -11, -10, -9, -8, -7, -6, -5, -4,
    -3,  -2,  -1,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10, 11, 12, 13, 14, 13, 12,
    11,  10,   9,   8,   7,   6,   5,   4,   3,   2,   1,   0,  -1,  -2, -3, -4, -5, -6, -7, -8,
    -9, -10, -11, -12, -13, -16, -15, -14, -13, -12, -11, -10,  -9,  -8, -7, -6, -5, -4, -3, -2,
    -1,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12, 13, 14, 15, 16, 15, 14,
    13,  12,  11,  10,   9,   8,   7,   6,   5,   4,   3,   2,   1,   0, -1, -2, -3, -4, -5, -6,
    -7,  -8,  -9, -10, -11, -12, -13, -14, -15,
};

const int ElevDetail[] = {0, 20, 40, 60, 80, 100, 120, 140, 160, 180, 200};

char* strcpy_n(char* dst, size_t count, const char* src);

MAP* next_tile(int x, int y, size_t offset, int* tx, int* ty);
MAP* mapsq(int x, int y);
int wrap(int x);
int x_dist(int x1, int x2);
int map_range(int x1, int y1, int x2, int y2);
int vector_dist(int x1, int y1, int x2, int y2);
int min_range(const Points& S, int x, int y);
int min_vector(const Points& S, int x, int y);
int avg_range(const Points& S, int x, int y);
bool is_ocean(MAP* sq);
bool is_ocean(BASE* base);
bool is_ocean(int x, int y);
bool is_ocean_shelf(MAP* sq);
bool is_shore_level(MAP* sq);
bool map_is_flat();
bool map_is_known(int faction_id);
bool adjacent_region(int x, int y, int owner, int threshold, bool ocean);
std::vector<MapTile> iterate_tiles(int x, int y, size_t start_index, size_t end_index);
int nearby_items(int x, int y, size_t start_index, size_t end_index, uint32_t item);

template <class A, class B>
int map_range(const A* a, const B* b) {
    return map_range(a->x, a->y, b->x, b->y);
}
template <class A, class B>
int vector_dist(const A* a, const B* b) {
    return vector_dist(a->x, a->y, b->x, b->y);
}

int __cdecl is_sensor(int x, int y);
int __cdecl is_known(int x, int y, int faction_id);
int __cdecl is_coast(int x, int y, bool is_base_radius);
int __cdecl is_port(int base_id, bool is_base_radius);
int __cdecl on_map(int x, int y);
int __cdecl bad_reg(int region);
void __cdecl rebuild_base_bits();
void __cdecl rebuild_vehicle_bits();
void __cdecl owner_set(int x, int y, int faction_id);
void __cdecl site_set(int x, int y, uint8_t site);
void __cdecl region_set(int x, int y, uint8_t region);
int __cdecl region_at(int x, int y);
int __cdecl base_at(int x, int y);
int __cdecl alt_at(int x, int y);
int __cdecl alt_detail_at(int x, int y);
void __cdecl alt_put_detail(int x, int y, uint8_t detail);
int __cdecl alt_natural(int x, int y);
int __cdecl elev_at(int x, int y);
void __cdecl world_alt_put_detail(int x, int y);
int __cdecl temp_at(int x, int y);
void __cdecl temp_set(int x, int y, uint8_t value);
void __cdecl climate_set(int x, int y, uint8_t rainfall);
int __cdecl using_at(int x, int y);
void __cdecl using_set(int x, int y, int faction_id);
int __cdecl lock_at(int x, int y);
void __cdecl lock_set(int x, int y, int faction_id);
int __cdecl lock_map(int x, int y, int faction_id);
void __cdecl unlock_map(int x, int y, int faction_id);
void __cdecl rocky_set(int x, int y, uint8_t rocky);
uint32_t __cdecl bit_at(int x, int y);
void __cdecl bit_put(int x, int y, uint32_t items);
void __cdecl bit_set(int x, int y, uint32_t items, bool add);
uint32_t __cdecl bit2_at(int x, int y);
void __cdecl bit2_set(int x, int y, uint32_t items, bool add);
uint32_t __cdecl code_at(int x, int y);
void __cdecl code_set(int x, int y, int code);
void __cdecl synch_bit(int x, int y, int faction_id);
void __cdecl site_radius(int x, int y);
int __cdecl has_temple(int faction_id);
int __cdecl find_landmark(int x, int y, size_t range_offset);
int __cdecl new_landmark(int x, int y, const char* name);
int __cdecl valid_landmark(int x, int y, int faction_id);
void __cdecl kill_landmark(int x, int y);
int __cdecl base_find(int x, int y);
int __cdecl base_find_2(int x, int y, int faction_id);
int __cdecl base_find_3(int x, int y, int faction_id, int region, int faction_id_2, int faction_id_3);
int __cdecl whose_territory(int faction_id, int x, int y, int* base_id, int ignore_comm);

} // namespace engine
} // namespace alpha
