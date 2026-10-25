/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "saved_speed.h"
#include "upstream_saved_speed_reference.h"
#include <stdio.h>
static void word(uint32_t& h,uint32_t n){for(unsigned s=0;s<32;s+=8)h=(h^((n>>s)&255u))*16777619u;}
int main(){
 alpha::UnitDefinitions d{};uint32_t cases=0,overflow=0,digest=2166136261u;
 const uint32_t flags[]={0,0x400u,0x800000u,0x20u,0x800420u,0x8000000u,0x8800420u};
 const int ids[]={0,2,8,19,63,64,320,511},roads[]={1,3,100},damage[]={0,1,9,10,11,19,39,40,255};
 for(unsigned variant=0;variant<2;++variant){
  d.weapon_count=variant?26:21;d.armor_count=variant?14:10;d.ability_count=variant?29:24;
  for(unsigned c=0;c<9;++c){d.chassis[c].triad=c<3?0:c<5?1:2;d.chassis[c].speed=(int32_t)c+1;}
  for(unsigned w=0;w<d.weapon_count;++w){d.weapons[w].mode=w==7?7:0;d.weapons[w].offense=w==12?-1:1;}
  for(int id:ids)for(unsigned chassis=0;chassis<9;++chassis)for(unsigned reactor=1;reactor<=4;++reactor)
  for(unsigned a=0;a<(variant?7u:5u);++a)for(unsigned options=0;options<16;++options)for(int road:roads)for(int hurt:damage){
   alpha::OriginalPrototype p{};p.chassis_id=(uint8_t)chassis;p.reactor_id=(uint8_t)reactor;p.weapon_id=(uint8_t)(options&1?7:options&2?12:0);p.armor_id=0;p.ability_flags=flags[a];p.plan=(int8_t)(options&4?12:3);
   alpha::OriginalVehicle v{};v.unit_id=(int16_t)id;v.faction_id=5;v.damage_taken=(uint8_t)hurt;
   alpha::SavedSpeedContext context{(options&1)!=0,(options&2)!=0,(options&8)!=0,options&4?6:3};
   saved_speed_reference::setup(d,p,v,road,context);uint32_t expected_proto=saved_speed_reference::speed_proto((uint32_t)id);
   uint32_t expected=saved_speed_reference::speed(0,context.skip_morale);if(saved_speed_reference::unexpected)return 1;
   uint32_t actual_proto=0xdeadbeefu,actual=0xdeadbeefu;
   if(alpha::saved_prototype_speed(d,p,(size_t)id,road,context.cloudbase_academy,actual_proto)!=alpha::LogicError::ok || actual_proto!=expected_proto)return 2;
   auto error=alpha::saved_vehicle_speed(d,p,v,road,context,actual);
   if(expected>255){++overflow;if(error!=alpha::LogicError::overflow || actual!=0xdeadbeefu)return 3;}
   else if(error!=alpha::LogicError::ok || actual!=expected)return 4;
   word(digest,expected_proto);word(digest,expected);word(digest,(uint32_t)error);++cases;
  }
 }
 unsigned invalid=0;d.weapon_count=26;d.armor_count=14;d.ability_count=29;
 alpha::OriginalPrototype good{};good.reactor_id=1;alpha::OriginalVehicle vehicle{};vehicle.unit_id=320;vehicle.faction_id=5;alpha::SavedSpeedContext context{false,false,false,3};
 for(unsigned which=0;which<12;++which){
  auto p=good;auto v=vehicle;auto ctx=context;int road=3;
  switch(which){case 0:p.chassis_id=9;break;case 1:p.weapon_id=26;break;case 2:p.armor_id=14;break;case 3:p.reactor_id=0;break;case 4:p.reactor_id=5;break;case 5:p.ability_flags=0x80000000u;break;case 6:v.unit_id=-1;break;case 7:v.unit_id=512;break;case 8:v.faction_id=8;break;case 9:ctx.effective_morale=-1;break;case 10:ctx.effective_morale=7;break;case 11:road=0;break;}
  uint32_t output=123;if(alpha::saved_vehicle_speed(d,p,v,road,ctx,output)==alpha::LogicError::ok || output!=123)return 5;++invalid;
 }
 unsigned independent=0;
#ifdef ALPHA_SPEED_VECTORS
#include "saved_speed_vectors.inc"
 for(const auto& row:speed_vectors){
  d.weapon_count=row[0]?26:21;d.armor_count=row[0]?14:10;d.ability_count=row[0]?29:24;
  alpha::OriginalPrototype p{};p.chassis_id=(uint8_t)row[2];p.weapon_id=(uint8_t)row[3];p.reactor_id=(uint8_t)row[4];p.ability_flags=row[5];p.plan=(int8_t)row[6];
  alpha::OriginalVehicle v{};v.unit_id=(int16_t)row[1];v.faction_id=5;v.damage_taken=(uint8_t)row[8];
  alpha::SavedSpeedContext ctx{(row[9]&1)!=0,(row[9]&2)!=0,(row[9]&8)!=0,(int32_t)row[10]};
  uint32_t proto=123,speed=123;
  if(alpha::saved_prototype_speed(d,p,row[1],(int32_t)row[7],ctx.cloudbase_academy,proto)!=alpha::LogicError::ok || proto!=row[11])return 6;
  auto error=alpha::saved_vehicle_speed(d,p,v,(int32_t)row[7],ctx,speed);
  if(row[12]>255){if(error!=alpha::LogicError::overflow || speed!=123)return 7;}
  else if(error!=alpha::LogicError::ok || speed!=row[12])return 8;
  ++independent;
 }
#endif
 printf("independent=%u saved_speeds=%lu overflow=%lu invalid=%u digest=%08lx rc=0\n",independent,(unsigned long)cases,(unsigned long)overflow,invalid,(unsigned long)digest);return 0;
}
