/* SPDX-License-Identifier: GPL-3.0-or-later. Policy and attribution in header. */
#include "world_drone_transition.h"
#include "world_unit_stats.h"
namespace alpha {namespace {
bool overlap(const void*a,size_t n,const void*b,size_t m){if(!n||!m)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(x<y+m&&y<x+n);}
bool aliases(const OriginalWorld&w,const void*p,size_t n){
 if(overlap(p,n,&w,sizeof w))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();return overlap(p,n,w.original_bytes(),w.original_size())||overlap(p,n,w.tile(0),g.map.info.tile_count*sizeof(OriginalMapTile))||
  overlap(p,n,w.base(0),g.base_count*sizeof(OriginalBase))||overlap(p,n,w.vehicle(0),g.vehicle_count*sizeof(OriginalVehicle))||
  overlap(p,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(p,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(p,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
DroneTransitionError translate(PsychError e){switch(e){case PsychError::bad_buffer:return DroneTransitionError::bad_buffer;case PsychError::unsupported:return DroneTransitionError::unsupported;case PsychError::overflow:return DroneTransitionError::overflow;case PsychError::out_of_memory:return DroneTransitionError::out_of_memory;default:return DroneTransitionError::bad_state;}}
}
DroneTransitionError world_drone_transition(const OriginalWorld&w,const Rules&t,const UnitDefinitions&u,const BasicRules&b,const FacilityDefinitions&d,size_t id,const WorldDroneStage&s,CombatRandomState&rng,WorldDroneResult&out){
 const void* inputs[]={&t,&u,&b,&d,&s};const size_t sizes[]={sizeof t,sizeof u,sizeof b,sizeof d,sizeof s};
 if(aliases(w,&rng,sizeof rng)||aliases(w,&out,sizeof out)||overlap(&rng,sizeof rng,&out,sizeof out))return DroneTransitionError::bad_buffer;
 for(unsigned i=0;i<5;++i){if(aliases(w,inputs[i],sizes[i])||overlap(inputs[i],sizes[i],&rng,sizeof rng)||overlap(inputs[i],sizes[i],&out,sizeof out))return DroneTransitionError::bad_buffer;
  for(unsigned j=0;j<i;++j)if(overlap(inputs[i],sizes[i],inputs[j],sizes[j]))return DroneTransitionError::bad_buffer;
 }
 if(!w.loaded()||t.count!=89||u.weapon_count!=21||d.slot_count!=72||d.first_project!=39||d.first_repeatable!=34||d.project_count!=33)return DroneTransitionError::bad_state;
 if(w.game().data_version>=11)return DroneTransitionError::unsupported;
 const auto*base=w.base(id);if(!base||base->pop_size<1||base->faction_id<1||base->faction_id>7)return DroneTransitionError::bad_state;
 const auto&assignment=s.assignment;
 if(assignment.population!=base->pop_size||assignment.specialist_count<0||assignment.specialist_count>base->pop_size||
    !(assignment.worked_tiles&1u)||(assignment.worked_tiles&~0x1fffffu)||s.fresh_psych<0)return DroneTransitionError::bad_state;
 unsigned workers=0;for(uint32_t mask=assignment.worked_tiles;mask;mask&=mask-1)++workers;
 if((int32_t)workers+assignment.specialist_count!=assignment.population+1)return DroneTransitionError::bad_state;
 for(int32_t i=0;i<assignment.specialist_count&&i<16;++i)if(((assignment.specialist_types[i/8]>>(4*(i&7)))&15u)>6)return DroneTransitionError::bad_state;
 WorldDroneResult result{};const WorldPsychStage ps{assignment.specialist_count,s.fresh_psych};
 const auto psych=world_base_psych_policy(w,u,b,id,ps,result.psych);if(psych!=PsychError::ok)return translate(psych);
 const auto*owner=w.faction((size_t)base->faction_id);const auto*definition=w.faction_definition((size_t)base->faction_id);
 if(!owner||!definition||definition->faction_bonus_count<0||definition->faction_bonus_count>8)return DroneTransitionError::bad_state;
 auto&f=result.facts;f.population=base->pop_size;f.talents=result.psych.psych.talents;f.drones=result.psych.psych.drones;f.superdrones=result.psych.psych.superdrones;
 f.pending_police=owner->SE_police_pending;f.assimilation_turns=base->assimilation_turns_left;f.owned_bases=result.psych.facts.faction_base_count;f.difficulty=owner->diff_level;f.ranking=owner->ranking;
 f.state_flags=(uint32_t)base->state_flags;f.governor_flags=(uint32_t)base->governor_flags;f.nerve_stapled=base->nerve_staple_turns_left!=0;
 if(world_has_project(w,30,base->faction_id,f.telepathic_matrix)!=LogicError::ok)return DroneTransitionError::bad_state;
 for(unsigned i=0;i<8;++i)f.built|=(uint64_t)base->facilities_built[i]<<(8*i);
 for(unsigned facility=2;facility<34;++facility){
  for(int32_t i=0;i<definition->faction_bonus_count;++i)if(definition->faction_bonus_val1[i]==(int32_t)facility){
   const int32_t bonus=definition->faction_bonus_id[i];bool protect=bonus==2;
   if(bonus==12){bool known=false;if(world_has_technology(w,t,d.slots[facility].prerequisite,base->faction_id,known)!=LogicError::ok)return DroneTransitionError::bad_state;protect=known;}
   if(protect)f.protected_facilities|=1ULL<<facility;
  }
 }
 auto next=rng;const auto error=drone_transition(f,next,result.transition);if(error!=DroneTransitionError::ok)return error;
 rng=next;out=result;return DroneTransitionError::ok;
}
}
