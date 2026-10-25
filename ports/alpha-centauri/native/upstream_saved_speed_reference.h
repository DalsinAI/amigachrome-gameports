/* SPDX-License-Identifier: GPL-3.0-or-later
 * TEST ONLY: verbatim function bodies from OpenSMACX veh.cpp, Brendan Casey
 * (C)2013-2021, pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 * Adapters replace original global addresses/callbacks with explicit context;
 * unknown callbacks are errors. The Fungal Tower constant is enabled only for
 * expansion rules: this is a documented base-profile adapter, not an original
 * base-executable differential claim. No original x86 code is executed.
 */
#ifndef AC_ALPHA_UPSTREAM_SAVED_SPEED_REFERENCE_H
#define AC_ALPHA_UPSTREAM_SAVED_SPEED_REFERENCE_H
#include "saved_speed.h"
namespace saved_speed_reference {
using BOOL=bool;
constexpr int TRIAD_LAND=0,TRIAD_SEA=1,TRIAD_AIR=2;
constexpr unsigned WPN_MODE_TRANSPORT=7,PLAN_ALIEN_ARTIFACT=12,MORALE_ELITE=6,MaxVehProtoFactionNum=64;
constexpr uint32_t ABL_SLOW=0x800000u,ABL_ANTIGRAV_STRUTS=0x400u,ABL_FUEL_NANOCELLS=0x8000000u,ABL_AIR_SUPERIORITY=0x20u;
constexpr unsigned SP_CLOUDBASE_ACADEMY=35,SP_MARITIME_CONTROL_CENTER=10;
static uint32_t BSC_FUNGAL_TOWER=19;
static const alpha::UnitDefinitions* definitions;
static alpha::OriginalPrototype VehPrototypes[512];
struct Vehicle { uint32_t proto_id;uint8_t faction_id,dmg_incurred; };
static Vehicle Vehs[1];
struct Basic {int move_rate_roads;};static Basic basic;static Basic* Rules=&basic;
static alpha::SavedSpeedContext context;
static bool unexpected;
static int range(int value,int low,int high){return value<low?low:value>high?high:value;}
static bool has_abil(uint32_t id,uint32_t flag){if(id>=512){unexpected=true;return false;}return (VehPrototypes[id].ability_flags&flag)!=0;}
static bool has_project(uint32_t project,uint32_t faction){
 if(project==SP_CLOUDBASE_ACADEMY && faction==Vehs[0].proto_id/64)return context.cloudbase_academy;
 if(project==SP_MARITIME_CONTROL_CENTER && faction==Vehs[0].faction_id)return context.maritime_control_center;
 unexpected=true;return false;
}
static uint8_t get_proto_triad(uint32_t id){return (uint8_t)definitions->chassis[VehPrototypes[id].chassis_id].triad;}
static int8_t get_proto_offense_rating(uint32_t id){return (int8_t)definitions->weapons[VehPrototypes[id].weapon_id].offense;}
static uint32_t morale_veh(uint32_t id,BOOL riots,int versus){if(id!=0 || !riots || versus!=0)unexpected=true;return (uint32_t)context.effective_morale;}
#define Chassis (definitions->chassis)
#define Weapon (definitions->weapons)
#define __cdecl
uint32_t __cdecl speed_proto(uint32_t proto_id) {
    if (proto_id == BSC_FUNGAL_TOWER) {
        return 0; // cannot move
    }
    int chas_id = VehPrototypes[proto_id].chassis_id;
    int speed_val = Chassis[chas_id].speed;
    uint8_t triad = Chassis[chas_id].triad;
    uint32_t weap_id = VehPrototypes[proto_id].weapon_id;
    if (triad == TRIAD_AIR) {
        speed_val += VehPrototypes[proto_id].reactor_id * 2;
    }
    if (Weapon[weap_id].mode == WPN_MODE_TRANSPORT) {
        speed_val--;
    }
    if (has_abil(proto_id, ABL_SLOW)) {
        speed_val--;
    }
    if (has_abil(proto_id, ABL_ANTIGRAV_STRUTS)) {
        speed_val += (triad == TRIAD_AIR) ? VehPrototypes[proto_id].reactor_id * 2 : 1;
    }
    if (triad == TRIAD_AIR) {
        if (has_abil(proto_id, ABL_FUEL_NANOCELLS)) {
            speed_val += 2;
        }
        if (has_project(SP_CLOUDBASE_ACADEMY, proto_id / MaxVehProtoFactionNum)) {
            speed_val += 2; // bonus from Aerospace Complex
        }
        if (has_abil(proto_id, ABL_AIR_SUPERIORITY)) {
            // generally -20% to -25%, in some cases higher due to lossy division rounding
            speed_val = (speed_val * 4) / 5;
        }
        if (Weapon[weap_id].mode == WPN_MODE_TRANSPORT) {
            speed_val /= 2; // 2nd penalty for air transports: -50%
        }
    }
    return range(speed_val, 1, 99) * Rules->move_rate_roads;
}

uint32_t __cdecl speed(uint32_t veh_id, BOOL skip_morale) {
    uint32_t proto_id = Vehs[veh_id].proto_id;
    if (proto_id == BSC_FUNGAL_TOWER) { // moved this check to top vs bottom, same logic
        return 0; // cannot move
    }
    uint32_t speed_val = speed_proto(proto_id);
    uint8_t triad = get_proto_triad(proto_id);
    if (triad == TRIAD_SEA && has_project(SP_MARITIME_CONTROL_CENTER, Vehs[veh_id].faction_id)) {
        speed_val += Rules->move_rate_roads * 2;
    }
    if (!skip_morale && morale_veh(veh_id, true, 0) == MORALE_ELITE
        && (proto_id >= MaxVehProtoFactionNum || get_proto_offense_rating(proto_id) >= 0)) {
        speed_val += Rules->move_rate_roads;
    }
    if (Vehs[veh_id].dmg_incurred && triad != TRIAD_AIR) {
        // optimized code without changes to logic
        uint32_t moves = speed_val / Rules->move_rate_roads;
        uint32_t reactor_fac;
        if (VehPrototypes[proto_id].plan == PLAN_ALIEN_ARTIFACT) {
            speed_val = reactor_fac = 1;
        } else {
            reactor_fac = range(VehPrototypes[proto_id].reactor_id, 1, 100) * 10;
            speed_val = range(reactor_fac, 1, 99);
        }
        speed_val = (moves
            * range(reactor_fac - Vehs[veh_id].dmg_incurred, 0, 9999) + speed_val - 1) / speed_val;
        speed_val = range(speed_val, (triad == TRIAD_SEA) ? 2 : 1, 999) * Rules->move_rate_roads;
    }
    return speed_val;
}
#undef __cdecl
#undef Chassis
#undef Weapon
static void setup(const alpha::UnitDefinitions& d,const alpha::OriginalPrototype& p,const alpha::OriginalVehicle& v,int road,const alpha::SavedSpeedContext& ctx){
 definitions=&d;VehPrototypes[v.unit_id]=p;Vehs[0]={(uint32_t)v.unit_id,v.faction_id,v.damage_taken};basic.move_rate_roads=road;context=ctx;unexpected=false;BSC_FUNGAL_TOWER=d.weapon_count==26?19u:0xffffffffu;
}
}
#endif
