/* SPDX-License-Identifier: GPL-3.0-or-later
 * Test-only OpenSMACX veh.cpp::hex_cost body, Brendan Casey(C)2013-2021,
 * pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c. Original branch/arithmetic order;
 * callbacks/global storage below are explicit test state, not production stubs.
 * Added grouping preserves the original && precedence in mixed || conditions.
 */
#ifndef AC_ALPHA_UPSTREAM_MOVEMENT_REFERENCE_H
#define AC_ALPHA_UPSTREAM_MOVEMENT_REFERENCE_H
#include <stdlib.h>
#include "movement.h"
#include "upstream_prototype_reference.h"
namespace movement_reference {
using namespace prototype_reference;
constexpr uint32_t BIT_BASE_IN_TILE=1,BIT_ROAD=4,BIT_MAGTUBE=8,BIT_FUNGUS=0x20,BIT_RIVER=0x80,BIT_FOREST=0x200000;
constexpr int ALT_BIT_OCEAN_SHELF=0x40,TERRAIN_BIT_ROLLING=0x40,SP_XENOEMPATYH_DOME=1;
constexpr int BSC_SEALURK=14,BSC_ISLE_OF_THE_DEEP=9,BSC_MIND_WORMS=8,BSC_SPORE_LAUNCHER=15,CHSI_HOVERTANK=2;
static alpha::MovementMap map;
static bool xeno;
struct PlayerRecord { struct { int32_t planet; } soc_effect_active; };
static PlayerRecord PlayersData[8];
static const alpha::MovementTile& tile(uint32_t x,uint32_t y){return map.tiles[(x+(uint32_t)map.geometry.width*y)/2];}
static uint32_t bit_at(uint32_t x,uint32_t y){return tile(x,y).items;}
static bool is_ocean(uint32_t x,uint32_t y){return tile(x,y).altitude<0x60;}
static uint32_t altitude_at(uint32_t x,uint32_t y){return tile(x,y).altitude;}
static uint32_t rocky_at(uint32_t x,uint32_t y){return tile(x,y).rockiness;}
static int get_proto_triad(int id){return Chassis[VehPrototypes[id].chassis_id].triad;}
static int get_proto_offense_rating(int id){return Weapon[VehPrototypes[id].weapon_id].offense_rating;}
static bool has_project(int id,int faction){if(id!=SP_XENOEMPATYH_DOME || faction<0 || faction>7)unsupported=true;return xeno;}
static int x_dist(int a,int b){int d=abs(a-b);if(!map.geometry.flat && d>map.geometry.width/2)d=map.geometry.width-d;return d;}
int hex_cost(int proto_id, int faction_id, uint32_t x_src, uint32_t y_src, uint32_t x_dst,
                     uint32_t y_dst, BOOL toggle) {
    uint32_t bit_dst = bit_at(x_dst, y_dst);
    if (is_ocean(x_dst, y_dst)) {
        if (bit_dst & BIT_FUNGUS && altitude_at(x_dst, y_dst) == ALT_BIT_OCEAN_SHELF
            && get_proto_triad(proto_id) == TRIAD_SEA
            && proto_id != BSC_SEALURK // Bug fix
            && proto_id != BSC_ISLE_OF_THE_DEEP && !has_project(SP_XENOEMPATYH_DOME, faction_id)) {
            return Rules->move_rate_roads * 3;
        }
        return Rules->move_rate_roads;
    }
    if (is_ocean(x_src, y_src)) {
        return Rules->move_rate_roads;
    }
    if (proto_id >= 0 && get_proto_triad(proto_id) != TRIAD_LAND) {
        return Rules->move_rate_roads;
    }
    // Land only conditions
    uint32_t bit_src = bit_at(x_src, y_src);
    if (bit_src & (BIT_MAGTUBE | BIT_BASE_IN_TILE) && bit_dst & (BIT_MAGTUBE | BIT_BASE_IN_TILE)
        && faction_id) {
        return 0;
    }
    if ((bit_src & (BIT_ROAD | BIT_BASE_IN_TILE) || (bit_src & BIT_FUNGUS && faction_id > 0
        && has_project(SP_XENOEMPATYH_DOME, faction_id))) && bit_dst & (BIT_ROAD | BIT_BASE_IN_TILE)
        && faction_id) {
        return 1;
    }
    if (faction_id >= 0 && (has_project(SP_XENOEMPATYH_DOME, faction_id) || !faction_id
        || proto_id == BSC_MIND_WORMS || proto_id == BSC_SPORE_LAUNCHER) && bit_dst & BIT_FUNGUS) {
        return 1;
    }
    if (bit_src & BIT_RIVER && bit_dst & BIT_RIVER && x_dist(x_src, x_dst) == 1
        && abs((int)y_src - (int)y_dst) == 1 && faction_id) {
        return 1;
    }
    if (VehPrototypes[proto_id].chassis_id == CHSI_HOVERTANK
        || has_abil(proto_id, ABL_ANTIGRAV_STRUTS)) {
        return Rules->move_rate_roads;
    }
    uint32_t cost = Rules->move_rate_roads;
    if (rocky_at(x_dst, y_dst) > TERRAIN_BIT_ROLLING && !toggle) {
        cost += Rules->move_rate_roads;
    }
    if (bit_dst & BIT_FOREST && !toggle) {
        cost += Rules->move_rate_roads;
    }
    if (faction_id && bit_dst & BIT_FUNGUS && (proto_id >= MaxVehProtoFactionNum
        || get_proto_offense_rating(proto_id) >= 0)) {
        uint8_t plan = VehPrototypes[proto_id].plan;
        if (plan != PLAN_TERRAFORMING && plan != PLAN_ALIEN_ARTIFACT
            && PlayersData[faction_id].soc_effect_active.planet <= 0) {
            return cost + Rules->move_rate_roads * 2;
        }
        uint32_t speed = speed_proto(proto_id);
        if (cost <= speed) {
            return speed;
        }
    }
    return cost;
}
}
#endif
