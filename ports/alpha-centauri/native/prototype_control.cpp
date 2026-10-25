/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include "prototype.h"
#include "upstream_prototype_reference.h"
namespace prototype_control {
static void word(uint32_t& h,uint32_t value) { for(unsigned shift=0;shift<32;shift+=8)h=(h^((value>>shift)&255u))*16777619u; }
static void setup(const alpha::UnitDefinitions& d,int32_t road) {
  using namespace prototype_reference;
  unsupported=false;MaxAbilityNum=(int)d.ability_count;Rules->move_rate_roads=road;
  for(size_t i=0;i<9;++i)Chassis[i]={(uint8_t)d.chassis[i].speed,(uint8_t)d.chassis[i].triad,(uint8_t)d.chassis[i].range,(uint8_t)d.chassis[i].missile,(uint8_t)d.chassis[i].cargo,(uint8_t)d.chassis[i].cost};
  for(size_t i=0;i<d.weapon_count;++i)Weapon[i]={(int8_t)d.weapons[i].offense,(uint8_t)d.weapons[i].mode,(uint8_t)d.weapons[i].cost};
  for(size_t i=0;i<d.armor_count;++i)Armor[i]={(int8_t)d.armor[i].defense,(uint8_t)d.armor[i].cost};
  for(size_t i=0;i<d.ability_count;++i)Ability[i]={d.abilities[i].cost_factor,d.abilities[i].flags};
}
int cost_case(const alpha::UnitDefinitions& d,uint32_t c,uint32_t w,uint32_t a,uint32_t abilities,uint32_t reactor,uint32_t& digest) {
  setup(d,3);
  uint32_t expected=prototype_reference::proto_cost(c,w,a,abilities,reactor);
  uint32_t actual=12345;
  if(alpha::calculate_prototype_cost(d,c,w,a,abilities,reactor,actual)!=alpha::LogicError::ok || actual!=expected || prototype_reference::unsupported)return 1;
  word(digest,actual);return 0;
}
int derived_case(const alpha::UnitDefinitions& d,size_t id,int32_t road,uint32_t& digest) {
  using namespace prototype_reference;
  setup(d,road);const auto& b=d.units[id];
  const uint32_t auto_cost=proto_cost((uint32_t)b.chassis,(uint32_t)b.weapon,(uint32_t)b.armor,b.abilities,(uint32_t)b.reactor);
  const uint32_t auto_carry=d.weapons[b.weapon].mode==7?transport_val((uint32_t)b.chassis,b.abilities,(uint32_t)b.reactor):0;
  alpha::PredefinedPrototype actual{};actual.flags=12345;
  auto error=alpha::derive_predefined_prototype(d,id,road,actual);
  // Explicit storage-domain policy: never silently wrap derived byte fields.
  if((!b.cost && auto_cost>255) || (!b.carry && auto_carry>255)) {
    if(error!=alpha::LogicError::overflow || actual.flags!=12345)return 2;
    word(digest,0xffffffffu);return 0;
  }
  if(error!=alpha::LogicError::ok)return 3;
  make_proto((int)id,(uint32_t)b.chassis,(uint32_t)b.weapon,(uint32_t)b.armor,b.abilities,(uint32_t)b.reactor);
  auto& p=VehPrototypes[id];
  if(b.plan!=-1)p.plan=(uint8_t)b.plan;
  if(b.cost)p.cost=(uint8_t)b.cost;
  if(b.carry)p.carry_capacity=(uint8_t)b.carry;
  p.icon_offset=(int8_t)b.icon;p.preq_tech=(int16_t)b.prerequisite;
  uint32_t expected_speed=speed_proto((uint32_t)id);
  const int32_t expected[]={p.chassis_id,p.weapon_id,p.armor_id,p.reactor_id,p.preq_tech,(int32_t)p.ability_flags,
    p.cost,p.carry_capacity,p.plan,p.unk_1,p.icon_offset,p.obsolete_factions,p.combat_factions,p.flags,(int32_t)expected_speed};
  const int32_t values[]={actual.chassis,actual.weapon,actual.armor,actual.reactor,actual.prerequisite,(int32_t)actual.abilities,
    actual.cost,actual.carry,actual.plan,actual.role_code,actual.icon,actual.obsolete_factions,actual.combat_factions,(int32_t)actual.flags,(int32_t)actual.speed_points};
  if(unsupported)return 4;
  for(size_t i=0;i<15;++i){if(values[i]!=expected[i])return 5;word(digest,(uint32_t)values[i]);}
  for(size_t i=0;i<32;++i){if(actual.name[i]!=b.name[i])return 6;if(!actual.name[i])break;}
  return 0;
}
void fixture(alpha::UnitDefinitions& d,bool expansion) {
  d=alpha::UnitDefinitions{};d.weapon_count=expansion?26:21;d.armor_count=expansion?14:10;d.ability_count=expansion?29:24;d.unit_count=64;
  const int32_t speeds[]={1,2,3,4,6,8,8,8,12};
  for(size_t i=0;i<9;++i){d.chassis[i].speed=speeds[i];d.chassis[i].cost=speeds[i];d.chassis[i].cargo=2;d.chassis[i].triad=i<3?0:i<5?1:2;d.chassis[i].missile=i==8;}
  for(size_t i=0;i<d.weapon_count;++i){d.weapons[i].offense=(int32_t)i+1;d.weapons[i].mode=(int32_t)(i%3);d.weapons[i].cost=(int32_t)(i%10)+1;}
  d.weapons[1].offense=99;d.weapons[2].mode=7;d.weapons[3].mode=8;d.weapons[4].mode=9;d.weapons[5].mode=10;d.weapons[6].mode=11;d.weapons[7].mode=12;
  for(size_t i=0;i<d.armor_count;++i){d.armor[i].defense=(int32_t)i+1;d.armor[i].cost=(int32_t)(i%10)+1;}
  for(size_t i=0;i<d.ability_count;++i){d.abilities[i].cost_factor=(int32_t)i-7;d.abilities[i].flags=i%2?0x400u:0;}
  for(size_t i=0;i<64;++i){auto& b=d.units[i];b.name[0]='U';b.name[1]=(char)('A'+i%26);b.name[2]=0;b.chassis=(int32_t)(i%9);b.weapon=(int32_t)(i%d.weapon_count);b.armor=(int32_t)(i%d.armor_count);b.reactor=(int32_t)(i%4)+1;b.plan=-1;b.prerequisite=-1;b.icon=-1;b.abilities=1u<<(i%d.ability_count);}
}
int synthetic(uint32_t& digest) {
  static alpha::UnitDefinitions d;
  for(unsigned variant=0;variant<2;++variant) {
    fixture(d,variant!=0);
    for(uint32_t c=0;c<9;++c)for(uint32_t r=1;r<=4;++r)for(uint32_t combo=0;combo<32;++combo) {
      const uint32_t ability=combo?((1u<<(combo%d.ability_count))|(1u<<((combo*3)%d.ability_count))):0;
      int rc=cost_case(d,c,(combo*7)%(uint32_t)d.weapon_count,(combo*3)%(uint32_t)d.armor_count,ability,r,digest);if(rc)return rc;
    }
    for(size_t i=0;i<64;++i)for(unsigned rate=0;rate<4;++rate) {
      const int32_t roads[]={1,3,10,100};int rc=derived_case(d,i,roads[rate],digest);if(rc)return rc;
    }
    d.units[0].cost=7;d.units[0].carry=3;d.units[0].plan=12;d.units[0].icon=5;
    int rc=derived_case(d,0,3,digest);if(rc)return rc;
  }
  alpha::PredefinedPrototype output{};output.flags=12345;uint32_t cost=12345;
  if(alpha::derive_predefined_prototype(d,64,3,output)!=alpha::LogicError::bad_state || output.flags!=12345)return 7;
  if(alpha::derive_predefined_prototype(d,0,0,output)!=alpha::LogicError::bad_state || output.flags!=12345)return 8;
  if(alpha::calculate_prototype_cost(d,9,0,0,0,1,cost)!=alpha::LogicError::bad_state || cost!=12345)return 9;
  if(alpha::calculate_prototype_cost(d,0,0,0,0,0,cost)!=alpha::LogicError::bad_state || cost!=12345)return 10;
  if(alpha::calculate_prototype_cost(d,0,0,0,0x80000000u,1,cost)!=alpha::LogicError::bad_state || cost!=12345)return 11;
  d.abilities[0].cost_factor=2147483647;d.abilities[1].cost_factor=1;
  if(alpha::calculate_prototype_cost(d,0,0,0,3,1,cost)!=alpha::LogicError::overflow || cost!=12345)return 12;
  return 0;
}
}
#ifndef ALPHA_CONTROL_NO_MAIN
int main(){uint32_t digest=2166136261u;int rc=prototype_control::synthetic(digest);
#ifndef ALPHA_CONTROL_RAW
 printf("cost=2304 derived=514 invalid=6 digest=%08lx rc=%d\n",(unsigned long)digest,rc);
#endif
 return rc;}
#endif
