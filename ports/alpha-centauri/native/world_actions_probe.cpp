/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_actions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
namespace {
uint8_t* read(const char* path,size_t& n){n=0;FILE* f=fopen(path,"rb");if(!f)return nullptr;
 if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long size=ftell(f);rewind(f);
 if(size<1 || size>64*1024*1024){fclose(f);return nullptr;}auto* p=(uint8_t*)malloc((size_t)size);
 if(p && fread(p,1,(size_t)size,f)==(size_t)size)n=(size_t)size;else {free(p);p=nullptr;}fclose(f);return p;
}
}
uint32_t digest(const OriginalWorld& w){
 uint32_t h=2166136261u;
 auto add=[&](const void* p,size_t n){const auto* b=(const uint8_t*)p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;};
 for(size_t i=0;i<w.game().map.info.tile_count;++i)add(w.tile(i),sizeof(OriginalMapTile));
 for(size_t i=0;i<w.game().vehicle_count;++i)add(w.vehicle(i),sizeof(OriginalVehicle));
 for(size_t i=0;i<w.game().base_count;++i)add(w.base(i),sizeof(OriginalBase));
 for(size_t i=0;i<512;++i)add(w.prototype(i),sizeof(OriginalPrototype));
 for(size_t i=0;i<8;++i){add(w.faction(i),sizeof(OriginalFactionState));add(w.faction_definition(i),sizeof(OriginalFactionDefinition));}
 return h;
}
int main(int argc,char** argv){
 if(argc!=3 && argc!=8)return 20;
 size_t n;uint8_t* text=read(argv[1],n);if(!text)return 20;
 auto* tech=(Rules*)calloc(1,sizeof(Rules));auto* defs=(UnitDefinitions*)calloc(1,sizeof(UnitDefinitions));
 if(!tech || !defs){free(text);free(tech);free(defs);return 20;}
 bool good=read_technologies(text,n,*tech)==Error::ok && read_unit_definitions(text,n,*tech,Variant::base,*defs)==Error::ok;
 free(text);if(!good){free(tech);free(defs);return 20;}
 size_t bytes_count;uint8_t* bytes=read(argv[2],bytes_count);if(!bytes){free(tech);free(defs);return 20;}
 OriginalWorld w;if(w.load(bytes,bytes_count,0xabcdef01)!=WorldLoadError::ok){free(bytes);free(tech);free(defs);return 20;}
 if(argc==8){
  const auto before=digest(w);const auto seed=w.runtime_seed();
  const auto r=WorldActions::clear_land_step(w,*defs,*tech,(size_t)strtoul(argv[3],nullptr,10),atoi(argv[4]),atoi(argv[5]),atoi(argv[6]),atoi(argv[7]));
  if((r.status!=StepStatus::moved && digest(w)!=before) || w.runtime_seed()!=seed || memcmp(w.original_bytes(),bytes,bytes_count))return 21;
  if(r.status==StepStatus::moved){
   VehicleStackStorage stack{(int32_t)w.game().map.info.width,(int32_t)w.game().map.info.height,const_cast<OriginalMapTile*>(w.tile(0)),w.game().map.info.tile_count,const_cast<OriginalVehicle*>(w.vehicle(0)),w.game().vehicle_count};
   if(validate_vehicle_stacks(stack)!=StackError::ok)return 21;
  }
  printf("status=%s price=%lu unchanged=%d seed=abcdef01 rc=0\n",step_status_name(r.status),(unsigned long)r.price,digest(w)==before);
  free(bytes);free(tech);free(defs);return 0;
 }
 const size_t count=w.game().vehicle_count;unsigned statuses[6]{};
 const int dx[]={1,2,1,0,-1,-2,-1,0},dy[]={-1,0,1,2,1,0,-1,-2};
 // Each attempt reloads the same baseline, so test order cannot consume moves.
 for(size_t id=0;id<count;++id)for(int direction=0;direction<8;++direction){
  if(w.load(bytes,bytes_count,0xabcdef01)!=WorldLoadError::ok)return 21;
  const OriginalVehicle before=*w.vehicle(id);const uint32_t seed=w.runtime_seed(),snapshot=digest(w);
  MapGeometry geometry{(int32_t)w.game().map.info.width,(int32_t)w.game().map.info.height,w.game().map.info.flat};int32_t x;
  if(movement_wrap_x(geometry,before.x+dx[direction],x)!=LogicError::ok)return 21;
  const auto result=WorldActions::clear_land_step(w,*defs,*tech,id,w.game().player,3,x,before.y+dy[direction]);
  ++statuses[(size_t)result.status];
  if(result.status==StepStatus::moved){
   const auto& after=*w.vehicle(id);
   if(after.x!=x || after.y!=before.y+dy[direction] || after.moves_spent!=before.moves_spent+result.price ||
      after.rotate_angle!=direction || !(after.state&4u) || w.runtime_seed()!=seed)return 21;
   printf("STEP id=%lu from=%d,%d to=%d,%d price=%lu spent=%u->%u\n",(unsigned long)id,before.x,before.y,after.x,after.y,(unsigned long)result.price,before.moves_spent,after.moves_spent);
  }else if(memcmp(&before,w.vehicle(id),sizeof(before)) || digest(w)!=snapshot || w.runtime_seed()!=seed)return 21;
  if(memcmp(w.original_bytes(),bytes,bytes_count))return 21;
 }
 printf("moved=%u exhausted=%u not_owner=%u unsupported=%u bad_state=%u out_of_memory=%u rc=0\n",statuses[0],statuses[1],statuses[2],statuses[3],statuses[4],statuses[5]);
 free(bytes);free(tech);free(defs);return 0;
}
