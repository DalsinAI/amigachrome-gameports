/* SPDX-License-Identifier: GPL-3.0-or-later.
 * battle_kill ordinary/native route: OpenSMACX veh.cpp, GPL-3.0-or-later,
 * pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c; original edition behavior.
 */
#include "world_casualty.h"
#include "native_combat_morale.h"
#include "technology_effects_ranges.h"
#include <string.h>
namespace alpha {namespace {
bool alias(const OriginalWorld&w,const void*p,size_t n){using detail::effects_range_overlap;using detail::effects_array_overlap;const auto&g=w.game();return effects_range_overlap(p,n,&w,sizeof w)||w.overlaps_byte_storage(p,n)||effects_array_overlap(p,n,g.bunglist.data,g.bunglist.size,1)||effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))||effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))||effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))||effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))||effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))||effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
void add(int32_t&v,uint32_t n){uint32_t bits;memcpy(&bits,&v,4);bits+=n;memcpy(&v,&bits,4);}
}
RemovalError CasualtyPlan::prepare(const OriginalWorld&w,size_t victim,const RemovalRuntime&r,const CasualtyInputs&i,size_t limit){
 const auto*v=w.vehicle(victim);if(!v||v->unit_id<0||v->unit_id>=512||v->faction_id>7||i.counterpart_actor< -1||i.faction_argument6< -1||i.faction_argument6>7)return RemovalError::bad_state;
 if(i.counterpart_actor>=0&&!w.vehicle((size_t)i.counterpart_actor))return RemovalError::bad_state;
 const auto*proto=w.prototype((size_t)v->unit_id);if(!proto)return RemovalError::bad_state;
 // Artifact capture precedes the source interlude gate. Its complete owned
 // storage/speed/event route is held; do not simulate it as ordinary deletion.
 if((uint8_t)proto->plan==12)return RemovalError::unsupported;
 if((v->state&0x800000u)&&i.counterpart_actor>=0&&!(w.game().preferences.GamePreferences&0x40000))return RemovalError::unsupported;
 uint32_t reward=0;
 if(v->faction_id==0){
  if(i.counterpart_actor<0)return RemovalError::bad_state;
  const auto*other=w.vehicle((size_t)i.counterpart_actor);if(other->faction_id>7)return RemovalError::bad_state;
  if(other->faction_id!=0&&i.native_reward.present){uint32_t rank;if(world_native_combat_morale(w,victim,0,rank)!=LogicError::ok)return RemovalError::bad_state;reward=(rank+1u)*10u;}
 }
 constexpr size_t extra=sizeof(CasualtyPlan)-sizeof(VehicleRemovalPlan);
 if(limit<extra)return RemovalError::out_of_memory;
 auto error=removal_.capture(w,limit-extra);if(error!=RemovalError::ok)return error;
 inputs_=i;
 // Match source ordering on the private frame. No published frame exists until
 // the checked dense removal also succeeds.
 auto&f=removal_.frame_.factions_[v->faction_id];
 f.units_lost[(size_t)v->unit_id]=(uint16_t)(f.units_lost[(size_t)v->unit_id]+1u);
 if((v->flags&0x20u)&&i.faction_argument6>=0)add(removal_.frame_.factions_[(size_t)i.faction_argument6].unk_101,1u);
 if(inputs_.native_reward.present)add(inputs_.native_reward.value,reward);
 if(inputs_.killed.present)add(inputs_.killed.value,1u);
 return removal_.apply(victim,r);
}
void CasualtyPlan::swap(CasualtyPlan&p){removal_.swap(p.removal_);const auto i=inputs_;inputs_=p.inputs_;p.inputs_=i;}
RemovalError stage_casualty(const OriginalWorld&w,size_t victim,const RemovalRuntime&r,const CasualtyInputs&i,CasualtyPlan&out,size_t limit){
 using detail::effects_range_overlap;
 if(!w.loaded()||victim>=w.game().vehicle_count||w.game().vehicle_count>2048||w.game().preferences.VehCount!=(int32_t)w.game().vehicle_count||alias(w,&r,sizeof r)||alias(w,&i,sizeof i)||alias(w,&out,sizeof out)||effects_range_overlap(&out,sizeof out,&r,sizeof r)||effects_range_overlap(&out,sizeof out,&i,sizeof i)||effects_range_overlap(&r,sizeof r,&i,sizeof i))return RemovalError::bad_state;
 if(out.frame()&&(alias(*out.frame(),&r,sizeof r)||alias(*out.frame(),&i,sizeof i)))return RemovalError::bad_state;
 if(r.observation!=RemovalObservation::supplied)return RemovalError::bad_state;
 if(r.multiplayer_active||r.logging_enabled||r.base_window_visible)return RemovalError::unsupported;
 CasualtyPlan next;auto error=next.prepare(w,victim,r,i,limit);if(error==RemovalError::ok)out.swap(next);return error;
}
}
