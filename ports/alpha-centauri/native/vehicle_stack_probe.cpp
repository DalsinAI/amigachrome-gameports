/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "vehicle_stack.h"
#include "original_world.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
int main(int argc,char** argv){
 if(argc!=2)return 20;
 FILE* f=fopen(argv[1],"rb");if(!f)return 20;
 if(fseek(f,0,SEEK_END)){fclose(f);return 20;}long length=ftell(f);
 if(length<=0 || length>64*1024*1024 || fseek(f,0,SEEK_SET)){fclose(f);return 20;}
 auto* bytes=(uint8_t*)malloc((size_t)length);if(!bytes){fclose(f);return 20;}
 if(fread(bytes,1,(size_t)length,f)!=(size_t)length){free(bytes);fclose(f);return 20;}fclose(f);
 OriginalWorld world;auto load=world.load(bytes,(size_t)length,0xabcdef01);free(bytes);
 if(load!=WorldLoadError::ok){printf("ERROR load=%s\n",world_load_error_name(load));return 20;}
 const auto& info=world.game().map.info;const size_t count=world.game().vehicle_count;
 auto* tiles=(OriginalMapTile*)malloc(info.tile_count*sizeof(OriginalMapTile));
 auto* vehicles=(OriginalVehicle*)malloc((count?count:1)*sizeof(OriginalVehicle));
 if(!tiles || !vehicles){free(tiles);free(vehicles);return 20;}
 VehicleStackStorage storage{(int32_t)info.width,(int32_t)info.height,tiles,info.tile_count,vehicles,count};
 auto reset=[&](){for(size_t t=0;t<info.tile_count;++t)tiles[t]=*world.tile(t);for(size_t v=0;v<count;++v)vehicles[v]=*world.vehicle(v);};
 reset();auto error=validate_vehicle_stacks(storage);
 if(error!=StackError::ok){printf("ERROR stack=%s\n",stack_error_name(error));free(tiles);free(vehicles);return 20;}
 size_t moves=0;
 // Exercise original records at every other vehicle's square and at the map
 // origin. This validates storage effects only, never legal player moves.
 for(size_t id=0;id<count;++id)for(size_t target=0;target<=count;++target)for(int triad=0;triad<3;++triad){
  reset();const int x=target<count?world.vehicle(target)->x:0,y=target<count?world.vehicle(target)->y:0;
  if(relocate_vehicle_storage(storage,id,x,y,triad)!=StackError::ok || validate_vehicle_stacks(storage)!=StackError::ok){free(tiles);free(vehicles);return 21;}
  for(size_t v=0;v<count;++v){
   OriginalVehicle changed=vehicles[v];const auto& original=*world.vehicle(v);
   changed.x=original.x;changed.y=original.y;changed.next_veh_id_stack=original.next_veh_id_stack;changed.prev_veh_id_stack=original.prev_veh_id_stack;
   if(memcmp(&changed,&original,sizeof(changed))){free(tiles);free(vehicles);return 21;}
  }++moves;
 }
 printf("vehicles=%lu tiles=%lu relocation_controls=%lu original_unchanged=1 rc=0\n",(unsigned long)count,(unsigned long)info.tile_count,(unsigned long)moves);
 free(tiles);free(vehicles);return 0;
}
