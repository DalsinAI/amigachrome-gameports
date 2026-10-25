/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "saved_speed.h"
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
 uint32_t h=2166136261u,cases=0;
 for(size_t i=0;i<w.game().vehicle_count;++i){
  const auto& v=*w.vehicle(i);const auto& p=*w.prototype((size_t)v.unit_id);
  for(unsigned options=0;options<16;++options){
   // Explicit dependency controls; not a claim that saved raw morale equals effective morale.
   alpha::SavedSpeedContext ctx{(options&1)!=0,(options&2)!=0,(options&8)!=0,options&4?6:3};
   saved_speed_reference::setup(definitions,p,v,basic.move_rate_roads,ctx);
   uint32_t expected_proto=saved_speed_reference::speed_proto((uint32_t)v.unit_id),expected=saved_speed_reference::speed(0,ctx.skip_morale);
   if(saved_speed_reference::unexpected)return 21;
   uint32_t actual_proto=123,actual=123;
   if(alpha::saved_prototype_speed(definitions,p,(size_t)v.unit_id,basic.move_rate_roads,ctx.cloudbase_academy,actual_proto)!=alpha::LogicError::ok || actual_proto!=expected_proto)return 21;
   auto result=alpha::saved_vehicle_speed(definitions,p,v,basic.move_rate_roads,ctx,actual);
   if(expected>255){if(result!=alpha::LogicError::overflow || actual!=123)return 21;}
   else if(result!=alpha::LogicError::ok || actual!=expected)return 21;
   word(h,(uint32_t)i);word(h,(uint32_t)v.unit_id);word(h,options);word(h,expected_proto);word(h,expected);word(h,(uint32_t)result);++cases;
  }
 }
 printf("version=%lu vehicles=%lu speed_contexts=%lu digest=%08lx rc=0\n",(unsigned long)w.game().data_version,(unsigned long)w.game().vehicle_count,(unsigned long)cases,(unsigned long)h);return 0;
}
