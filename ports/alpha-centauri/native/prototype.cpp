/* SPDX-License-Identifier: GPL-3.0-or-later. See prototype.h for source pin. */
#include "prototype.h"
namespace alpha {
static bool word(int64_t value) { return value >= (-2147483647LL-1) && value <= 2147483647LL; }
static bool components(const UnitDefinitions& d,uint32_t c,uint32_t w,uint32_t a,uint32_t abilities,uint32_t r) {
  if (!((d.weapon_count==21 && d.armor_count==10 && d.ability_count==24) ||
        (d.weapon_count==26 && d.armor_count==14 && d.ability_count==29))) return false;
  return c<9 && w<d.weapon_count && a<d.armor_count && r>=1 && r<=4 &&
    !(abilities & ~((1u << d.ability_count)-1u));
}
LogicError calculate_prototype_cost(const UnitDefinitions& d,uint32_t chassis,uint32_t weapon,
                                    uint32_t armor,uint32_t abilities,uint32_t reactor,uint32_t& output) {
  if (!components(d,chassis,weapon,armor,abilities,reactor)) return LogicError::bad_state;
  const auto& c=d.chassis[chassis];const auto& w=d.weapons[weapon];const auto& a=d.armor[armor];
  const int64_t weapon_cost=w.cost;
  if (c.missile && w.offense>=99) { output=(uint32_t)weapon_cost;return LogicError::ok; }
  int64_t armor_cost=a.cost,speed_cost=c.cost,modifier=0,land_flags=0;
  for(size_t i=0;i<d.ability_count;++i) if (abilities&(1u<<i)) {
    if (modifier) { ++modifier;if(!word(modifier))return LogicError::overflow; }
    const int64_t factor=d.abilities[i].cost_factor;
    if(factor>0)modifier+=factor;
    else switch(factor) {
      case -1: if(armor_cost) { int64_t ratio=weapon_cost/armor_cost;modifier+=ratio>2?2:ratio; }break;
      case -2:modifier+=weapon_cost-1;break;
      case -3:modifier+=armor_cost-1;break;
      case -4:modifier+=speed_cost-1;break;
      case -5:modifier+=weapon_cost+armor_cost-2;break;
      case -6:modifier+=weapon_cost+speed_cost-2;break;
      case -7:modifier+=armor_cost+speed_cost-2;break;
      default:break;
    }
    if(!word(modifier))return LogicError::overflow;
    if((d.abilities[i].flags&0x400u) && c.triad==0)++land_flags;
  }
  modifier+=land_flags;if(!word(modifier))return LogicError::overflow;
  if(c.triad==1) { armor_cost/=2;speed_cost+=reactor; }
  else if(c.triad==2) { if(armor_cost>1)armor_cost*=reactor*2;speed_cost+=reactor*2; }
  int64_t combat=armor_cost/2+1;if(combat<weapon_cost)combat=weapon_cost;
  int64_t proto;
  if(combat==1 && armor_cost==1 && speed_cost==1 && reactor==1)proto=1;
  else {
    const int64_t denominator=2LL<<reactor;
    const int64_t numerator=(speed_cost+armor_cost)*combat+denominator/2;
    if(!word(numerator))return LogicError::overflow;
    proto=numerator/denominator;
    if(speed_cost==1)proto=proto/2+1;
    if(weapon_cost>1 && a.cost>1) { ++proto;if(c.triad==0 && speed_cost>1)++proto; }
    if(c.triad==1 && w.mode!=11)proto=(proto+1)/2;
    else if(c.triad==2)proto/=w.mode>2?2:4;
    const int64_t minimum=(reactor*3+1)/2;
    if(proto<minimum)proto=minimum;
  }
  const int64_t factor=modifier+4;
  const int64_t price=proto*factor+2;
  if(!word(factor) || !word(price) || price<0)return LogicError::overflow;
  output=(uint32_t)(price/4);return LogicError::ok;
}
LogicError derive_predefined_prototype(const UnitDefinitions& d,size_t id,int32_t road_rate,PredefinedPrototype& output) {
  if(d.unit_count>64 || id>=d.unit_count || road_rate<1 || road_rate>100)return LogicError::bad_state;
  const auto& u=d.units[id];
  if(u.chassis<0 || u.weapon<0 || u.armor<0 || u.reactor<0 ||
      !components(d,(uint32_t)u.chassis,(uint32_t)u.weapon,(uint32_t)u.armor,u.abilities,(uint32_t)u.reactor))return LogicError::bad_state;
  const auto& c=d.chassis[u.chassis];const auto& w=d.weapons[u.weapon];const auto& a=d.armor[u.armor];
  uint32_t price=0;
  auto error=calculate_prototype_cost(d,(uint32_t)u.chassis,(uint32_t)u.weapon,(uint32_t)u.armor,u.abilities,(uint32_t)u.reactor,price);
  if(error!=LogicError::ok)return error;
  int64_t carry=w.mode==7?c.cargo:0;
  if(w.mode==7) {
    if(c.triad==1)carry*=u.reactor;
    if(u.abilities&0x800000u)carry/=2;
    if(u.abilities&0x80000u)carry=(carry*3+1)/2;
  }
  int32_t plan=0,role=0;
  if(c.missile) {
    if(w.offense<99) {
      if(u.weapon==24) { plan=13;role=22; }
      else if(u.weapon==25) { plan=14;role=23; }
      else { plan=0;role=13; }
    } else { plan=5;role=14; }
  } else if(w.mode>=7) { plan=w.mode;role=w.mode+32; }
  else if(c.triad==1) { plan=6;role=6; }
  else if(c.triad==2) {
    if(u.abilities&0x20u) { plan=4;role=9; }
    else { plan=0;role=8; }
  } else {
    plan=a.defense>1?2:0;
    if(w.offense>=a.defense && plan==2)plan=1;
    if(c.speed>1 && w.offense>a.defense)plan=0;
    if(u.abilities&0x8018u)plan=0;
    role=3;if(a.defense>w.offense)role=2;if(w.offense>a.defense*2)role=1;
    if(c.speed==2)role=4;else if(c.speed>=3)role=5;
    if(w.offense==1 && a.defense==1) {
      if(c.speed>1)plan=3;
      if(u.abilities&0x400000u)plan=2;
      role=10;
    }
  }
  if(u.plan!=-1)plan=u.plan;
  if(u.cost)price=(uint32_t)u.cost;
  if(u.carry)carry=u.carry;
  if(price>255 || carry<0 || carry>255 || plan<0 || plan>255 || role<-128 || role>127)return LogicError::overflow;
  int64_t speed=c.speed;
  if(id==19)speed=0; // immobile Fungal Tower by predefined index
  else {
    if(c.triad==2)speed+=u.reactor*2;
    if(w.mode==7)--speed;
    if(u.abilities&0x800000u)--speed;
    if(u.abilities&0x400u)speed+=c.triad==2?u.reactor*2:1;
    if(c.triad==2) {
      if(u.abilities&0x8000000u)speed+=2;
      // Predefined IDs have faction0: no faction Cloudbase project bonus.
      if(u.abilities&0x20u)speed=(speed*4)/5;
      if(w.mode==7)speed/=2;
    }
    speed=(speed<1?1:speed>99?99:speed)*road_rate;
  }
  PredefinedPrototype result{};
  bool terminated=false;
  for(size_t i=0;i<sizeof(result.name);++i) { result.name[i]=u.name[i];if(!u.name[i]){terminated=true;break;} }
  if(!terminated)return LogicError::bad_state;
  result.chassis=u.chassis;result.weapon=u.weapon;result.armor=u.armor;result.reactor=u.reactor;
  result.prerequisite=u.prerequisite;result.abilities=u.abilities;result.cost=(int32_t)price;
  result.carry=(int32_t)carry;result.plan=plan;result.role_code=role;result.icon=u.icon;
  result.obsolete_factions=0;result.combat_factions=-1;result.flags=1;result.speed_points=(uint32_t)speed;
  output=result;return LogicError::ok;
}
}
