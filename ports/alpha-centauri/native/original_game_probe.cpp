/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "original_game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void word(uint32_t& h,uint32_t n){for(unsigned s=0;s<32;s+=8)h=(h^((n>>s)&255u))*16777619u;}
static uint32_t digest(const uint8_t* p,size_t n){uint32_t h=2166136261u;for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;return h;}
static uint32_t hash(const alpha::OriginalGamePreferences& r){uint32_t h=2166136261u;
#define WIRE_SCALAR(type,name,off) word(h,(uint32_t)r.name);
#define WIRE_ARRAY(type,name,count,off) for(unsigned i=0;i<count;++i)word(h,(uint32_t)r.name[i]);
#define WIRE_STRING(name,count,off) for(unsigned i=0;i<count;++i)h=(h^(uint8_t)r.name[i])*16777619u;if(r.name[count]!=0)return 0;
#include "original_prefs_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
return h;}
static uint32_t hash(const alpha::OriginalVehicle& r){uint32_t h=2166136261u;
#define WIRE_SCALAR(type,name,off) word(h,(uint32_t)r.name);
#define WIRE_ARRAY(type,name,count,off) for(unsigned i=0;i<count;++i)word(h,(uint32_t)r.name[i]);
#define WIRE_STRING(name,count,off) for(unsigned i=0;i<count;++i)h=(h^(uint8_t)r.name[i])*16777619u;if(r.name[count]!=0)return 0;
#include "original_vehicle_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
return h;}
static uint32_t hash(const alpha::OriginalPrototype& r){uint32_t h=2166136261u;
#define WIRE_SCALAR(type,name,off) word(h,(uint32_t)r.name);
#define WIRE_ARRAY(type,name,count,off) for(unsigned i=0;i<count;++i)word(h,(uint32_t)r.name[i]);
#define WIRE_STRING(name,count,off) for(unsigned i=0;i<count;++i)h=(h^(uint8_t)r.name[i])*16777619u;if(r.name[count]!=0)return 0;
#include "original_prototype_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
return h;}
static uint32_t hash(const alpha::OriginalBase& r){uint32_t h=2166136261u;
#define WIRE_SCALAR(type,name,off) word(h,(uint32_t)r.name);
#define WIRE_ARRAY(type,name,count,off) for(unsigned i=0;i<count;++i)word(h,(uint32_t)r.name[i]);
#define WIRE_STRING(name,count,off) for(unsigned i=0;i<count;++i)h=(h^(uint8_t)r.name[i])*16777619u;if(r.name[count]!=0)return 0;
#include "original_base_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
return h;}
int main(int argc,char** argv){
 if(argc!=2)return 20;
 FILE* f=fopen(argv[1],"rb");if(!f)return 20;
 if(fseek(f,0,SEEK_END)){fclose(f);return 20;}long n=ftell(f);rewind(f);
 if(n<0 || n>512L*1024*1024){fclose(f);return 20;}
 uint8_t* data=(uint8_t*)malloc((size_t)n+1);if(!data){fclose(f);return 20;}
 bool read=fread(data,1,(size_t)n,f)==(size_t)n;fclose(f);
 alpha::OriginalGameView before{};before.data_version=123;before.factions.data=data;
 alpha::OriginalGameView v;memcpy(&v,&before,sizeof(v));
 auto e=read?alpha::read_original_game(data,(size_t)n,v):alpha::GameReadError::truncated;
 if(e!=alpha::GameReadError::ok){bool intact=!memcmp(&v,&before,sizeof(v));printf("ERROR %s\n",alpha::game_read_error_name(e));free(data);return intact?20:21;}
 uint32_t vehicles=2166136261u,prototypes=vehicles,bases=vehicles;
 for(size_t i=0;i<v.vehicle_count;++i){alpha::OriginalVehicle r;if(alpha::original_vehicle(v,i,r)!=alpha::GameReadError::ok)return 21;word(vehicles,hash(r));}
 for(size_t i=0;i<512;++i){alpha::OriginalPrototype r;if(alpha::original_prototype(v,i,r)!=alpha::GameReadError::ok)return 21;word(prototypes,hash(r));}
 for(size_t i=0;i<v.base_count;++i){alpha::OriginalBase r;if(alpha::original_base(v,i,r)!=alpha::GameReadError::ok)return 21;word(bases,hash(r));}
 alpha::OriginalVehicle veh{},vb{};veh.x=-123;memcpy(&vb,&veh,sizeof(veh));
 alpha::OriginalBase base{},bb{};base.x=-123;memcpy(&bb,&base,sizeof(base));
 alpha::OriginalPrototype proto{},pb{};proto.chassis_id=123;memcpy(&pb,&proto,sizeof(proto));
 if(alpha::original_vehicle(v,v.vehicle_count,veh)!=alpha::GameReadError::bad_index || memcmp(&veh,&vb,sizeof(veh)) ||
    alpha::original_base(v,v.base_count,base)!=alpha::GameReadError::bad_index || memcmp(&base,&bb,sizeof(base)) ||
    alpha::original_prototype(v,512,proto)!=alpha::GameReadError::bad_index || memcmp(&proto,&pb,sizeof(proto)))return 21;
 uint32_t map=2166136261u;
 for(size_t i=0;i<v.map.info.tile_count;++i){alpha::OriginalMapTile t;if(alpha::original_map_tile(v.map,i,t)!=alpha::MapReadError::ok)return 21;
  const uint32_t values[]={t.climate,t.contour,t.val2,t.region,t.visibility,t.val3,t.unknown,(uint32_t)(int32_t)t.owner,t.items,t.landmarks};
  for(auto n:values)word(map,n);
  for(auto n:t.visible_items)word(map,n);
 }
 // Headerless embedded decoder must bound consumption and preserve both outputs on failure.
 alpha::OriginalMapView before_map{};before_map.info.width=123;alpha::OriginalMapView block;memcpy(&block,&before_map,sizeof(block));size_t consumed=123;
 if(alpha::read_original_map_block(nullptr,0,block,consumed)==alpha::MapReadError::ok || consumed!=123 || memcmp(&block,&before_map,sizeof(block)))return 21;
 if(alpha::read_original_map_block(v.map.metadata,alpha::original_map_metadata_size-1,block,consumed)==alpha::MapReadError::ok || consumed!=123 || memcmp(&block,&before_map,sizeof(block)))return 21;
 if(alpha::read_original_map_block(v.map.metadata,(size_t)(data+n-v.map.metadata),block,consumed)!=alpha::MapReadError::ok || consumed!=alpha::original_map_metadata_size+v.map.info.tile_count*alpha::original_map_tile_size+v.map.info.abstract_count)return 21;
 uint32_t spans=2166136261u;
 const alpha::OriginalByteSpan parts[]={v.factions,v.faction_definitions,v.rankings,v.faction_turn_might,v.replay,v.continents,v.bunglist,v.scenario,v.scenario_name,v.objectives,v.monuments,v.tectonic};
 for(const auto& p:parts){word(spans,(uint32_t)p.size);word(spans,digest(p.data,p.size));}
 uint32_t ui=2166136261u;for(auto x:v.map_cursor)word(ui,(uint32_t)x);
 for(const auto& w:v.views){const int32_t values[]={w.draw_b,w.draw_a,w.flags,w.zoom,w.x,w.y};for(auto x:values)word(ui,(uint32_t)x);}
 word(ui,(uint32_t)v.canvas_width);word(ui,(uint32_t)v.canvas_height);
 printf("version=%lu marker=%lu expanded=%u player=%ld turn=%ld year=%ld vehicles=%lu bases=%lu width=%lu height=%lu replay=%lu tick=%08lx prefs=%08lx vehicle_digest=%08lx prototype_digest=%08lx base_digest=%08lx spans=%08lx map=%08lx ui=%08lx rc=0\n",
 (unsigned long)v.data_version,(unsigned long)v.legacy_marker_version,(unsigned)v.expanded_preferences,(long)v.player,(long)v.preferences.CurrentTurn,(long)v.preferences.CurrentMissionYear,
 (unsigned long)v.vehicle_count,(unsigned long)v.base_count,(unsigned long)v.map.info.width,(unsigned long)v.map.info.height,(unsigned long)v.replay_size,(unsigned long)v.tick_seed,
 (unsigned long)hash(v.preferences),(unsigned long)vehicles,(unsigned long)prototypes,(unsigned long)bases,(unsigned long)spans,(unsigned long)map,(unsigned long)ui);
 free(data);return 0;
}
