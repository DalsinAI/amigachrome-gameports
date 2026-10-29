/* SPDX-License-Identifier: GPL-3.0-or-later. See land_engagement.h.
 * Current-frame cloning follows the owned discovery frame pattern. */
#include "land_engagement.h"
#include "vehicle_stack.h"
#include "world_unit_stats.h"
#include "technology_effects_ranges.h"
#include <stdlib.h>
#include <string.h>
#include <new>
#include <type_traits>
namespace alpha {namespace {
template<class T>T* copy(const T* from,size_t n){static_assert(std::is_trivially_copyable<T>::value,"owned record copy");if(!n||!from||n>SIZE_MAX/sizeof(T))return nullptr;auto*p=(T*)malloc(n*sizeof(T));if(p){for(size_t i=0;i<n;++i)new(p+i)T;memcpy(p,from,n*sizeof(T));}return p;}
bool alias(const OriginalWorld&w,const void*p,size_t n){using detail::effects_range_overlap;using detail::effects_array_overlap;const auto&g=w.game();return effects_range_overlap(p,n,&w,sizeof w)||w.overlaps_byte_storage(p,n)||effects_array_overlap(p,n,g.bunglist.data,g.bunglist.size,1)||effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))||effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))||effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))||effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))||effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))||effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
EngagementError mapped(LogicError e){return e==LogicError::overflow?EngagementError::overflow:EngagementError::bad_state;}
}
EngagementError LandEngagementPlan::capture(const OriginalWorld&w,size_t limit){
 if(!w.loaded()||!w.game().bunglist.data||w.game().bunglist.size!=4096)return EngagementError::bad_state;
 const auto&g=w.game();const uint64_t arrays=(uint64_t)g.map.info.tile_count*sizeof(OriginalMapTile)+(uint64_t)g.vehicle_count*sizeof(OriginalVehicle)+(uint64_t)g.base_count*sizeof(OriginalBase)+512u*(uint64_t)sizeof(OriginalPrototype)+8u*(uint64_t)(sizeof(OriginalFactionState)+sizeof(OriginalFactionDefinition));const uint64_t required=sizeof(*this)+(uint64_t)w.length_+0x27a0u+arrays;if(required>limit||required>SIZE_MAX)return EngagementError::out_of_memory;
 frame_.length_=w.length_;frame_.memory_=(size_t)required-sizeof(*this)+sizeof(OriginalWorld);frame_.random_seed_=w.random_seed_;frame_.view_=g;
 frame_.raw_=copy(w.raw_,w.length_);frame_.monuments_=copy(w.monuments_,size_t(0x27a0));frame_.tiles_=copy(w.tiles_,g.map.info.tile_count);frame_.vehicles_=copy(w.vehicles_,g.vehicle_count);frame_.bases_=copy(w.bases_,g.base_count);frame_.prototypes_=copy(w.prototypes_,512);frame_.factions_=copy(w.factions_,8);frame_.definitions_=copy(w.definitions_,8);
 if(!frame_.raw_||!frame_.monuments_||!frame_.tiles_||!frame_.prototypes_||!frame_.factions_||!frame_.definitions_||(g.vehicle_count&&!frame_.vehicles_)||(g.base_count&&!frame_.bases_))return EngagementError::out_of_memory;
 auto relocate=[&](const uint8_t*source,size_t size,const uint8_t*&dest){if(!source&&!size){dest=nullptr;return true;}const uintptr_t x=(uintptr_t)source,start=(uintptr_t)w.raw_;if(x<start||x-start>w.length_||size>w.length_-(size_t)(x-start))return false;dest=frame_.raw_+(size_t)(x-start);return true;};auto span=[&](OriginalByteSpan&s){return relocate(s.data,s.size,s.data);};
 if(!relocate(g.map.metadata,original_map_metadata_size,frame_.view_.map.metadata)||!relocate(g.map.tiles,(size_t)g.map.info.tile_count*original_map_tile_size,frame_.view_.map.tiles)||!relocate(g.map.abstract,g.map.info.abstract_count,frame_.view_.map.abstract))return EngagementError::bad_state;
#define REBASE(name) if(!span(frame_.view_.name))return EngagementError::bad_state;
 REBASE(factions) REBASE(faction_definitions) REBASE(rankings) REBASE(faction_turn_might) REBASE(replay) REBASE(continents) REBASE(prototypes) REBASE(vehicles) REBASE(bases) REBASE(scenario) REBASE(scenario_name) REBASE(objectives) REBASE(monuments) REBASE(tectonic)
#undef REBASE
 memcpy(history_,g.bunglist.data,4096);frame_.view_.bunglist={history_,4096};return EngagementError::ok;
}
void LandEngagementPlan::swap(LandEngagementPlan&p){frame_.swap(p.frame_);for(unsigned i=0;i<4096;++i){auto x=history_[i];history_[i]=p.history_[i];p.history_[i]=x;}auto s=summary_;summary_=p.summary_;p.summary_=s;const bool b=ready_;ready_=p.ready_;p.ready_=b;if(frame_.loaded())frame_.view_.bunglist={history_,4096};if(p.frame_.loaded())p.frame_.view_.bunglist={p.history_,4096};}
EngagementError stage_land_engagement(const OriginalWorld&w,const UnitDefinitions&d,const Rules&r,const BasicRules&basic,size_t attacker,size_t defender,const EngagementFacts&facts,LandEngagementPlan&out,size_t limit){
 using detail::effects_range_overlap;
 const void*inputs[]={&d,&r,&basic,&facts};const size_t sizes[]={sizeof d,sizeof r,sizeof basic,sizeof facts};
 if(!w.loaded()||attacker==defender||alias(w,&out,sizeof out))return EngagementError::bad_state;
 for(unsigned i=0;i<4;++i){if(alias(w,inputs[i],sizes[i])||effects_range_overlap(&out,sizeof out,inputs[i],sizes[i]))return EngagementError::bad_state;for(unsigned j=0;j<i;++j)if(effects_range_overlap(inputs[i],sizes[i],inputs[j],sizes[j]))return EngagementError::bad_state;}
 for(unsigned i=0;i<4;++i)if(alias(out.frame_,inputs[i],sizes[i]))return EngagementError::bad_state;
 if(facts.observation!=EngagementObservation::supplied||facts.incoming_direction>7)return EngagementError::bad_state;
 const auto*a=w.vehicle(attacker);const auto*b=w.vehicle(defender);if(!a||!b||a->unit_id<0||a->unit_id>=512||b->unit_id<0||b->unit_id>=512)return EngagementError::bad_state;
 const auto*ap=w.prototype((size_t)a->unit_id);const auto*bp=w.prototype((size_t)b->unit_id);
 // Real component reactors are1..4. Wider wire values remain supported by the
 // single-round leaf, but byte wrap can prevent finite engagement termination.
 if(!ap||!bp||ap->reactor_id<1||ap->reactor_id>4||bp->reactor_id<1||bp->reactor_id>4)return EngagementError::unsupported;
 if(ap->plan<0||ap->plan>3||bp->plan<0||(bp->plan>3&&bp->plan!=12))return EngagementError::unsupported;
 LandEngagementPlan next;auto&e=next.summary_;auto err=world_land_combat_strengths(w,d,r,basic,attacker,defender,e.strengths);if(err!=LogicError::ok)return mapped(err);
 RetreatInitResult init{};err=world_land_retreat_initialization(w,d,r,basic,attacker,defender,0,init);if(err!=LogicError::ok)return mapped(err);
 auto ce=next.capture(w,limit);if(ce!=EngagementError::ok)return ce;
 auto*av=const_cast<OriginalVehicle*>(next.frame_.vehicle(attacker));auto*bv=const_cast<OriginalVehicle*>(next.frame_.vehicle(defender));e.initial_counter=e.remaining_counter=init.counter;e.next_crt_state=facts.crt_state;e.retreat_direction=-1;
 for(unsigned attempt=0;attempt<=80;++attempt){
  CombatHealth ah{},bh{};const CombatHealthContext ac{(uint8_t)ap->plan,ap->reactor_id,av->damage_taken},bc{(uint8_t)bp->plan,bp->reactor_id,bv->damage_taken};if(combat_health(ac,ah)!=LogicError::ok||combat_health(bc,bh)!=LogicError::ok)return EngagementError::bad_state;
  e.attacker_damage=av->damage_taken;e.defender_damage=bv->damage_taken;e.attacker_health=ah.remaining;e.defender_health=bh.remaining;
  if(!ah.remaining||!bh.remaining){e.boundary=!ah.remaining?EngagementBoundary::attacker_casualty:EngagementBoundary::defender_casualty;next.ready_=true;out.swap(next);return EngagementError::ok;}
  if(attempt==80)return EngagementError::overflow;
  const CombatRoundContext ctx{e.strengths.offense,e.strengths.defense,false,ac,bc,e.next_crt_state};CombatRoundResult round{};err=combat_round_preview(ctx,round);if(err!=LogicError::ok)return mapped(err);++e.rounds;e.draws+=round.draw_count;e.next_crt_state=round.next_crt_state;av->damage_taken=(uint8_t)round.attacker_damage;bv->damage_taken=(uint8_t)round.defender_damage;
  if(round.hit_side==2&&round.defender_health&&e.remaining_counter){--e.remaining_counter;if(!e.remaining_counter){RetreatDestination destination{};err=world_land_retreat_destination(next.frame_,d,defender,facts.incoming_direction,facts.multiplayer_active,facts.pods,destination);if(err!=LogicError::ok)return mapped(err);
   if(destination.direction>=0){if(bp->carry_capacity||(bp->ability_flags&128u)||bv->prev_veh_id_stack!=-1||bv->next_veh_id_stack!=-1)return EngagementError::unsupported;
    if(next.frame_.game().data_version>=11&&bv->unit_id>=64){const auto* owner=next.frame_.faction_definition((size_t)bv->unit_id/64);if(!owner||owner->faction_bonus_count<0||owner->faction_bonus_count>8)return EngagementError::bad_state;for(int i=0;i<owner->faction_bonus_count;++i)if(owner->faction_bonus_id[i]==15)return EngagementError::unsupported;}
    const auto&m=next.frame_.game().map.info;VehicleStackStorage storage{(int32_t)m.width,(int32_t)m.height,const_cast<OriginalMapTile*>(next.frame_.tile(0)),m.tile_count,const_cast<OriginalVehicle*>(next.frame_.vehicle(0)),next.frame_.game().vehicle_count};const auto se=relocate_vehicle_storage(storage,defender,destination.x,destination.y,0);if(se!=StackError::ok)return se==StackError::out_of_memory?EngagementError::out_of_memory:EngagementError::bad_state;
    e.attacker_damage=round.attacker_damage;e.defender_damage=round.defender_damage;e.attacker_health=round.attacker_health;e.defender_health=round.defender_health;e.retreat_direction=destination.direction;e.retreat_x=destination.x;e.retreat_y=destination.y;e.boundary=EngagementBoundary::retreated;next.ready_=true;out.swap(next);return EngagementError::ok;
   }
  }}
 }
 return EngagementError::overflow;
}
}
