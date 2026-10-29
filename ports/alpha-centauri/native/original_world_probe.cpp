/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "original_world.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t digest(const uint8_t* p,size_t n){uint32_t h=2166136261u;for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;return h;}
static bool same_decoded(const alpha::OriginalWorld& w){
 const auto& v=w.game();
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
 for(size_t i=0;i<8;++i){alpha::OriginalFactionState r;if(alpha::original_faction_state(v,i,r)!=alpha::GameReadError::ok)return false;const auto& s=*w.faction(i);
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
 for(size_t i=0;i<v.map.info.tile_count;++i){alpha::OriginalMapTile r;if(alpha::original_map_tile(v.map,i,r)!=alpha::MapReadError::ok)return false;const auto& s=*w.tile(i);
  if(s.climate!=r.climate || s.contour!=r.contour || s.val2!=r.val2 || s.region!=r.region || s.visibility!=r.visibility || s.val3!=r.val3 || s.unknown!=r.unknown || s.owner!=r.owner || s.items!=r.items || s.landmarks!=r.landmarks)return false;
  for(unsigned j=0;j<7;++j)if(s.visible_items[j]!=r.visible_items[j])return false;
 }
 return !w.vehicle(v.vehicle_count) && !w.base(v.base_count) && !w.prototype(512) && !w.faction(8) && !w.faction_definition(8) && !w.tile(v.map.info.tile_count);
}
int main(int argc,char** argv){
 if(argc<2 || argc>3)return 20;
 FILE* f=fopen(argv[1],"rb");if(!f)return 20;
 if(fseek(f,0,SEEK_END)){fclose(f);return 20;}long n=ftell(f);rewind(f);
 if(n<0 || n>64L*1024*1024){fclose(f);return 20;}
 auto* data=(uint8_t*)malloc((size_t)n+1);if(!data){fclose(f);return 20;}
 bool read=fread(data,1,(size_t)n,f)==(size_t)n;fclose(f);if(!read){free(data);return 20;}
 const uint32_t hash=digest(data,(size_t)n);alpha::OriginalWorld w;
 const size_t limit=argc==3?(size_t)strtoul(argv[2],nullptr,0):64u*1024u*1024u;
 auto error=w.load(data,(size_t)n,0x12345678u,limit);
 if(error!=alpha::WorldLoadError::ok){if(w.loaded() || w.original_size() || w.memory_bytes() || w.runtime_seed() || w.vehicle(0) || w.faction(0) || w.current_monuments().data || w.current_monuments().size)return 21;printf("ERROR %s\n",alpha::world_load_error_name(error));free(data);return 20;}
 if(!w.loaded() || w.original_size()!=(size_t)n || !w.memory_bytes() || w.memory_bytes()>limit || w.runtime_seed()!=0x12345678u || w.original_bytes()==data || memcmp(w.original_bytes(),data,(size_t)n) || !same_decoded(w))return 21;
 memset(data,0,(size_t)n);free(data);data=nullptr;
 // Caller memory is gone: all borrowed spans now point into the owned snapshot.
 if(digest(w.original_bytes(),w.original_size())!=hash || !same_decoded(w))return 21;
 const uint8_t* snapshot=w.original_bytes();const auto memory=w.memory_bytes();
 if(w.load(nullptr,0,999)!=alpha::WorldLoadError::bad_format || w.original_bytes()!=snapshot || w.memory_bytes()!=memory || w.runtime_seed()!=0x12345678u || !same_decoded(w))return 21;
 if(w.load(snapshot,w.original_size(),999,0)!=alpha::WorldLoadError::memory_limit || w.original_bytes()!=snapshot || w.memory_bytes()!=memory || w.runtime_seed()!=0x12345678u || !same_decoded(w))return 21;
 if(w.game().vehicle_count){
  auto* corrupt=(uint8_t*)malloc(w.original_size());if(!corrupt)return 21;memcpy(corrupt,snapshot,w.original_size());
  size_t off=(size_t)(w.game().vehicles.data-snapshot);corrupt[off]=0xff;corrupt[off+1]=0xff;
  error=w.load(corrupt,w.original_size(),999);free(corrupt);
  if(error!=alpha::WorldLoadError::bad_reference || w.original_bytes()!=snapshot || w.memory_bytes()!=memory || w.runtime_seed()!=0x12345678u || !same_decoded(w))return 21;
 }
 // The separate table participates in the declared World memory budget.
 if(w.load(snapshot,w.original_size(),999,memory-1)!=alpha::WorldLoadError::memory_limit ||
    w.original_bytes()!=snapshot || w.memory_bytes()!=memory || !same_decoded(w))return 21;
 // Reload from our own snapshot without freeing it before the replacement owns it.
 if(w.load(w.original_bytes(),w.original_size(),0xabcdef01u,memory)!=alpha::WorldLoadError::ok || w.runtime_seed()!=0xabcdef01u || digest(w.original_bytes(),w.original_size())!=hash || !same_decoded(w))return 21;
 printf("version=%lu vehicles=%lu bases=%lu tiles=%lu bytes=%lu snapshot=%08lx seed=%08lx ownership=1 transactional=1 monuments=%08lx rc=0\n",(unsigned long)w.game().data_version,(unsigned long)w.game().vehicle_count,(unsigned long)w.game().base_count,(unsigned long)w.game().map.info.tile_count,(unsigned long)w.original_size(),(unsigned long)hash,(unsigned long)w.runtime_seed(),(unsigned long)digest(w.current_monuments().data,w.current_monuments().size));
 w.clear();w.clear();if(w.loaded() || w.original_size() || w.memory_bytes() || w.runtime_seed() || w.faction(0) || w.current_monuments().data || w.current_monuments().size)return 21;
 return 0;
}
