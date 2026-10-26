/* SPDX-License-Identifier: GPL-3.0-or-later. Behavior recovered privately from
 * both owner originals; source veh_lift/veh_kill context also documented by
 * OpenSMACX veh.cpp, pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 * See vehicle_stack.h and the retained upstream licensing notices. */
#include "vehicle_removal.h"
#include "technology_effects_ranges.h"
#include <string.h>
namespace alpha {
RemovalError remove_vehicle_storage(VehicleRemovalStorage&s,size_t victim){
 auto&v=s.stacks;
 if(!s.runtime||!s.prototypes||s.prototype_count!=512||!s.factions||s.faction_count!=8||v.vehicle_count>2048||victim>=v.vehicle_count||!v.vehicles||!v.tiles||v.tile_count>size_t(4096)*2048)return RemovalError::bad_state;
 const void* ptr[]={&s,v.vehicles,v.tiles,s.prototypes,s.factions,s.runtime};
 const size_t bytes[]={sizeof s,v.vehicle_count*sizeof(OriginalVehicle),v.tile_count*sizeof(OriginalMapTile),512*sizeof(OriginalPrototype),8*sizeof(OriginalFactionState),sizeof(RemovalRuntime)};
 for(unsigned i=0;i<6;++i)for(unsigned j=i+1;j<6;++j)if(detail::effects_range_overlap(ptr[i],bytes[i],ptr[j],bytes[j]))return RemovalError::bad_state;
 auto&r=*s.runtime;
 if(r.observation!=RemovalObservation::supplied)return RemovalError::bad_state;
 if(r.multiplayer_active||r.logging_enabled||r.base_window_visible)return RemovalError::unsupported;
 const auto&a=v.vehicles[victim];
 if(a.unit_id<0||a.unit_id>=512||a.faction_id>7)return RemovalError::bad_state;
 const auto check=validate_vehicle_stacks(v);
 if(check!=StackError::ok)return check==StackError::out_of_memory?RemovalError::out_of_memory:RemovalError::bad_state;
 // No failing operation below this point.
 auto&f=s.factions[a.faction_id];const auto&p=s.prototypes[(size_t)a.unit_id];
 if((uint8_t)p.plan<8){uint32_t bits;memcpy(&bits,&f.total_combat_units,4);--bits;memcpy(&f.total_combat_units,&bits,4);}
 --f.units_active[(size_t)a.unit_id];
 const int16_t prev=a.prev_veh_id_stack,next=a.next_veh_id_stack;
 r.lift_vehicle=(int32_t)victim;r.lift_x=a.x;r.lift_y=a.y;
 if(prev>=0)v.vehicles[prev].next_veh_id_stack=next;
 if(next>=0)v.vehicles[next].prev_veh_id_stack=prev;
 else if(prev<0)v.tiles[((size_t)a.y*(size_t)v.width+(size_t)a.x)/2].items&=~uint32_t(2);
 auto&removed=v.vehicles[victim];removed.x=removed.y=removed.prev_veh_id_stack=removed.next_veh_id_stack=-1;
 for(size_t i=victim;i+1<v.vehicle_count;++i)v.vehicles[i]=v.vehicles[i+1];
 --v.vehicle_count;
 for(size_t i=0;i<v.vehicle_count;++i){auto&b=v.vehicles[i];
  if(b.prev_veh_id_stack>(int32_t)victim)--b.prev_veh_id_stack;
  if(b.next_veh_id_stack>(int32_t)victim)--b.next_veh_id_stack;
  if(b.order==1){if(b.waypoint_x[0]==(int32_t)victim)b.order=0;else if(b.waypoint_x[0]>(int32_t)victim)--b.waypoint_x[0];}
 }
 for(auto&x:r.cursor){if(x==(int32_t)victim)x=-1;else if(x>(int32_t)victim)--x;}
 if(r.iteration_cursor&&r.iteration_cursor>=(int32_t)victim)--r.iteration_cursor;
 return RemovalError::ok;
}
}
