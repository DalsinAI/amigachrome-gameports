/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "territorial_aftermath.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
bool alias(const OriginalWorld&w,const void*p,size_t n){using detail::effects_range_overlap;using detail::effects_array_overlap;const auto&g=w.game();return effects_range_overlap(p,n,&w,sizeof w)||w.overlaps_byte_storage(p,n)||effects_array_overlap(p,n,g.bunglist.data,g.bunglist.size,1)||effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))||effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))||effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))||effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))||effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))||effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}

PromotedAftermathError mapped(PromotedBodyError e){using E=PromotedAftermathError;return e==PromotedBodyError::ok?E::ok:e==PromotedBodyError::unsupported?E::unsupported:e==PromotedBodyError::out_of_memory?E::out_of_memory:e==PromotedBodyError::overflow?E::overflow:e==PromotedBodyError::selection_random?E::selection_random:e==PromotedBodyError::promotion_random?E::promotion_random:e==PromotedBodyError::promotion_notification?E::promotion_notification:E::bad_state;}
}
PromotedAftermathError TerritorialBattleAftermathPlan::prepare(const OriginalWorld&w,const UnitDefinitions&units,const Rules&tech,const BasicRules&basic,size_t a,size_t b,const LandResolutionFacts&facts,const BattleAftermathRuntime&run,const TerritoryRuntime&diplomacy,size_t limit){
 using E=PromotedAftermathError;
 if(run.observation!=BookkeepingObservation::supplied||(uint32_t)run.edition>1||run.current_player< -1||run.current_player>7||run.multiplayer_active>1||run.pbem_active>1)return E::bad_state;
 if(run.multiplayer_active||run.pbem_active||(run.edition==BookkeepingEdition::smax)!=(w.game().data_version>=11))return E::unsupported;
 const CasualtyAccumulator*pools[]={&facts.pools.killed,&facts.pools.captured,&facts.pools.native_reward};for(auto*p:pools)if(!p->present||p->value)return E::unsupported;
 const auto*av=w.vehicle(a);const auto*bv=w.vehicle(b);const int32_t af=av->faction_id,bf=bv->faction_id;const auto&m=w.game().map.info;
 const OriginalVehicle*actors[]={av,bv};for(const auto*v:actors){
  if(v->x<0||v->y<0||(uint32_t)v->x>=m.width||(uint32_t)v->y>=m.height||((v->x^v->y)&1))return E::bad_state;
  const auto*t=w.tile((size_t)v->y*(m.width/2)+(uint32_t)v->x/2);if(!t)return E::bad_state;if(t->items&1u)return E::unsupported;
  for(size_t j=0;j<w.game().base_count;++j){const auto*base=w.base(j);if(base->x==v->x&&base->y==v->y)return E::unsupported;}
  for(size_t j=0;j<w.game().vehicle_count;++j)if(j!=a&&j!=b){const auto*other=w.vehicle(j);if(other->x==v->x&&other->y==v->y)return E::unsupported;}
 }
 constexpr size_t extra=sizeof(TerritorialBattleAftermathPlan)-sizeof(TerritorialPromotedLandPlan);if(limit<extra)return E::out_of_memory;
 const PromotionRuntime promotion_runtime{EngagementObservation::supplied,run.edition==BookkeepingEdition::smax?Variant::alien_crossfire:Variant::base,run.current_player};
 auto e=mapped(stage_territorial_promoted_body(w,units,tech,basic,a,b,facts,promotion_runtime,diplomacy,body_,limit-extra));if(e!=E::ok)return e;
 summary_=*body_.summary();territorial_=*body_.territorial_summary();diplomacy_=*body_.diplomacy_runtime();if(summary_.body.kind==LandResolutionKind::retreated||summary_.body.kind==LandResolutionKind::defender_captured)return E::unsupported;
 runtime_=*body_.runtime();pools_=*body_.accumulators();
 WorldBookkeepingFacts wf{run.observation,run.edition,af,bf,summary_.body.kind==LandResolutionKind::attacker_deleted?BookkeepingOutcome::attacker_lost:BookkeepingOutcome::defender_lost,run.current_player,run.multiplayer_active,run.pbem_active,run.start_year,{},{}};for(size_t i=0;i<8;++i){wf.wins[i]=run.wins[i];wf.losses[i]=run.losses[i];}
 const size_t heap=body_.frame()->memory_bytes()-sizeof(OriginalWorld);constexpr size_t outside=sizeof(TerritorialBattleAftermathPlan)-sizeof(WorldBattleBookkeepingPlan);if(heap>limit||outside>limit-heap)return E::out_of_memory;
 const auto status=stage_world_battle_bookkeeping(*body_.frame(),wf,after_,limit-heap-outside);if(status!=WorldBookkeepingError::ok)return status==WorldBookkeepingError::out_of_memory?E::out_of_memory:status==WorldBookkeepingError::monument_ui?E::monument_ui:status==WorldBookkeepingError::unsupported?E::unsupported:E::bad_state;
 TerritorialPromotedLandPlan empty;body_.swap(empty);ready_=true;return E::ok;
}
void TerritorialBattleAftermathPlan::swap(TerritorialBattleAftermathPlan&p){body_.swap(p.body_);auto t=territorial_;territorial_=p.territorial_;p.territorial_=t;auto d=diplomacy_;diplomacy_=p.diplomacy_;p.diplomacy_=d;after_.swap(p.after_);const auto summary=summary_;summary_=p.summary_;p.summary_=summary;const auto runtime=runtime_;runtime_=p.runtime_;p.runtime_=runtime;const auto pools=pools_;pools_=p.pools_;p.pools_=pools;const auto ready=ready_;ready_=p.ready_;p.ready_=ready;}
PromotedAftermathError stage_territorial_aftermath(const OriginalWorld&w,const UnitDefinitions&units,const Rules&tech,const BasicRules&basic,size_t a,size_t b,const LandResolutionFacts&facts,const BattleAftermathRuntime&run,const TerritoryRuntime&diplomacy,TerritorialBattleAftermathPlan&out,size_t limit){
 using detail::effects_range_overlap;if(!w.loaded()||a==b||!w.vehicle(a)||!w.vehicle(b)||alias(w,&out,sizeof out))return PromotedAftermathError::bad_state;
 const void*inputs[]={&units,&tech,&basic,&facts,&run,&diplomacy};const size_t sizes[]={sizeof units,sizeof tech,sizeof basic,sizeof facts,sizeof run,sizeof diplomacy};for(unsigned j=0;j<6;++j){if(alias(w,inputs[j],sizes[j])||effects_range_overlap(&out,sizeof out,inputs[j],sizes[j]))return PromotedAftermathError::bad_state;for(unsigned k=0;k<j;++k)if(effects_range_overlap(inputs[j],sizes[j],inputs[k],sizes[k]))return PromotedAftermathError::bad_state;}
 if(const auto*owned=out.frame())for(unsigned j=0;j<6;++j)if(alias(*owned,inputs[j],sizes[j]))return PromotedAftermathError::bad_state;
 TerritorialBattleAftermathPlan next;const auto e=next.prepare(w,units,tech,basic,a,b,facts,run,diplomacy,limit);if(e==PromotedAftermathError::ok)out.swap(next);return e;
}
}
