/* SPDX-License-Identifier: GPL-3.0-or-later.
 * Original battle_kill capture path; OpenSMACX veh.cpp GPL-3.0-or-later,
 * Brendan Casey, pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 */
#include "artifact_capture.h"
#include "world_unit_stats.h"
#include "movement.h"
#include "technology_effects_ranges.h"
#include <string.h>
namespace alpha {namespace {
bool alias(const OriginalWorld&w,const void*p,size_t n){using detail::effects_range_overlap;using detail::effects_array_overlap;const auto&g=w.game();return effects_range_overlap(p,n,&w,sizeof w)||w.overlaps_byte_storage(p,n)||effects_array_overlap(p,n,g.bunglist.data,g.bunglist.size,1)||effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))||effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))||effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))||effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))||effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))||effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
void increment(int32_t&v){uint32_t bits;memcpy(&bits,&v,4);++bits;memcpy(&v,&bits,4);}
}
RemovalError ArtifactCapturePlan::prepare(const OriginalWorld&w,size_t id,const RemovalRuntime&r,const UnitDefinitions&units,const Rules&tech,int32_t road,const CasualtyInputs&in,size_t limit){
 const auto*v=w.vehicle(id);if(!v||v->unit_id<0||v->unit_id>=512||v->faction_id>7||in.counterpart_actor<0||in.faction_argument6< -1||in.faction_argument6>7||road<1||road>100||tech.count!=89)return RemovalError::bad_state;
 const auto*other=w.vehicle((size_t)in.counterpart_actor);if(!other||other->unit_id<0||other->unit_id>=512||other->faction_id>7)return RemovalError::bad_state;
 if((size_t)in.counterpart_actor==id||other->faction_id==0)return RemovalError::unsupported;
 const auto*proto=w.prototype((size_t)v->unit_id);const auto*recipient=w.prototype((size_t)other->unit_id);
 if(!proto||!recipient||proto->chassis_id>=9||recipient->chassis_id>=9)return RemovalError::bad_state;
 const bool expansion=w.game().data_version>=11;
 if(units.weapon_count!=(expansion?26u:21u)||units.armor_count!=(expansion?14u:10u)||units.ability_count!=(expansion?29u:24u))return RemovalError::bad_state;
 if((uint8_t)proto->plan!=12||units.chassis[proto->chassis_id].triad!=0||units.chassis[recipient->chassis_id].triad!=0||units.chassis[recipient->chassis_id].missile!=0)return RemovalError::unsupported;
 const MapGeometry geometry{(int32_t)w.game().map.info.width,(int32_t)w.game().map.info.height,w.game().map.info.flat};size_t destination;
 if(movement_tile_index(geometry,other->x,other->y,destination)!=LogicError::ok)return RemovalError::bad_state;
 const auto*tile=w.tile(destination);if(!tile)return RemovalError::bad_state;
 if((tile->climate&0xe0u)<0x60u)return RemovalError::unsupported;
 constexpr size_t extra=sizeof(ArtifactCapturePlan)-sizeof(VehicleRemovalPlan);if(limit<extra)return RemovalError::out_of_memory;
 auto error=storage_.capture(w,limit-extra);if(error!=RemovalError::ok)return error;
 auto&frame=storage_.frame_;auto&actor=frame.vehicles_[id];storage_.removed_=actor;inputs_=in;
 const auto old_faction=actor.faction_id,new_faction=other->faction_id;const int32_t oldx=actor.x,oldy=actor.y,dx=other->x,dy=other->y;const auto uid=(size_t)actor.unit_id;
 // Loss first, then transfer, checked lift/drop and actual new-owner speed.
 // This entire early-return route preempts the interlude bit/preference gate.
 auto&lost=frame.factions_[old_faction].units_lost[uid];lost=(uint16_t)(lost+1u);
 actor.faction_id=new_faction;actor.damage_taken=0;actor.order=0;actor.state&=0xfcffbdffu;
 --frame.factions_[old_faction].units_active[uid];++frame.factions_[new_faction].units_active[uid];
 const auto&m=frame.game().map.info;VehicleStackStorage stacks{(int32_t)m.width,(int32_t)m.height,frame.tiles_,m.tile_count,frame.vehicles_,frame.game().vehicle_count};
 const auto moved=relocate_vehicle_storage(stacks,id,dx,dy,0);if(moved!=StackError::ok)return moved==StackError::out_of_memory?RemovalError::out_of_memory:RemovalError::bad_state;
 uint32_t speed;const auto se=world_vehicle_speed(frame,units,tech,id,road,false,speed);if(se!=LogicError::ok)return se==LogicError::overflow?RemovalError::unsupported:RemovalError::bad_state;
 if(speed>255)return RemovalError::unsupported;
 actor.moves_spent=(uint8_t)speed;
 if(inputs_.captured.present)increment(inputs_.captured.value);
 storage_.runtime_=r;storage_.runtime_.lift_vehicle=-1;storage_.runtime_.lift_x=oldx;storage_.runtime_.lift_y=oldy;
 preview_={id,(size_t)in.counterpart_actor,old_faction,new_faction,speed};storage_.ready_=true;return RemovalError::ok;
}
void ArtifactCapturePlan::swap(ArtifactCapturePlan&p){storage_.swap(p.storage_);const auto in=inputs_;inputs_=p.inputs_;p.inputs_=in;const auto preview=preview_;preview_=p.preview_;p.preview_=preview;}
RemovalError stage_artifact_capture(const OriginalWorld&w,size_t id,const RemovalRuntime&r,const UnitDefinitions&units,const Rules&tech,int32_t road,const CasualtyInputs&in,ArtifactCapturePlan&out,size_t limit){
 using detail::effects_range_overlap;
 const void*inputs[]={&r,&units,&tech,&in};const size_t sizes[]={sizeof r,sizeof units,sizeof tech,sizeof in};
 if(!w.loaded()||id>=w.game().vehicle_count||w.game().vehicle_count>2048||w.game().preferences.VehCount!=(int32_t)w.game().vehicle_count||alias(w,&out,sizeof out))return RemovalError::bad_state;
 for(unsigned j=0;j<4;++j){if(alias(w,inputs[j],sizes[j])||effects_range_overlap(&out,sizeof out,inputs[j],sizes[j])||(out.frame()&&alias(*out.frame(),inputs[j],sizes[j])))return RemovalError::bad_state;for(unsigned k=0;k<j;++k)if(effects_range_overlap(inputs[j],sizes[j],inputs[k],sizes[k]))return RemovalError::bad_state;}
 if(r.observation!=RemovalObservation::supplied)return RemovalError::bad_state;
 if(r.multiplayer_active||r.logging_enabled||r.base_window_visible)return RemovalError::unsupported;
 ArtifactCapturePlan next;const auto error=next.prepare(w,id,r,units,tech,road,in,limit);if(error==RemovalError::ok)out.swap(next);return error;
}
}
