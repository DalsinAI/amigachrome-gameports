/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_unit_stats.h"
#include "upstream_world_stats_reference.h"
#include "original_world.h"
#include "upstream_saved_speed_reference.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static alpha::UnitDefinitions definitions;
static alpha::Rules technologies;
static alpha::BasicRules basic;
static alpha::WorldRules world_rules;
static void word(uint32_t& h,uint32_t n){for(unsigned s=0;s<32;s+=8)h=(h^((n>>s)&255u))*16777619u;}
static uint8_t* read_file(const char* path,size_t& size,size_t max){
 FILE* f=fopen(path,"rb");if(!f)return nullptr;
 if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long n=ftell(f);rewind(f);
 if(n<=0 || (size_t)n>max){fclose(f);return nullptr;}auto* data=(uint8_t*)malloc((size_t)n);if(!data){fclose(f);return nullptr;}
 bool valid=fread(data,1,(size_t)n,f)==(size_t)n;fclose(f);if(!valid){free(data);return nullptr;}size=(size_t)n;return data;
}
int main(int argc,char** argv){
 if(argc!=4 || (strcmp(argv[1],"base") && strcmp(argv[1],"expansion")))return 20;
 const bool base=strcmp(argv[1],"base")==0;const auto variant=base?alpha::Variant::base:alpha::Variant::alien_crossfire;
 size_t size=0;uint8_t* data=read_file(argv[2],size,1048576);if(!data)return 20;
 bool valid=alpha::read_technologies(data,size,technologies)==alpha::Error::ok &&
  alpha::read_unit_definitions(data,size,technologies,variant,definitions)==alpha::Error::ok &&
  alpha::read_basic_rules(data,size,technologies,variant,basic,world_rules)==alpha::Error::ok;
 free(data);if(!valid)return 20;
 data=read_file(argv[3],size,64u*1024u*1024u);if(!data)return 20;
 alpha::OriginalWorld w;auto error=w.load(data,size,0x12345678u);free(data);if(error!=alpha::WorldLoadError::ok)return 20;
 if((w.game().data_version<11)!=base){puts("Unsupported original game/rules component ID profile");return 20;}
 world_stats_reference::setup(w,definitions,technologies,0);
 uint32_t h=2166136261u,cases=0;
 for(size_t i=0;i<w.game().vehicle_count;++i){
  const auto& v=*w.vehicle(i);const auto& p=*w.prototype((size_t)v.unit_id);
  world_stats_reference::setup(w,definitions,technologies,i);
  uint32_t ability_flags=0x31415926u;
  auto ability_error=alpha::world_speed_ability_flags(w,definitions,technologies,(size_t)v.unit_id,ability_flags);
  if(ability_error!=alpha::LogicError::ok){if(ability_flags!=0x31415926u)return 21;printf("ERROR abilities=%d unit=%lu\n",(int)ability_error,(unsigned long)i);return 20;}
  alpha::OriginalPrototype effective=p;
  const int ability_triad=definitions.chassis[p.chassis_id].triad;
  for(unsigned bit=0;bit<definitions.ability_count;++bit)if((bit==5 || bit==10 || bit==23 || bit==27) && (ability_triad==2 || bit==10 || bit==23)){
   if(world_stats_reference::has_speed_ability((unsigned)v.unit_id,1u<<bit))effective.ability_flags|=1u<<bit;
  }
  if(world_stats_reference::unexpected || effective.ability_flags!=ability_flags)return 21;
  uint32_t expected[2];
  for(unsigned riots=0;riots<2;++riots){
   uint32_t morale=123;auto result=alpha::world_effective_morale(w,definitions,technologies,i,riots!=0,morale);
   if(result!=alpha::LogicError::ok){if(morale!=123)return 21;printf("ERROR morale=%d unit=%lu\n",(int)result,(unsigned long)i);return 20;}
   expected[riots]=world_stats_reference::morale_veh(0,riots!=0,0);
   if(world_stats_reference::unexpected || morale!=expected[riots])return 21;
   word(h,(uint32_t)i);word(h,riots);word(h,morale);++cases;
  }
  for(unsigned skip=0;skip<2;++skip){
   uint32_t speed=123;auto result=alpha::world_vehicle_speed(w,definitions,technologies,i,basic.move_rate_roads,skip!=0,speed);
   if(result!=alpha::LogicError::ok && result!=alpha::LogicError::overflow){if(speed!=123)return 21;printf("ERROR speed=%d unit=%lu\n",(int)result,(unsigned long)i);return 20;}
   const int triad=definitions.chassis[p.chassis_id].triad;
   const bool tower=definitions.weapon_count==26 && v.unit_id==19;
   alpha::SavedSpeedContext ctx{false,false,skip!=0,(int32_t)expected[1]};
   if(!tower && triad==2)ctx.cloudbase_academy=world_stats_reference::has_project(35,(unsigned)v.unit_id/64);
   if(!tower && triad==1)ctx.maritime_control_center=world_stats_reference::has_project(10,v.faction_id);
   saved_speed_reference::setup(definitions,effective,v,basic.move_rate_roads,ctx);uint32_t source=saved_speed_reference::speed(0,ctx.skip_morale);
   if(world_stats_reference::unexpected || saved_speed_reference::unexpected)return 21;
   if(source>255){if(result!=alpha::LogicError::overflow || speed!=123)return 21;}
   else if(result!=alpha::LogicError::ok || speed!=source)return 21;
   word(h,skip);word(h,source);word(h,(uint32_t)result);++cases;
   bool dependency=false;
   const auto* owner=w.faction_definition((size_t)v.unit_id/64);
   const uint32_t queried=triad==2?(0x800420u|(definitions.ability_count>27?0x8000000u:0u)):0x800400u;
   if(definitions.weapon_count==26 && v.unit_id>=64)for(int b=0;b<owner->faction_bonus_count;++b)if(owner->faction_bonus_id[b]==15){
    const int a=owner->faction_bonus_val1[b];if(a>=0 && (unsigned)a<definitions.ability_count && (queried&(1u<<a)) && !(p.ability_flags&(1u<<a)))dependency=true;
   }
   if(skip && !dependency){alpha::Rules absent{};uint32_t lazy=123;if(alpha::world_vehicle_speed(w,definitions,absent,i,basic.move_rate_roads,true,lazy)!=result || lazy!=speed)return 21;}
  }
 }
 unsigned queries=0;
 for(unsigned faction=0;faction<8;++faction){
  for(unsigned project=0;project<64;++project){bool value=true;auto result=alpha::world_has_project(w,project,(int32_t)faction,value);
   if(result!=alpha::LogicError::ok){if(!value)return 21;printf("ERROR project=%d\n",(int)result);return 20;}
   bool source=world_stats_reference::has_project(project,faction);if(world_stats_reference::unexpected || value!=source)return 21;word(h,value);++queries;
  }
  for(int tech=-2;tech<90;++tech){bool value=true;if(alpha::world_has_technology(w,technologies,tech,(int32_t)faction,value)!=alpha::LogicError::ok)return 21;
   bool source=world_stats_reference::has_tech(tech,(int)faction);if(value!=source)return 21;word(h,value);++queries;
  }
 }
 uint32_t sentinel=123;bool flag=true;
 if(alpha::world_speed_ability_flags(w,definitions,technologies,512,sentinel)!=alpha::LogicError::bad_state || sentinel!=123 ||
    alpha::world_effective_morale(w,definitions,technologies,w.game().vehicle_count,true,sentinel)!=alpha::LogicError::bad_state || sentinel!=123 ||
    alpha::world_vehicle_speed(w,definitions,technologies,w.game().vehicle_count,basic.move_rate_roads,false,sentinel)!=alpha::LogicError::bad_state || sentinel!=123 ||
    alpha::world_has_project(w,64,5,flag)!=alpha::LogicError::bad_state || !flag ||
    alpha::world_has_technology(w,technologies,0,8,flag)!=alpha::LogicError::bad_state || !flag)return 21;
 printf("version=%lu vehicles=%lu stats=%lu queries=%u digest=%08lx rc=0\n",(unsigned long)w.game().data_version,(unsigned long)w.game().vehicle_count,(unsigned long)cases,queries,(unsigned long)h);return 0;
}
