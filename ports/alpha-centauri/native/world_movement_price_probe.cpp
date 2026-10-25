/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_movement_price.h"
#include "upstream_world_movement_reference.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static alpha::UnitDefinitions definitions;static alpha::Rules technologies;
static alpha::BasicRules basic;static alpha::WorldRules world_rules;
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
 if((w.game().data_version<11)!=base){puts("ERROR profile");return 20;}
 const alpha::MapGeometry geometry{(int32_t)w.game().map.info.width,(int32_t)w.game().map.info.height,w.game().map.info.flat};
 const int ox[8]={1,2,1,0,-1,-2,-1,0},oy[8]={-1,0,1,2,1,0,-1,-2};
 uint32_t h=2166136261u,cases=0,rejected=0;
 for(size_t i=0;i<w.game().vehicle_count;++i){
  const auto& v=*w.vehicle(i);
  world_movement_reference::setup(w,definitions,technologies,i,basic.move_rate_roads);
  for(unsigned edge=0;edge<8;++edge){
   int32_t x=v.x+ox[edge],y=v.y+oy[edge];
   if(alpha::movement_wrap_x(geometry,x,x)!=alpha::LogicError::ok)return 21;
   size_t index;if(alpha::movement_tile_index(geometry,x,y,index)!=alpha::LogicError::ok)continue;
   const bool self=x==v.x && y==v.y;
   for(unsigned rough=0;rough<2;++rough){
    uint32_t price=123;auto result=alpha::world_movement_price(w,definitions,technologies,i,basic.move_rate_roads,x,y,rough!=0,price);
    if(self){if(result!=alpha::LogicError::bad_state || price!=123)return 21;++rejected;continue;}
    if(result!=alpha::LogicError::ok){if(price!=123)return 21;printf("ERROR price=%d unit=%lu\n",(int)result,(unsigned long)i);return 20;}
    uint32_t source=world_movement_reference::hex_cost(v.unit_id,v.faction_id,v.x,v.y,x,y,rough!=0);
    if(world_movement_reference::unexpected || source!=price){printf("MISMATCH unit=%lu edge=%u rough=%u price=%lu source=%lu\n",(unsigned long)i,edge,rough,(unsigned long)price,(unsigned long)source);return 21;}
    word(h,(uint32_t)i);word(h,edge);word(h,rough);word(h,price);++cases;
   }
  }
 }
 uint32_t sentinel=123;
 if(alpha::world_movement_price(w,definitions,technologies,w.game().vehicle_count,basic.move_rate_roads,0,0,false,sentinel)!=alpha::LogicError::bad_state || sentinel!=123)return 21;
 if(w.game().vehicle_count){const auto& v=*w.vehicle(0);
  if(alpha::world_movement_price(w,definitions,technologies,0,basic.move_rate_roads,v.x,v.y,false,sentinel)!=alpha::LogicError::bad_state || sentinel!=123 ||
     alpha::world_movement_price(w,definitions,technologies,0,0,v.x,v.y,false,sentinel)!=alpha::LogicError::bad_state || sentinel!=123)return 21;
 }
 printf("version=%lu vehicles=%lu prices=%lu rejected=%lu digest=%08lx rc=0\n",(unsigned long)w.game().data_version,(unsigned long)w.game().vehicle_count,(unsigned long)cases,(unsigned long)rejected,(unsigned long)h);return 0;
}
