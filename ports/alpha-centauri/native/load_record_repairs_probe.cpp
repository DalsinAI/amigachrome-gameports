/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "original_world.h"
#include "rules_text.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t digest(const uint8_t* p,size_t n){uint32_t h=2166136261u;for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;return h;}
static bool same_decoded(const alpha::OriginalWorld& w){
 alpha::OriginalGameView baseline;if(alpha::read_original_game(w.original_bytes(),w.original_size(),baseline)!=alpha::GameReadError::ok)return false;const auto& v=baseline;
 auto expected_preferences=v.preferences;expected_preferences.FactionStatus[1]=w.game().preferences.FactionStatus[1];
 const auto& actual_preferences=w.game().preferences;
#define WIRE_SCALAR(type,name,off) if(expected_preferences.name!=actual_preferences.name)return false;
#define WIRE_ARRAY(type,name,count,off) for(unsigned j=0;j<count;++j)if(expected_preferences.name[j]!=actual_preferences.name[j])return false;
#define WIRE_MATRIX(type,name,rows,cols,off) for(unsigned j=0;j<rows;++j)for(unsigned k=0;k<cols;++k)if(expected_preferences.name[j][k]!=actual_preferences.name[j][k])return false;
#define WIRE_STRING(name,count,off) if(memcmp(expected_preferences.name,actual_preferences.name,count+1))return false;
#include "original_prefs_fields.inc"
#undef WIRE_STRING
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_MATRIX
 const auto current=w.current_monuments();
 if(!current.data || current.size!=0x27a0 || current.size!=v.monuments.size ||
    memcmp(current.data,v.monuments.data,current.size))return false;
 const uintptr_t at=(uintptr_t)current.data,raw=(uintptr_t)w.original_bytes();
 if(at<raw+w.original_size() && raw<at+current.size)return false;
 for(size_t i=0;i<v.vehicle_count;++i){alpha::OriginalVehicle r;if(alpha::original_vehicle(v,i,r)!=alpha::GameReadError::ok)return false;const auto& s=*w.vehicle(i);
#define WIRE_SCALAR(type,name,off) if(s.name!=r.name)return false;
#define WIRE_ARRAY(type,name,count,off) for(unsigned j=0;j<count;++j)if(s.name[j]!=r.name[j])return false;
#define WIRE_STRING(name,count,off) if(memcmp(s.name,r.name,count+1))return false;
#include "original_vehicle_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
 }
 for(size_t i=0;i<v.base_count;++i){alpha::OriginalBase r;if(alpha::original_base(v,i,r)!=alpha::GameReadError::ok)return false;const auto& s=*w.base(i);
#define WIRE_SCALAR(type,name,off) if(s.name!=r.name)return false;
#define WIRE_ARRAY(type,name,count,off) for(unsigned j=0;j<count;++j)if(s.name[j]!=r.name[j])return false;
#define WIRE_STRING(name,count,off) if(memcmp(s.name,r.name,count+1))return false;
#include "original_base_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
 }
 for(size_t i=0;i<512;++i){alpha::OriginalPrototype r;if(alpha::original_prototype(v,i,r)!=alpha::GameReadError::ok)return false;const auto& s=*w.prototype(i);
#define WIRE_SCALAR(type,name,off) if(s.name!=r.name)return false;
#define WIRE_ARRAY(type,name,count,off) for(unsigned j=0;j<count;++j)if(s.name[j]!=r.name[j])return false;
#define WIRE_STRING(name,count,off) if(memcmp(s.name,r.name,count+1))return false;
#include "original_prototype_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
 }
 for(size_t i=0;i<8;++i){alpha::OriginalFactionState r;if(alpha::original_faction_state(v,i,r)!=alpha::GameReadError::ok)return false;const auto& s=*w.faction(i);r.player_flags=s.player_flags;
#define WIRE_SCALAR(type,name,off) if(s.name!=r.name)return false;
#define WIRE_ARRAY(type,name,count,off) for(unsigned j=0;j<count;++j)if(s.name[j]!=r.name[j])return false;
#define WIRE_STRING(name,count,off) if(memcmp(s.name,r.name,count+1))return false;
#define WIRE_MATRIX(type,name,rows,cols,off) for(unsigned j=0;j<rows;++j)for(unsigned k=0;k<cols;++k)if(s.name[j][k]!=r.name[j][k])return false;
#define WIRE_TEXT_MATRIX(name,rows,cols,off) for(unsigned j=0;j<rows;++j)if(memcmp(s.name[j],r.name[j],cols+1))return false;
#define WIRE_GOALS(name,count,off) for(unsigned j=0;j<count;++j){const auto& a=s.name[j];const auto& b=r.name[j];if(a.type!=b.type || a.priority!=b.priority || a.x!=b.x || a.y!=b.y || a.base_id!=b.base_id)return false;}
#include "original_faction_state_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
#undef WIRE_MATRIX
#undef WIRE_TEXT_MATRIX
#undef WIRE_GOALS
 }
 for(size_t i=0;i<8;++i){alpha::OriginalFactionDefinition r;if(alpha::original_faction_definition(v,i,r)!=alpha::GameReadError::ok)return false;const auto& s=*w.faction_definition(i);
#define WIRE_SCALAR(type,name,off) if(s.name!=r.name)return false;
#define WIRE_ARRAY(type,name,count,off) for(unsigned j=0;j<count;++j)if(s.name[j]!=r.name[j])return false;
#define WIRE_STRING(name,count,off) if(memcmp(s.name,r.name,count+1))return false;
#define WIRE_MATRIX(type,name,rows,cols,off) for(unsigned j=0;j<rows;++j)for(unsigned k=0;k<cols;++k)if(s.name[j][k]!=r.name[j][k])return false;
#define WIRE_TEXT_MATRIX(name,rows,cols,off) for(unsigned j=0;j<rows;++j)if(memcmp(s.name[j],r.name[j],cols+1))return false;
#define WIRE_GOALS(name,count,off) for(unsigned j=0;j<count;++j){const auto& a=s.name[j];const auto& b=r.name[j];if(a.type!=b.type || a.priority!=b.priority || a.x!=b.x || a.y!=b.y || a.base_id!=b.base_id)return false;}
#include "original_faction_definition_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
#undef WIRE_MATRIX
#undef WIRE_TEXT_MATRIX
#undef WIRE_GOALS
 }
 for(size_t i=0;i<v.map.info.tile_count;++i){alpha::OriginalMapTile r;if(alpha::original_map_tile(v.map,i,r)!=alpha::MapReadError::ok)return false;const auto& s=*w.tile(i);r.val2=s.val2;r.items=s.items;
  if(s.climate!=r.climate || s.contour!=r.contour || s.val2!=r.val2 || s.region!=r.region || s.visibility!=r.visibility || s.val3!=r.val3 || s.unknown!=r.unknown || s.owner!=r.owner || s.items!=r.items || s.landmarks!=r.landmarks)return false;
  for(unsigned j=0;j<7;++j)if(s.visible_items[j]!=r.visible_items[j])return false;
 }
 return !w.vehicle(v.vehicle_count) && !w.base(v.base_count) && !w.prototype(512) && !w.faction(8) && !w.faction_definition(8) && !w.tile(v.map.info.tile_count);
}
static void put(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;++i){p[i]=(uint8_t)n;n>>=8;}}
static uint8_t* read(const char* path,size_t& n){FILE*f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long len=ftell(f);if(len<=0){fclose(f);return nullptr;}rewind(f);auto*p=(uint8_t*)malloc((size_t)len);if(!p){fclose(f);return nullptr;}n=(size_t)len;bool ok=fread(p,1,n,f)==n;fclose(f);if(!ok){free(p);return nullptr;}return p;}
int main(int argc,char**argv){
 if(argc!=5)return 20;
 size_t n=0,tn=0;auto*b=read(argv[1],n);auto*t=read(argv[2],tn);if(!b||!t)return 20;
 auto*rules=new alpha::Rules;auto*original_rules=new alpha::Rules;if(alpha::read_technologies(t,tn,*rules)!=alpha::Error::ok)return 20;*original_rules=*rules;
 const unsigned mode=(unsigned)strtoul(argv[4],nullptr,0);alpha::OriginalWorld w;
 if(w.load(b,n,0xabcdef01)!=alpha::WorldLoadError::ok)return 20;
 const uint8_t* prior=w.original_bytes();const size_t memory=w.memory_bytes();const uint32_t hash=digest(b,n);
 if(mode==1)rules->count=88;
 if(mode==2)rules->technologies[88].prerequisites[1]=89;
 if(mode==3)rules->technologies[0].prerequisites[0]=-3;
 const auto e=w.load_with_record_repairs(mode==6?nullptr:prior,mode==7?n-1:n,0xabcdef01,*rules,mode==4?memory-1:memory);
 if((mode>=1 && mode<=4) || mode==6 || mode==7){
  if(e==alpha::WorldLoadError::ok || prior!=w.original_bytes() || !same_decoded(w) || w.runtime_seed()!=0xabcdef01 || digest(w.original_bytes(),n)!=hash)return 21;
  printf("rejected unchanged=1 rc=0\n");delete rules;delete original_rules;free(b);free(t);return 0;
 }
 if(e!=alpha::WorldLoadError::ok || !same_decoded(w) || w.original_size()!=n || w.memory_bytes()!=memory || w.runtime_seed()!=0xabcdef01 ||
    memcmp(w.original_bytes(),b,n) || memcmp(rules,original_rules,sizeof *rules))return 21;
 // This probe-only projection is independent explicit wire output for tests,
 // not an engine save encoder, filesystem protocol or original-game save claim.
 auto*projected=(uint8_t*)malloc(n);if(!projected)return 20;memcpy(projected,b,n);const auto&g=w.game();
 const size_t tiles=(size_t)(g.map.tiles-w.original_bytes()),factions=(size_t)(g.factions.data-w.original_bytes()),stride=g.data_version<11?0x20bc:0x20cc;
 for(size_t i=0;i<g.map.info.tile_count;++i){projected[tiles+i*44+2]=w.tile(i)->val2;put(projected+tiles+i*44+8,w.tile(i)->items);}
 for(size_t i=0;i<8;++i)put(projected+factions+i*stride,w.faction(i)->player_flags);
 FILE*f=fopen(argv[3],"wb");if(!f)return 20;
 bool ok=fwrite(&g.preferences.FactionStatus[1],1,1,f)==1 && fwrite(projected,1,n,f)==n;fclose(f);
 printf("repaired bytes=%lu alive=%u input=%08lx rc=0\n",(unsigned long)n,(unsigned)g.preferences.FactionStatus[1],(unsigned long)hash);
 free(projected);delete rules;delete original_rules;free(b);free(t);return ok?0:21;
}
