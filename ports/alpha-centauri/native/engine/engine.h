/* SPDX-License-Identifier: GPL-3.0-or-later
 * Alpha Centauri engine core (WP0). Umbrella header.
 * Ported from Thinker Mod src/engine.h and src/main.h at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 *
 * Differences from Thinker: no Windows headers, no fixed executable addresses,
 * no function pointers into terranx.exe. Every Thinker global keeps its name
 * but is a pointer bound to the active EngineState (see game_state.h), so
 * ported code using *VehCount, Vehs[i], Factions[i] etc. compiles unchanged.
 * Records are plain C++ structs in native byte order; the original SAV wire
 * format is read and written field by field in savegame.cpp (little-endian).
 *
 * Everything lives in namespace alpha::engine. Include only this header.
 */
#pragma once

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#ifndef __cdecl
#define __cdecl
#endif
#ifndef __thiscall
#define __thiscall
#endif
#ifdef __GNUC__
#define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#define UNUSED(x) UNUSED_ ## x
#endif

/* Thinker release builds compile with NDEBUG, so its assert() calls are
 * documentation only and ported code relies on that (for example on_map()
 * asserting tile parity). Keep that behaviour unless ENGINE_ASSERTS is set,
 * while still type-checking the expression. */
#ifndef ENGINE_ASSERTS
#undef assert
#define assert(e) ((void)sizeof((e) ? 1 : 0))
#endif

namespace alpha {
namespace engine {

typedef uint8_t byte;
typedef struct { char str[256]; } char256;
typedef volatile int net_int_t;

typedef int (__cdecl *fp_none)();
typedef int (__cdecl *fp_1int)(int);
typedef int (__cdecl *fp_2int)(int, int);
typedef int (__cdecl *fp_3int)(int, int, int);
typedef int (__cdecl *fp_4int)(int, int, int, int);

/* Forward declarations needed by member functions of the record types
 * (identical to the block at the top of Thinker engine.h). */
struct CChassis;
extern CChassis* Chassis;
struct CWeapon;
extern CWeapon* Weapon;
struct CArmor;
extern CArmor* Armor;
struct UNIT;
extern UNIT* Units;
struct VEH;
extern VEH* Vehs;
struct Faction;
extern Faction* Factions;
struct MFaction;
extern MFaction* MFactions;
extern int* GameState;
extern int* GameRules;
extern int* CurrentTurn;
extern int* ExpansionEnabled;
bool is_human(int faction_id);
bool can_repair(int unit_id);
bool can_monolith(int unit_id);
bool map_is_known(int faction_id);

/* Thinker debug logging. Arguments are always evaluated (as functions, not
 * macros) so ported code does not trip unused-variable warnings. */
extern FILE* debug_log;
void debug(const char* fmt, ...)
#ifdef __GNUC__
    __attribute__((format(printf, 1, 2)))
#endif
    ;
void debug_ver(const char* fmt, ...)
#ifdef __GNUC__
    __attribute__((format(printf, 1, 2)))
#endif
    ;
void debugw(const char* fmt, ...)
#ifdef __GNUC__
    __attribute__((format(printf, 1, 2)))
#endif
    ;
void flushlog();

/* Small helpers from Thinker engine.h. */
template <class T, class C>
bool has_item(const T& a, const C& b) {
    return std::find(a.begin(), a.end(), b) != a.end();
}
template <class T>
const T& min (const T& a, const T& b) {
    return std::min(a, b);
}
template <class T>
const T& max (const T& a, const T& b) {
    return std::max(a, b);
}
template <class T>
const T& clamp (const T& value, const T& low, const T& high) {
    return (value < low ? low : (value > high ? high : value));
}
template <typename T>
int bit_count(T value) {
    static_assert(std::is_integral<T>::value, "integer type required");
    using U = typename std::make_unsigned<T>::type;
    U v = static_cast<U>(value);
    int n = 0;
    while (v) {
        v &= static_cast<U>(v - 1);
        n++;
    }
    return n;
}

} // namespace engine
} // namespace alpha

#include "engine_enums.h"
#include "engine_types.h"
#include "engine_data.h"
#include "engine_base.h"
#include "engine_veh.h"
#include "engine_win.h"

namespace alpha {
namespace engine {

struct MapTile {
    int x;
    int y;
    int i;
    MAP* sq;
};

struct TileValue {
    int x;
    int y;
    int i;
    MAP* sq;
    int nutrient;
    int mineral;
    int energy;
};

struct Point {
    int x;
    int y;

    bool operator==(const Point& obj) const {
        return (x == obj.x && y == obj.y);
    }
    bool operator<(const Point& obj) const {
        return x < obj.x || (x == obj.x && y < obj.y);
    }
};

struct MapNode {
    int x;
    int y;
    int type;

    bool operator<(const MapNode& obj) const {
        return x < obj.x || (x == obj.x && (
            y < obj.y || (y == obj.y && (type < obj.type))
        ));
    }
};

inline TriadFlag operator| (TriadFlag a, TriadFlag b) { return (TriadFlag)((uint32_t)a | (uint32_t)b); }
inline TriadFlag& operator|= (TriadFlag& a, TriadFlag b) { a = a | b; return a; }
inline VehAblFlag operator| (VehAblFlag a, VehAblFlag b) { return (VehAblFlag)((uint32_t)a | (uint32_t)b); }
inline VehAblFlag& operator|= (VehAblFlag& a, VehAblFlag b) { a = a | b; return a; }

} // namespace engine
} // namespace alpha

template <>
struct std::hash<alpha::engine::Point> {
    std::size_t operator()(const alpha::engine::Point& p) const {
        return (std::size_t)(p.x ^ (p.y << 16));
    }
};

namespace alpha {
namespace engine {

typedef std::set<MapNode> NodeSet;
typedef std::set<Point> Points;
typedef std::vector<Point> PointList;
typedef std::set<std::string> set_str_t;
typedef std::vector<std::string> vec_str_t;
typedef std::map<std::string, std::string> map_str_t;
typedef std::map<int, int> map_int_t;

} // namespace engine
} // namespace alpha

#include "config.h"
#include "game_state.h"
#include "random.h"
#include "events.h"
#include "map.h"
#include "faction.h"
#include "veh.h"
#include "bases.h"
#include "savegame.h"
#include "engine_wp.h"
