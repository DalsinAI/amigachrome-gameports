/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "vehicle_stack.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
using namespace alpha;
namespace {
constexpr size_t tiles=24,units=8;
OriginalMapTile actual_tiles[tiles],reference_tiles[tiles],before_tiles[tiles];
OriginalVehicle actual_units[units],reference_units[units],before_units[units];
size_t cases=0,rejections=0;uint32_t digest=2166136261u;
void require(bool b){if(!b){fprintf(stderr,"stack control failed at %lu/%lu\n",(unsigned long)cases,(unsigned long)rejections);exit(21);}}
size_t at(int x,int y){return ((size_t)y*8u+(size_t)x)/2u;}
VehicleStackStorage storage(){return {8,6,actual_tiles,tiles,actual_units,units};}
void mix(uint32_t n){for(int b=0;b<4;++b){digest=(digest^(n&255))*16777619u;n>>=8;}}
void setup(unsigned seed){
 memset(actual_tiles,0,sizeof(actual_tiles));memset(actual_units,0,sizeof(actual_units));
 for(size_t t=0;t<tiles;++t){actual_tiles[t].val2=(uint8_t)(0xa0u|((seed+t)%8));actual_tiles[t].items=(t%3==0?1u:0u)|0x400u;}
 int16_t heads[tiles];for(size_t t=0;t<tiles;++t)heads[t]=-1;
 // Deterministic shuffled insertion gives non-record-order stack roots/middles.
 for(size_t step=0;step<units;++step){size_t id=(step*3+seed)%units;auto& v=actual_units[id];
  size_t t=((id+seed)*(seed%5+1))%6;
  v.x=(int16_t)((t%4)*2);v.y=(int16_t)((t/4)*2);
  v.faction_id=(uint8_t)((id+seed)%8);v.unit_id=(int16_t)(320+id);v.state=0x12340000u+(uint32_t)id;
  v.flags=0x8051;v.moves_spent=(uint8_t)(id+1);v.order=7;v.home_base_id=-1;v.rotate_angle=(int8_t)id;
  size_t tile=at(v.x,v.y);v.prev_veh_id_stack=-1;v.next_veh_id_stack=heads[tile];
  if(heads[tile]>=0)actual_units[heads[tile]].prev_veh_id_stack=(int16_t)id;
  heads[tile]=(int16_t)id;actual_tiles[tile].items|=2;
 }
 memcpy(reference_units,actual_units,sizeof(actual_units));memcpy(reference_tiles,actual_tiles,sizeof(actual_tiles));
}
/* Separate source-flow reference for OpenSMACX veh_lift/veh_drop (GPL3,
 * Brendan Casey 2013-2021, pinned in vehicle_stack.h). The reference obtains
 * the destination via the original record scan + previous-link walk, rather
 * than the production validator's per-tile root table.
 */
void source_relocate(size_t id,int x,int y,int triad){
 auto& v=reference_units[id];const int prev=v.prev_veh_id_stack,next=v.next_veh_id_stack;
 if(prev>=0)reference_units[prev].next_veh_id_stack=(int16_t)next;
 if(next>=0)reference_units[next].prev_veh_id_stack=(int16_t)prev;
 else if(prev<0)reference_tiles[at(v.x,v.y)].items&=~2u;
 v.x=-1;v.y=-1;v.next_veh_id_stack=-1;v.prev_veh_id_stack=-1;
 int top=-1;
 if(reference_tiles[at(x,y)].items&2u)for(size_t i=0;i<units;++i)
  if(reference_units[i].x==x && reference_units[i].y==y){
   top=(int)i;while(reference_units[top].prev_veh_id_stack>=0)top=reference_units[top].prev_veh_id_stack;break;
  }
 v.next_veh_id_stack=(int16_t)top;v.prev_veh_id_stack=-1;v.x=(int16_t)x;v.y=(int16_t)y;
 if(top<0){if(!(reference_tiles[at(x,y)].items&1u))reference_tiles[at(x,y)].val2=(uint8_t)((reference_tiles[at(x,y)].val2&0xf0u)|(v.faction_id&0xfu));}
 else reference_units[top].prev_veh_id_stack=(int16_t)id;
 reference_tiles[at(x,y)].items|=(v.faction_id && triad!=2)?0x100002u:2u;
}
void rejection(VehicleStackStorage s,size_t id,int x,int y,int triad){
 memcpy(before_units,actual_units,sizeof(actual_units));memcpy(before_tiles,actual_tiles,sizeof(actual_tiles));
 require(relocate_vehicle_storage(s,id,x,y,triad)!=StackError::ok);
 require(!memcmp(before_units,actual_units,sizeof(actual_units)) && !memcmp(before_tiles,actual_tiles,sizeof(actual_tiles)));++rejections;
}
}
int main(){
 for(unsigned seed=0;seed<32;++seed)for(size_t id=0;id<units;++id)for(int triad=0;triad<3;++triad)for(size_t dest=0;dest<tiles;++dest){
  setup(seed);require(validate_vehicle_stacks(storage())==StackError::ok);
  const int y=(int)(dest/4),x=(int)(dest%4)*2+(y&1);
  source_relocate(id,x,y,triad);require(relocate_vehicle_storage(storage(),id,x,y,triad)==StackError::ok);
  require(!memcmp(actual_units,reference_units,sizeof(actual_units)) && !memcmp(actual_tiles,reference_tiles,sizeof(actual_tiles)));
  require(validate_vehicle_stacks(storage())==StackError::ok);
  // Stable field-level digest is endian independent; never hash native padding.
  for(const auto& v:actual_units){mix((uint32_t)(int32_t)v.x);mix((uint32_t)(int32_t)v.y);mix((uint32_t)(int32_t)v.prev_veh_id_stack);mix((uint32_t)(int32_t)v.next_veh_id_stack);}
  for(const auto& t:actual_tiles){mix(t.items);mix(t.val2);}++cases;
 }
 for(int kind=0;kind<20;++kind){setup(1);auto s=storage();
  switch(kind){
   case 0:s.width=7;break;case 1:s.tile_count--;break;case 2:s.vehicles=nullptr;break;
   case 3:actual_units[0].next_veh_id_stack=8;break;case 4:actual_units[0].prev_veh_id_stack=-2;break;
   case 5:actual_units[0].x=1;break;case 6:actual_units[0].faction_id=8;break;
   case 7:actual_units[0].prev_veh_id_stack=0;actual_units[0].next_veh_id_stack=0;break;
   case 8:actual_tiles[at(actual_units[0].x,actual_units[0].y)].items&=~2u;break;
   case 9:actual_tiles[23].items|=2u;break;
   case 10:rejection(s,8,0,0,0);continue;
   case 11:rejection(s,0,1,0,0);continue;
   case 12:rejection(s,0,0,6,0);continue;
   case 13:rejection(s,0,0,0,-1);continue;
   case 14:rejection(s,0,0,0,3);continue;
   case 15:actual_units[0].x=-1;actual_units[0].y=-1;break;
   case 16:actual_units[0].prev_veh_id_stack=1;actual_units[0].next_veh_id_stack=1;
    actual_units[1].prev_veh_id_stack=0;actual_units[1].next_veh_id_stack=0;break;
   case 17:actual_units[0].prev_veh_id_stack=-1;actual_units[1].prev_veh_id_stack=-1;
    actual_units[1].x=actual_units[0].x;actual_units[1].y=actual_units[0].y;break;
   case 18:actual_units[0].next_veh_id_stack=1;actual_units[1].prev_veh_id_stack=0;
    actual_units[1].x=6;actual_units[1].y=4;break;
   case 19:s.tiles=nullptr;break;
  }rejection(s,0,0,0,0);
 }
 OriginalMapTile empty[1]{};VehicleStackStorage no_units{2,1,empty,1,nullptr,0};
 require(validate_vehicle_stacks(no_units)==StackError::ok);require(relocate_vehicle_storage(no_units,0,0,0,0)==StackError::bad_reference);
 printf("source_cases=%lu rejections=%lu digest=%08lx rc=0\n",(unsigned long)cases,(unsigned long)rejections,(unsigned long)digest);return 0;
}
