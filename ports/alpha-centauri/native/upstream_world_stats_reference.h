/* SPDX-License-Identifier: GPL-3.0-or-later
 * TEST ONLY: OpenSMACX technology.cpp::has_tech and veh.cpp::morale_alien/
 * morale_veh bodies, Brendan Casey (C)2013-2021,
 * pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 * Adapters bind original global names to qualified owned-world record values.
 * Map/project/facility queries use actual owned data, not canned true/false
 * callbacks. Unknown accesses are errors. No original x86 executes.
 * Expansion-only tower/spore constants are disabled in the base profile.
 */
#ifndef AC_ALPHA_UPSTREAM_WORLD_STATS_REFERENCE_H
#define AC_ALPHA_UPSTREAM_WORLD_STATS_REFERENCE_H
#include "world_unit_stats.h"
namespace world_stats_reference {
using BOOL=bool;
constexpr int TechNone=-1,TechDisabled=-2,MaxTechnologyNum=89,MaxVehProtoFactionNum=64;
constexpr unsigned PLAN_INFO_WARFARE=11,SP_TELEPATHIC_MATRIX=30,TFLAG_IMPROVED_PROBES=2;
constexpr unsigned FAC_CHILDREN_CRECHE=2,FAC_BROOD_PIT=35,RFLAG_MORALE=0x8000,BSTATE_DRONE_RIOTS_ACTIVE=2;
constexpr unsigned BIT_FUNGUS=0x20,ALT_BIT_OCEAN_SHELF=0x40,VSTATE_MONOLITH_UPGRADED=0x100,BSC_LOCUSTS_OF_CHIRON=10;
static int BSC_FUNGAL_TOWER=19,BSC_SPORE_LAUNCHER=15;
static const alpha::OriginalWorld* world;
static bool unexpected;
struct Vehicle{int16_t x,y,proto_id,home_base_id;uint8_t faction_id,morale;uint16_t flags;uint32_t state;};
static Vehicle Vehs[1];
static alpha::OriginalPrototype VehPrototypes[512];
static alpha::OriginalFactionDefinition Players[8];
struct Effects{int32_t probe,morale;};struct Faction{Effects soc_effect_active;int32_t major_atrocities;};static Faction PlayersData[8];
struct Base{uint32_t state;};static Base Bases[512];
struct TechnologyData{uint32_t flags;int32_t preq_tech_1,preq_tech_2;};static TechnologyData Technology[89];
static uint8_t GameTechAchieved[89];static int32_t TectonicDetonationCount[8];
static int32_t turn;static int32_t* TurnCurrentNum=&turn;
static const int RadiusRange[]={1,9};static const int RadiusOffsetX[]={0,1,2,1,0,-1,-2,-1,0},RadiusOffsetY[]={0,-1,0,1,2,1,0,-1,-2};
static int range(int n,int a,int b){return n<a?a:n>b?b:n;}
static int xrange(int x){if(!world->game().map.info.flat){int w=(int)world->game().map.info.width;if(x>=w)x-=w;else if(x<0)x+=w;}return x;}
static bool on_map(int x,int y){return x>=0 && (uint32_t)x<world->game().map.info.width && y>=0 && (uint32_t)y<world->game().map.info.height && !((x+y)&1);}
static const alpha::OriginalMapTile* tile(int x,int y){if(!on_map(x,y)){unexpected=true;return nullptr;}return world->tile(((size_t)y*world->game().map.info.width+(size_t)x)/2);}
static uint32_t bit_at(int x,int y){const auto* t=tile(x,y);return t?t->items:0;}
static unsigned altitude_at(int x,int y){const auto* t=tile(x,y);return t?t->climate&0xe0u:0;}
static bool has_project(unsigned project,unsigned faction){
 if(project>=64 || faction>=8){unexpected=true;return false;}
 int base=world->game().preferences.SecretProjects[project];if(base<0)return false;
 const auto* b=world->base((size_t)base);if(!b){unexpected=true;return false;}return (unsigned)b->faction_id==faction;
}
static bool has_fac_built(unsigned facility,unsigned base){const auto* b=world->base(base);if(!b || facility>64){unexpected=true;return false;}return (b->facilities_built[facility/8]&(1u<<(facility%8)))!=0;}
static int32_t weapon_offense[26];
static int8_t get_proto_offense_rating(int id){if(id<0 || id>=512){unexpected=true;return 0;}return (int8_t)weapon_offense[VehPrototypes[id].weapon_id];}
#define __cdecl
BOOL __cdecl has_tech(int tech_id, int faction_id) {
    if (faction_id <= 0) {
        return false;
    }
    if (tech_id == TechNone) {
        return true;
    }
    if (tech_id < 0 || tech_id >= (MaxTechnologyNum - 1) // excludes 'Transcendent Thought'
        || Technology[tech_id].preq_tech_1 == TechDisabled
        || (Technology[tech_id].preq_tech_2 == TechDisabled
            && Technology[tech_id].preq_tech_1 != TechNone)) {
        // "none, disable" ; valid #TECH preq_tech entry
        return false;
    }
    return ((1 << faction_id) & GameTechAchieved[tech_id]) != 0;
}

uint32_t __cdecl morale_alien(int veh_id, int faction_id_vs_native) {
    int morale;
    // Fungal Tower specific code, shifted to start and added veh_id check to prevent crash
    if (veh_id >= 0 && Vehs[veh_id].proto_id == BSC_FUNGAL_TOWER) {
        morale = 0;
        int16_t x = Vehs[veh_id].x;
        int16_t y = Vehs[veh_id].y;
        // similar to is_coast() > except with fungus check + Ocean Shelf included
        for (int i = RadiusRange[0]; i < RadiusRange[1]; i++) {
            int x_radius = xrange(x + RadiusOffsetX[i]);
            int y_radius = y + RadiusOffsetY[i];
            if (on_map(x_radius, y_radius) && bit_at(x_radius, y_radius) & BIT_FUNGUS
                && altitude_at(x_radius, y_radius) >= ALT_BIT_OCEAN_SHELF) {
                morale++;
            }
        }
        morale -= 2;
    } else { // everything else
        if (*TurnCurrentNum < 45) {
            morale = 0;
        } else if (*TurnCurrentNum < 90) {
            morale = 1;
        } else if (*TurnCurrentNum < 170) {
            morale = 2;
        } else if (*TurnCurrentNum < 250) {
            morale = 3;
        } else if (*TurnCurrentNum < 330) {
            morale = 4;
        } else { // 330+
            morale = 6;
        }
        if (faction_id_vs_native > 0) {
            morale += (PlayersData[faction_id_vs_native].major_atrocities != 0)
                + (TectonicDetonationCount[faction_id_vs_native] != 0);
        }
        if (veh_id >= 0) {
            if (Vehs[veh_id].state & VSTATE_MONOLITH_UPGRADED) {
                morale++;
            }
            if (Vehs[veh_id].proto_id == BSC_LOCUSTS_OF_CHIRON) {
                morale++;
            }
            morale += (Vehs[veh_id].flags >> 3) & 3; // 0x8|0x10 > +1, +2, or +3; TODO: id unk flags
        }
    }
    return range(morale, 0, 6);
}

uint32_t __cdecl morale_veh(uint32_t veh_id, BOOL check_drone_riot, int faction_id_vs_native) {
    uint32_t faction_id = Vehs[veh_id].faction_id;
    if (!faction_id) {
        return morale_alien(veh_id, faction_id_vs_native);
    }
    int16_t proto_id = Vehs[veh_id].proto_id;
    if (VehPrototypes[proto_id].plan == PLAN_INFO_WARFARE) { // probe
        int probe_morale = range(PlayersData[faction_id].soc_effect_active.probe, 0, 3);
        probe_morale += has_project(SP_TELEPATHIC_MATRIX, faction_id) ? 2 : 0;
        for (int i = 0; i < MaxTechnologyNum; i++) {
            if (Technology[i].flags & TFLAG_IMPROVED_PROBES && has_tech(i, faction_id)) {
                probe_morale++;
            }
        }
        probe_morale += Vehs[veh_id].morale;
        return range(probe_morale, 2, 6);
    }
    int8_t offense_rating = get_proto_offense_rating(proto_id);
    if (proto_id < MaxVehProtoFactionNum && offense_rating < 0) {
        return range(Vehs[veh_id].morale, 0, 6); // Basic Psi Veh
    }
    // everything else
    int morale_modifier = range(PlayersData[faction_id].soc_effect_active.morale, -4, 4);
    if (morale_modifier <= -2) {
        morale_modifier++;
    } else if (morale_modifier >= 2) {
        morale_modifier--;
    }
    int rule_morale = Players[faction_id].rule_morale; // different from 'SOCIAL, MORALE'
    if (rule_morale < 0) { // negative effects 1st
        morale_modifier += rule_morale;
    }
    int home_base_id = Vehs[veh_id].home_base_id;
    if (home_base_id >= 0) { // home base countering negative effects
        if (has_fac_built(FAC_CHILDREN_CRECHE, home_base_id) && morale_modifier < 0) {
            morale_modifier /= 2;
        }
        if (has_fac_built(FAC_BROOD_PIT, home_base_id) && proto_id < MaxVehProtoFactionNum
            && (offense_rating < 0 || proto_id == BSC_SPORE_LAUNCHER) && morale_modifier < 0) {
            morale_modifier /= 2; // never reached due to above 'Basic Psi Veh' checks
        }
    }
    if (rule_morale > 0) {
        morale_modifier += rule_morale;
    }
    BOOL morale_flag = Players[faction_id].rule_flags & RFLAG_MORALE;
    if (morale_flag && morale_modifier < 0) {
        morale_modifier = 0;
    }
    if (check_drone_riot && home_base_id >= 0
        && Bases[home_base_id].state & BSTATE_DRONE_RIOTS_ACTIVE && !morale_flag) {
        // bug fix: removed premature range bounding negating negative morale effects
        morale_modifier--;
    }
    return range(Vehs[veh_id].morale + morale_modifier, 0, 6);
}
#undef __cdecl
static int32_t ability_prerequisite[29];static unsigned ability_count;
/* Adaptation of original has_abil for the four speed_proto query bits only.
 * Alien Deep Radar and Nethack cannot answer these bits, so are outside scope.
 * Production rejects invalid indexes before this original-domain reference. */
static inline bool has_speed_ability(unsigned prototype,unsigned ability){
 if(VehPrototypes[prototype].ability_flags&ability)return true;
 const unsigned faction=prototype/64;if(!faction)return false;
 const auto& f=Players[faction];
 for(int i=0;i<f.faction_bonus_count;++i)if(f.faction_bonus_id[i]==15){
  int id=f.faction_bonus_val1[i];if(id<0 || (unsigned)id>=ability_count){unexpected=true;return false;}
  if(has_tech(ability_prerequisite[id],(int)faction) && (ability&(1u<<id)))return true;
 }
 return false;
}
static void setup(const alpha::OriginalWorld& w,const alpha::UnitDefinitions& d,const alpha::Rules& tech,size_t index){
 ability_count=(unsigned)d.ability_count;for(unsigned a=0;a<ability_count;++a)ability_prerequisite[a]=d.abilities[a].prerequisite;
 world=&w;unexpected=false;const auto* v=w.vehicle(index);Vehs[0]=v?Vehicle{v->x,v->y,v->unit_id,v->home_base_id,v->faction_id,v->morale,v->flags,v->state}:Vehicle{};
 for(size_t i=0;i<512;++i)VehPrototypes[i]=*w.prototype(i);
 for(size_t i=0;i<w.game().base_count;++i)Bases[i].state=(uint32_t)w.base(i)->state_flags;
 for(unsigned i=0;i<8;++i){const auto& f=*w.faction(i);PlayersData[i]={{f.SE_probe,f.SE_morale},f.major_atrocities};Players[i]=*w.faction_definition(i);TectonicDetonationCount[i]=0;}
 if(w.game().tectonic.size==32)for(unsigned i=0;i<8;++i){const auto* p=w.game().tectonic.data+4*i;uint32_t n=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);TectonicDetonationCount[i]=n<=0x7fffffff?(int32_t)n:-1-(int32_t)(~n);}
 for(unsigned i=0;i<89;++i){const auto& t=tech.technologies[i];Technology[i]={t.flags,t.prerequisites[0],t.prerequisites[1]};GameTechAchieved[i]=w.game().preferences.TechOwners[i];}
 for(unsigned i=0;i<d.weapon_count;++i)weapon_offense[i]=d.weapons[i].offense;
 turn=w.game().preferences.CurrentTurn;BSC_FUNGAL_TOWER=d.weapon_count==26?19:-999;BSC_SPORE_LAUNCHER=d.weapon_count==26?15:-999;
}
}
#endif
