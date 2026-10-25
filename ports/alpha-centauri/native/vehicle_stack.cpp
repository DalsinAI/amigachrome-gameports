/* SPDX-License-Identifier: GPL-3.0-or-later. See vehicle_stack.h for attribution. */
#include "vehicle_stack.h"
#include <stdlib.h>
namespace alpha {
namespace {
constexpr uint32_t base_bit=1,vehicle_bit=2,supply_bit=0x100000;
struct Heads {
 int32_t* data=nullptr;
 ~Heads(){free(data);}
};
bool index(const VehicleStackStorage& s,int32_t x,int32_t y,size_t& out) {
 if(x<0 || y<0 || x>=s.width || y>=s.height || ((x+y)&1))return false;
 out=((size_t)y*(size_t)s.width+(size_t)x)/2;return true;
}
StackError inspect(const VehicleStackStorage& s,Heads& heads) {
 if(s.width<2 || s.width>4096 || (s.width&1) || s.height<1 || s.height>4096 ||
    s.tile_count!=(size_t)(s.width/2)*(size_t)s.height || !s.tiles ||
    s.vehicle_count>32767 || (s.vehicle_count && !s.vehicles))return StackError::bad_geometry;
 heads.data=(int32_t*)malloc(s.tile_count*sizeof(int32_t));
 if(!heads.data)return StackError::out_of_memory;
 for(size_t t=0;t<s.tile_count;++t)heads.data[t]=-1;
 for(size_t i=0;i<s.vehicle_count;++i){
  const auto& v=s.vehicles[i];size_t tile;
  if(!index(s,v.x,v.y,tile) || v.faction_id>7 || v.prev_veh_id_stack < -1 || v.next_veh_id_stack < -1 ||
     v.prev_veh_id_stack>=(int32_t)s.vehicle_count || v.next_veh_id_stack>=(int32_t)s.vehicle_count)
   return StackError::bad_reference;
  if(v.prev_veh_id_stack<0){
   if(heads.data[tile]>=0)return StackError::bad_topology;
   heads.data[tile]=(int32_t)i;
  }
 }
 // Every node must lead backwards to its coordinate's unique root. A step
 // bound rejects cycles without trusting serialized links or extra pointers.
 for(size_t i=0;i<s.vehicle_count;++i){
  const auto& v=s.vehicles[i];size_t tile;
  if(!index(s,v.x,v.y,tile))return StackError::bad_reference;
  int32_t at=(int32_t)i;size_t steps=0;
  while(s.vehicles[at].prev_veh_id_stack>=0){
   if(++steps>=s.vehicle_count)return StackError::bad_topology;
   const int32_t prev=s.vehicles[at].prev_veh_id_stack;
   if(s.vehicles[prev].next_veh_id_stack!=at || s.vehicles[prev].x!=v.x || s.vehicles[prev].y!=v.y)
    return StackError::bad_topology;
   at=prev;
  }
  if(heads.data[tile]!=at)return StackError::bad_topology;
  const int32_t next=v.next_veh_id_stack;
  if(next>=0 && (s.vehicles[next].prev_veh_id_stack!=(int32_t)i ||
                s.vehicles[next].x!=v.x || s.vehicles[next].y!=v.y))return StackError::bad_topology;
 }
 for(size_t t=0;t<s.tile_count;++t)
  if(((s.tiles[t].items&vehicle_bit)!=0)!=(heads.data[t]>=0))return StackError::bad_tile_bits;
 return StackError::ok;
}
}
StackError validate_vehicle_stacks(const VehicleStackStorage& s){Heads heads;return inspect(s,heads);}
StackError relocate_vehicle_storage(const VehicleStackStorage& s,size_t id,int32_t x,int32_t y,int32_t triad){
 Heads heads;const auto error=inspect(s,heads);if(error!=StackError::ok)return error;
 size_t dest;if(id>=s.vehicle_count || triad<0 || triad>2 || !index(s,x,y,dest))return StackError::bad_reference;
 auto& v=s.vehicles[id];size_t source;
 if(!index(s,v.x,v.y,source))return StackError::bad_reference;
 const int16_t prev=v.prev_veh_id_stack,next=v.next_veh_id_stack;
 if(prev>=0)s.vehicles[prev].next_veh_id_stack=next;
 else heads.data[source]=next;
 if(next>=0)s.vehicles[next].prev_veh_id_stack=prev;
 else if(prev<0)s.tiles[source].items&=~vehicle_bit;
 const int32_t top=heads.data[dest];
 v.prev_veh_id_stack=-1;v.next_veh_id_stack=(int16_t)top;v.x=(int16_t)x;v.y=(int16_t)y;
 if(top>=0)s.vehicles[top].prev_veh_id_stack=(int16_t)id;
 else if(!(s.tiles[dest].items&base_bit))
  s.tiles[dest].val2=(uint8_t)((s.tiles[dest].val2&0xf0u)|v.faction_id);
 s.tiles[dest].items|=vehicle_bit;
 if(v.faction_id && triad!=2)s.tiles[dest].items|=supply_bit;
 return StackError::ok;
}
const char* stack_error_name(StackError e){switch(e){
 case StackError::ok:return "ok";case StackError::bad_geometry:return "bad_geometry";
 case StackError::bad_reference:return "bad_reference";case StackError::bad_topology:return "bad_topology";
 case StackError::bad_tile_bits:return "bad_tile_bits";case StackError::out_of_memory:return "out_of_memory";
 }return "unknown";}
}
