/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_production_queue.h"
#include "technology_effects_ranges.h"
#include <string.h>
namespace alpha { namespace {
bool aliases(const OriginalWorld& w,const Rules& t,const UnitDefinitions& d,const FacilityDefinitions& f,const void* out,size_t n){
 using detail::effects_range_overlap;using detail::effects_array_overlap;
 if(effects_range_overlap(out,n,&w,sizeof w) || effects_range_overlap(out,n,&t,sizeof t) || effects_range_overlap(out,n,&d,sizeof d) || effects_range_overlap(out,n,&f,sizeof f))return true;
 if(!w.loaded())return false;
 const auto& g=w.game();
 return w.overlaps_byte_storage(out,n) ||
 effects_array_overlap(out,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile)) ||
 effects_array_overlap(out,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle)) ||
 effects_array_overlap(out,n,w.base(0),g.base_count,sizeof(OriginalBase)) ||
 effects_array_overlap(out,n,w.prototype(0),512,sizeof(OriginalPrototype)) ||
 effects_array_overlap(out,n,w.faction(0),8,sizeof(OriginalFactionState)) ||
 effects_array_overlap(out,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));
}}
LogicError world_production_queue(const OriginalWorld& w,const Rules& t,const UnitDefinitions& d,
 const FacilityDefinitions& f,Variant profile,bool expansion_enabled,size_t base_id,WorldProductionQueue& output){
 if(aliases(w,t,d,f,&output,sizeof output) || !w.loaded() || t.count!=89 ||
 (profile!=Variant::base && profile!=Variant::alien_crossfire) ||
 ((w.game().data_version>=11)!=(profile==Variant::alien_crossfire)) ||
 d.weapon_count!=(profile==Variant::base?21u:26u) || d.armor_count!=(profile==Variant::base?10u:14u) ||
 d.ability_count!=(profile==Variant::base?24u:29u) ||
 f.slot_count!=(profile==Variant::base?72u:134u) || f.first_repeatable!=(profile==Variant::base?34u:65u) ||
 f.first_project!=(profile==Variant::base?39u:70u) || f.project_count!=f.slot_count-f.first_project ||
 (profile==Variant::base && expansion_enabled))return LogicError::bad_state;
 const auto* b=w.base(base_id);if(!b || base_id>2147483647u || b->faction_id<1 || b->faction_id>7 || b->queue_size<0 || b->queue_size>9)return LogicError::bad_state;
 const auto* owner=w.faction((size_t)b->faction_id);if(!owner)return LogicError::bad_state;
 QueueShiftInput current{};current.queue_size=b->queue_size;memcpy(current.items,b->queue_items,sizeof current.items);memcpy(current.units_queue,owner->units_queue,sizeof current.units_queue);
 WorldProductionQueue next{};next.queue.queue_size=current.queue_size;memcpy(next.queue.items,current.items,sizeof current.items);memcpy(next.queue.units_queue,current.units_queue,sizeof current.units_queue);
 WorldFacilityOptions options{};bool facilities_ready=false;
 while(current.queue_size>0){
  if(queue_shift(current,next.queue)!=LogicError::ok)return LogicError::bad_state;
  ++next.iterations;const int32_t item=next.queue.items[0];bool available=false;
  if(item>=0){
   if(world_unit_availability(w,t,d,profile,expansion_enabled,b->faction_id,(int32_t)base_id,item,available)!=LogicError::ok)return LogicError::bad_state;
  }else{
   if(item<-(int32_t)f.slot_count+1)return LogicError::bad_state;
   if(!facilities_ready){if(world_facility_options(w,t,f,profile,expansion_enabled,b->faction_id,(int32_t)base_id,0,options)!=LogicError::ok)return LogicError::bad_state;facilities_ready=true;}
   available=options.available[(size_t)-item]!=0;
  }
  if(available){next.available=true;next.project_notification_required=item<=-(int32_t)f.first_project;break;}
  current.queue_size=next.queue.queue_size;memcpy(current.items,next.queue.items,sizeof current.items);memcpy(current.units_queue,next.queue.units_queue,sizeof current.units_queue);
 }
 output=next;return LogicError::ok;
}
}
