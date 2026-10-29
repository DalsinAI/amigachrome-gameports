/* SPDX-License-Identifier: GPL-3.0-or-later
 * TEST ONLY: OpenSMACX veh.cpp::hex_cost body, Brendan Casey(C)2013-2021,
 * pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c. Added grouping preserves
 * operator precedence. Storage adapters read the actual owned world. Base
 * profile disables expansion Sealurk/Spore exceptions; this is NOT original
 * executable differential proof. Recovered prototype-speed body supplies the
 * land fungus speed dependency; no production pricing calls supply answers.
 */
#ifndef AC_ALPHA_UPSTREAM_WORLD_MOVEMENT_REFERENCE_H
#define AC_ALPHA_UPSTREAM_WORLD_MOVEMENT_REFERENCE_H
#include "world_movement_price.h"
#include "upstream_saved_speed_reference.h"
#include "upstream_world_stats_reference.h"
#include <stdlib.h>
namespace world_movement_reference {
using BOOL=bool;
constexpr uint32_t BIT_BASE_IN_TILE=1,BIT_ROAD=4,BIT_MAGTUBE=8,BIT_FUNGUS=0x20,BIT_RIVER=0x80,BIT_FOREST=0x200000,ABL_ANTIGRAV_STRUTS=0x400;
constexpr int ALT_BIT_OCEAN_SHELF=0x40,TERRAIN_BIT_ROLLING=0x40,SP_XENOEMPATYH_DOME=8;
constexpr int BSC_ISLE_OF_THE_DEEP=9,BSC_MIND_WORMS=8,CHSI_HOVERTANK=2,TRIAD_SEA=1,TRIAD_LAND=0,MaxVehProtoFactionNum=64,PLAN_TERRAFORMING=9,PLAN_ALIEN_ARTIFACT=12;
static int BSC_SEALURK=14,BSC_SPORE_LAUNCHER=15;
static const alpha::OriginalWorld* world;static const alpha::UnitDefinitions* definitions;
static bool unexpected;static alpha::OriginalPrototype VehPrototypes[512];
struct Basic {int move_rate_roads;};static Basic basic;static Basic* Rules=&basic;
struct Player {struct {int32_t planet;} soc_effect_active;};static Player PlayersData[8];
static const alpha::OriginalMapTile* tile(uint32_t x,uint32_t y){
 if(x>=world->game().map.info.width || y>=world->game().map.info.height || ((x+y)&1)){unexpected=true;return nullptr;}
 return world->tile(((size_t)y*world->game().map.info.width+x)/2);
}
static uint32_t bit_at(uint32_t x,uint32_t y){const auto* t=tile(x,y);return t?t->items:0;}
static uint32_t altitude_at(uint32_t x,uint32_t y){const auto* t=tile(x,y);return t?t->climate&0xe0u:0;}
static uint32_t rocky_at(uint32_t x,uint32_t y){const auto* t=tile(x,y);return t?t->val3&0xc0u:0;}
static bool is_ocean(uint32_t x,uint32_t y){return altitude_at(x,y)<0x60;}
static int get_proto_triad(int id){return definitions->chassis[VehPrototypes[id].chassis_id].triad;}
static int get_proto_offense_rating(int id){return definitions->weapons[VehPrototypes[id].weapon_id].offense;}
static bool has_abil(int id,uint32_t flag){
 if(flag!=0x400u && flag!=0x800000u){unexpected=true;return false;}
 if(VehPrototypes[id].ability_flags&flag)return true;
 if(definitions->weapon_count==21)return false; // Original base has_abil is raw-only.
 const unsigned owner=(unsigned)id/64;if(!owner)return false;
 const auto* f=world->faction_definition(owner);if(!f || f->faction_bonus_count<0 || f->faction_bonus_count>8){unexpected=true;return false;}
 for(int i=0;i<f->faction_bonus_count;++i)if(f->faction_bonus_id[i]==15){
  int a=f->faction_bonus_val1[i];if(a<0 || (unsigned)a>=definitions->ability_count){unexpected=true;return false;}
  if(world_stats_reference::has_tech(definitions->abilities[a].prerequisite,owner) && (flag&(1u<<a)))return true;
 }
 return false;
}
static bool has_project(int project,int faction){
 if(project!=8 || faction<0 || faction>=8){unexpected=true;return false;}
 int b=world->game().preferences.SecretProjects[project];if(b<0)return false;
 const auto* base=world->base((size_t)b);if(!base){unexpected=true;return false;}return base->faction_id==faction;
}
static int x_dist(int a,int b){int d=abs(a-b);if(!world->game().map.info.flat && d>(int)world->game().map.info.width/2)d=(int)world->game().map.info.width-d;return d;}
static uint32_t speed_proto(int id){uint32_t value=saved_speed_reference::speed_proto((uint32_t)id);if(saved_speed_reference::unexpected)unexpected=true;return value;}
#define __cdecl
int __cdecl hex_cost(int proto_id, int faction_id, uint32_t x_src, uint32_t y_src, uint32_t x_dst,
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
#undef __cdecl
static void setup(const alpha::OriginalWorld& w,const alpha::UnitDefinitions& d,const alpha::Rules& tech,size_t index,int road){
 world=&w;definitions=&d;unexpected=false;world_stats_reference::setup(w,d,tech,index);basic.move_rate_roads=road;
 for(unsigned i=0;i<512;++i)VehPrototypes[i]=*w.prototype(i);
 for(unsigned i=0;i<8;++i)PlayersData[i].soc_effect_active.planet=w.faction(i)->SE_planet;
 BSC_SEALURK=d.weapon_count==26?14:-999;BSC_SPORE_LAUNCHER=d.weapon_count==26?15:-999;
 const auto& v=*w.vehicle(index);alpha::SavedSpeedContext ctx{false,false,true,0};
 // hex_cost calls speed_proto only on land; the air/sea projects cannot affect it.
 alpha::OriginalPrototype effective=*w.prototype((size_t)v.unit_id);
 if(get_proto_triad(v.unit_id)==0){if(has_abil(v.unit_id,0x400))effective.ability_flags|=0x400;if(has_abil(v.unit_id,0x800000))effective.ability_flags|=0x800000;}
 saved_speed_reference::setup(d,effective,v,road,ctx);
}
}
#endif
