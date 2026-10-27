/* SPDX-License-Identifier: GPL-3.0-or-later. Included stage attributions apply. */
#include "land_resolution.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
bool alias(const OriginalWorld&w,const void*p,size_t n){using detail::effects_range_overlap;using detail::effects_array_overlap;const auto&g=w.game();return effects_range_overlap(p,n,&w,sizeof w)||w.overlaps_byte_storage(p,n)||effects_array_overlap(p,n,g.bunglist.data,g.bunglist.size,1)||effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))||effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))||effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))||effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))||effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))||effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
EngagementError mapped(RemovalError e){return e==RemovalError::ok?EngagementError::ok:e==RemovalError::out_of_memory?EngagementError::out_of_memory:e==RemovalError::unsupported?EngagementError::unsupported:EngagementError::bad_state;}
}
const OriginalWorld*LandResolutionPlan::frame()const{if(!ready_)return nullptr;if(summary_.kind==LandResolutionKind::retreated)return engagement_.frame();if(summary_.kind==LandResolutionKind::defender_captured)return capture_.frame();return deletion_.frame();}
const RemovalRuntime*LandResolutionPlan::runtime()const{if(!ready_||summary_.kind==LandResolutionKind::retreated)return nullptr;return summary_.kind==LandResolutionKind::defender_captured?capture_.runtime():deletion_.runtime();}
EngagementError LandResolutionPlan::prepare(const OriginalWorld&w,const UnitDefinitions&units,const Rules&tech,const BasicRules&basic,size_t a,size_t b,const LandResolutionFacts&facts,size_t limit){
 if(facts.caller_arguments!=EngagementObservation::supplied||facts.attacker_loss_argument6< -1||facts.attacker_loss_argument6>7||facts.defender_loss_argument6< -1||facts.defender_loss_argument6>7||facts.removal.observation!=RemovalObservation::supplied)return EngagementError::bad_state;
 if(facts.combat.multiplayer_active||facts.removal.multiplayer_active||facts.removal.logging_enabled||facts.removal.base_window_visible)return EngagementError::unsupported;
 const size_t actors[]={a,b};for(const auto id:actors){
  const auto*v=w.vehicle(id);if(!v||v->unit_id<0||v->unit_id>=512||v->faction_id>7)return EngagementError::bad_state;
  const auto*p=w.prototype((size_t)v->unit_id);if(!p)return EngagementError::bad_state;
  if(!v->faction_id||v->prev_veh_id_stack!=-1||v->next_veh_id_stack!=-1||p->carry_capacity||(p->ability_flags&128u))return EngagementError::unsupported;
  if(w.game().data_version>=11&&v->unit_id>=64){const auto*f=w.faction_definition((size_t)v->unit_id/64);if(!f||f->faction_bonus_count<0||f->faction_bonus_count>8)return EngagementError::bad_state;for(int j=0;j<f->faction_bonus_count;++j)if(f->faction_bonus_id[j]==15)return EngagementError::unsupported;}
 }
 constexpr size_t extra=sizeof(LandResolutionPlan)-sizeof(LandEngagementPlan);if(limit<extra)return EngagementError::out_of_memory;
 auto error=stage_land_engagement(w,units,tech,basic,a,b,facts.combat,engagement_,limit-extra);if(error!=EngagementError::ok)return error;
 summary_.engagement=*engagement_.summary();summary_.attacker_after=(int32_t)a;summary_.defender_after=(int32_t)b;pools_=facts.pools;
 if(summary_.engagement.boundary==EngagementBoundary::retreated){summary_.kind=LandResolutionKind::retreated;summary_.casualty_before=summary_.counterpart_before=summary_.argument6=summary_.source_casualty_return=-1;ready_=true;return EngagementError::ok;}
 if(!summary_.engagement.attacker_health&&!summary_.engagement.defender_health)return EngagementError::unsupported;
 const bool attacker_lost=summary_.engagement.boundary==EngagementBoundary::attacker_casualty;const auto victim=attacker_lost?a:b;
 const auto&current=*engagement_.frame();const auto*actor=current.vehicle(victim);const auto*proto=current.prototype((size_t)actor->unit_id);
 const CasualtyInputs in{attacker_lost?-1:(int32_t)a,attacker_lost?facts.attacker_loss_argument6:facts.defender_loss_argument6,facts.pools.killed,facts.pools.captured,facts.pools.native_reward};
 summary_.casualty_before=(int32_t)victim;summary_.counterpart_before=in.counterpart_actor;summary_.argument6=in.faction_argument6;
 const size_t heap=current.memory_bytes()-sizeof(OriginalWorld);
 if(!attacker_lost&&(uint8_t)proto->plan==12){
  constexpr size_t outside=sizeof(LandResolutionPlan)-sizeof(ArtifactCapturePlan);if(heap>limit||outside>limit-heap)return EngagementError::out_of_memory;
  error=mapped(stage_artifact_capture(current,victim,facts.removal,units,tech,basic.move_rate_roads,in,capture_,limit-heap-outside));if(error!=EngagementError::ok)return error;
  const auto&p=*capture_.accumulators();pools_={p.killed,p.captured,p.native_reward};summary_.kind=LandResolutionKind::defender_captured;summary_.source_casualty_return=0;
 }else{
  constexpr size_t outside=sizeof(LandResolutionPlan)-sizeof(CasualtyPlan);if(heap>limit||outside>limit-heap)return EngagementError::out_of_memory;
  error=mapped(stage_casualty(current,victim,facts.removal,in,deletion_,limit-heap-outside));if(error!=EngagementError::ok)return error;
  const auto&p=*deletion_.accumulators();pools_={p.killed,p.captured,p.native_reward};summary_.kind=attacker_lost?LandResolutionKind::attacker_deleted:LandResolutionKind::defender_deleted;summary_.source_casualty_return=1;
  summary_.attacker_after=attacker_lost?-1:(int32_t)(a-(a>victim));summary_.defender_after=attacker_lost?(int32_t)(b-(b>victim)):-1;
 }
 // Keep only the final owned frame after the intermediate heap was budgeted.
 LandEngagementPlan empty;engagement_.swap(empty);ready_=true;return EngagementError::ok;
}
void LandResolutionPlan::swap(LandResolutionPlan&p){engagement_.swap(p.engagement_);deletion_.swap(p.deletion_);capture_.swap(p.capture_);const auto pools=pools_;pools_=p.pools_;p.pools_=pools;const auto summary=summary_;summary_=p.summary_;p.summary_=summary;const auto ready=ready_;ready_=p.ready_;p.ready_=ready;}
EngagementError stage_land_resolution(const OriginalWorld&w,const UnitDefinitions&units,const Rules&tech,const BasicRules&basic,size_t a,size_t b,const LandResolutionFacts&facts,LandResolutionPlan&out,size_t limit){
 using detail::effects_range_overlap;const void*inputs[]={&units,&tech,&basic,&facts};const size_t sizes[]={sizeof units,sizeof tech,sizeof basic,sizeof facts};
 if(!w.loaded()||w.game().vehicle_count>2048||w.game().preferences.VehCount!=(int32_t)w.game().vehicle_count||a==b||!w.vehicle(a)||!w.vehicle(b)||alias(w,&out,sizeof out))return EngagementError::bad_state;
 for(unsigned j=0;j<4;++j){if(alias(w,inputs[j],sizes[j])||effects_range_overlap(&out,sizeof out,inputs[j],sizes[j]))return EngagementError::bad_state;for(unsigned k=0;k<j;++k)if(effects_range_overlap(inputs[j],sizes[j],inputs[k],sizes[k]))return EngagementError::bad_state;}
 if(const auto*owned=out.frame())for(unsigned j=0;j<4;++j)if(alias(*owned,inputs[j],sizes[j]))return EngagementError::bad_state;
 LandResolutionPlan next;const auto e=next.prepare(w,units,tech,basic,a,b,facts,limit);if(e==EngagementError::ok)out.swap(next);return e;
}
}
