/* SPDX-License-Identifier: GPL-3.0-or-later. See promoted_land_body.h. */
#include "territorial_promoted_body.h"
#include "world_unit_stats.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
bool alias(const OriginalWorld&w,const void*p,size_t n){using detail::effects_range_overlap;using detail::effects_array_overlap;const auto&g=w.game();return effects_range_overlap(p,n,&w,sizeof w)||w.overlaps_byte_storage(p,n)||effects_array_overlap(p,n,g.bunglist.data,g.bunglist.size,1)||effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))||effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))||effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))||effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))||effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))||effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
PromotedBodyError mapped(EngagementError e){using E=PromotedBodyError;return e==EngagementError::ok?E::ok:e==EngagementError::out_of_memory?E::out_of_memory:e==EngagementError::overflow?E::overflow:e==EngagementError::unsupported?E::unsupported:E::bad_state;}
}
PromotedBodyError TerritorialPromotedLandPlan::prepare(const OriginalWorld&w,const UnitDefinitions&units,const Rules&tech,const BasicRules&basic,size_t a,size_t b,const LandResolutionFacts&facts,const PromotionRuntime&run,const TerritoryRuntime&diplomacy,size_t limit){
 using E=PromotedBodyError;
 if(facts.caller_arguments!=EngagementObservation::supplied||facts.attacker_loss_argument6< -1||facts.attacker_loss_argument6>7||facts.defender_loss_argument6< -1||facts.defender_loss_argument6>7||facts.removal.observation!=RemovalObservation::supplied||run.observation!=EngagementObservation::supplied||run.current_player< -1||run.current_player>7||(run.edition!=Variant::base&&run.edition!=Variant::alien_crossfire))return E::bad_state;
 if(facts.combat.multiplayer_active||facts.removal.multiplayer_active||facts.removal.logging_enabled||facts.removal.base_window_visible||(run.edition==Variant::alien_crossfire)!=(w.game().data_version>=11))return E::unsupported;
 const auto&m=w.game().map.info;
 const size_t ids[]={a,b};for(size_t id:ids){
  const auto*v=w.vehicle(id);if(!v||v->unit_id<0||v->unit_id>=512||v->faction_id>7)return E::bad_state;const auto*p=w.prototype((size_t)v->unit_id);if(!p)return E::bad_state;
  if(!v->faction_id||v->prev_veh_id_stack!=-1||v->next_veh_id_stack!=-1||p->carry_capacity||(p->ability_flags&128u)||(uint8_t)p->plan==12)return E::unsupported;
  if(w.game().data_version>=11&&v->unit_id>=64){const auto*f=w.faction_definition((size_t)v->unit_id/64);if(!f||f->faction_bonus_count<0||f->faction_bonus_count>8)return E::bad_state;for(int j=0;j<f->faction_bonus_count;++j)if(f->faction_bonus_id[j]==15)return E::unsupported;}
  if(v->x<0||v->y<0||(uint32_t)v->x>=m.width||(uint32_t)v->y>=m.height||((v->x^v->y)&1))return E::bad_state;
  if(w.tile((size_t)v->y*(m.width/2)+(uint32_t)v->x/2)->items&1u)return E::unsupported;
  for(size_t j=0;j<w.game().base_count;++j){const auto*base=w.base(j);if(base->x==v->x&&base->y==v->y)return E::unsupported;}
  for(size_t j=0;j<w.game().vehicle_count;++j)if(j!=a&&j!=b){const auto*other=w.vehicle(j);if(other->x==v->x&&other->y==v->y)return E::unsupported;}
 }
 const auto*av=w.vehicle(a);const auto*dv=w.vehicle(b);const auto*ap=w.prototype((size_t)av->unit_id);const auto*dp=w.prototype((size_t)dv->unit_id);CombatHealth initial_a{},initial_d{};
 auto le=combat_health({(uint8_t)ap->plan,ap->reactor_id,av->damage_taken},initial_a);if(le!=LogicError::ok)return E::bad_state;le=combat_health({(uint8_t)dp->plan,dp->reactor_id,dv->damage_taken},initial_d);if(le!=LogicError::ok)return E::bad_state;
 if(!initial_a.remaining||!initial_d.remaining)return E::unsupported;
 summary_.initial_attacker_health=initial_a.remaining;summary_.initial_defender_health=initial_d.remaining;
 constexpr size_t extra=sizeof(TerritorialPromotedLandPlan)-sizeof(LandEngagementPlan);if(limit<extra)return E::out_of_memory;
 auto error=mapped(stage_land_engagement(w,units,tech,basic,a,b,facts.combat,engagement_,limit-extra));if(error!=E::ok)return error;
 auto&body=summary_.body;body.engagement=*engagement_.summary();if(body.engagement.boundary==EngagementBoundary::retreated||(!body.engagement.attacker_health&&!body.engagement.defender_health))return E::unsupported;
 const bool lost=body.engagement.boundary==EngagementBoundary::attacker_casualty;const size_t victim=lost?a:b,winner=lost?b:a;const OriginalWorld*current=engagement_.frame();uint32_t morale=0;
 {const size_t heap=current->memory_bytes()-sizeof(OriginalWorld);constexpr size_t outside=sizeof(TerritorialPromotedLandPlan)-sizeof(WorldTerritorialCombatPlan);if(heap>limit||outside>limit-heap)return E::out_of_memory;
  const auto de=stage_world_territorial_combat(*current,a,b,diplomacy,territory_,limit-heap-outside);if(de!=WorldTerritoryError::ok)return de==WorldTerritoryError::out_of_memory?E::out_of_memory:de==WorldTerritoryError::unsupported?E::unsupported:E::bad_state;
  territorial_=*territory_.summary();diplomacy_=*territory_.runtime();current=territory_.frame();
  LandEngagementPlan empty;engagement_.swap(empty);
 }

 le=world_effective_morale(*current,units,tech,winner,true,morale);if(le!=LogicError::ok)return le==LogicError::overflow?E::overflow:E::bad_state;
 const PromotionSelectionContext selection{SelectionObservation::supplied,lost?SelectionOutcome::attacker_lost:SelectionOutcome::attacker_won,{(uint8_t)ap->plan,ap->reactor_id,body.engagement.attacker_damage},{(uint8_t)dp->plan,dp->reactor_id,body.engagement.defender_damage},body.engagement.strengths.offense,body.engagement.strengths.defense,initial_a.remaining,initial_d.remaining,morale,false};
 le=inspect_promotion_selection(selection,summary_.selection);if(le!=LogicError::ok)return E::bad_state;if(summary_.selection.intent==PromotionIntent::random_required)return E::selection_random;
 if(summary_.selection.intent!=PromotionIntent::promote)return E::unsupported;
 if(summary_.selection.intent==PromotionIntent::promote){
  const size_t heap=current->memory_bytes()-sizeof(OriginalWorld);constexpr size_t outside=sizeof(TerritorialPromotedLandPlan)-sizeof(WorldPromotionPlan);if(heap>limit||outside>limit-heap)return E::out_of_memory;
  const auto pe=stage_world_promotion(*current,units,tech,winner,run,promotion_,limit-heap-outside);
  if(pe!=PromotionError::ok)return pe==PromotionError::out_of_memory?E::out_of_memory:pe==PromotionError::overflow?E::overflow:pe==PromotionError::random_required?E::promotion_random:pe==PromotionError::notification_required?E::promotion_notification:pe==PromotionError::unsupported?E::unsupported:E::bad_state;
  summary_.helper_called=true;summary_.promotion=*promotion_.summary();current=promotion_.frame();
  {WorldTerritorialCombatPlan empty;territory_.swap(empty);}
 }
 const CasualtyInputs in{lost?-1:(int32_t)a,lost?facts.attacker_loss_argument6:facts.defender_loss_argument6,facts.pools.killed,facts.pools.captured,facts.pools.native_reward};
 const size_t heap=current->memory_bytes()-sizeof(OriginalWorld);constexpr size_t outside=sizeof(TerritorialPromotedLandPlan)-sizeof(CasualtyPlan);if(heap>limit||outside>limit-heap)return E::out_of_memory;
 const auto de=stage_casualty(*current,victim,facts.removal,in,deletion_,limit-heap-outside);if(de!=RemovalError::ok)return de==RemovalError::out_of_memory?E::out_of_memory:de==RemovalError::unsupported?E::unsupported:E::bad_state;
 const auto&p=*deletion_.accumulators();pools_={p.killed,p.captured,p.native_reward};body.kind=lost?LandResolutionKind::attacker_deleted:LandResolutionKind::defender_deleted;body.casualty_before=(int32_t)victim;body.counterpart_before=in.counterpart_actor;body.argument6=in.faction_argument6;body.source_casualty_return=1;body.attacker_after=lost?-1:(int32_t)(a-(a>victim));body.defender_after=lost?(int32_t)(b-(b>victim)):-1;
 {LandEngagementPlan empty;engagement_.swap(empty);}{WorldPromotionPlan empty;promotion_.swap(empty);}ready_=true;return E::ok;
}
void TerritorialPromotedLandPlan::swap(TerritorialPromotedLandPlan&p){engagement_.swap(p.engagement_);territory_.swap(p.territory_);auto t=territorial_;territorial_=p.territorial_;p.territorial_=t;auto d=diplomacy_;diplomacy_=p.diplomacy_;p.diplomacy_=d;promotion_.swap(p.promotion_);deletion_.swap(p.deletion_);auto s=summary_;summary_=p.summary_;p.summary_=s;auto pool=pools_;pools_=p.pools_;p.pools_=pool;bool b=ready_;ready_=p.ready_;p.ready_=b;}
PromotedBodyError stage_territorial_promoted_body(const OriginalWorld&w,const UnitDefinitions&units,const Rules&tech,const BasicRules&basic,size_t a,size_t b,const LandResolutionFacts&facts,const PromotionRuntime&run,const TerritoryRuntime&diplomacy,TerritorialPromotedLandPlan&out,size_t limit){
 using detail::effects_range_overlap;if(!w.loaded()||w.game().vehicle_count>2048||w.game().preferences.VehCount!=(int32_t)w.game().vehicle_count||a==b||!w.vehicle(a)||!w.vehicle(b)||alias(w,&out,sizeof out))return PromotedBodyError::bad_state;
 const void*inputs[]={&units,&tech,&basic,&facts,&run,&diplomacy};const size_t sizes[]={sizeof units,sizeof tech,sizeof basic,sizeof facts,sizeof run,sizeof diplomacy};for(unsigned j=0;j<6;++j){if(alias(w,inputs[j],sizes[j])||effects_range_overlap(&out,sizeof out,inputs[j],sizes[j]))return PromotedBodyError::bad_state;for(unsigned k=0;k<j;++k)if(effects_range_overlap(inputs[j],sizes[j],inputs[k],sizes[k]))return PromotedBodyError::bad_state;}
 if(const auto*owned=out.frame())for(unsigned j=0;j<6;++j)if(alias(*owned,inputs[j],sizes[j]))return PromotedBodyError::bad_state;
 TerritorialPromotedLandPlan next;const auto e=next.prepare(w,units,tech,basic,a,b,facts,run,diplomacy,limit);if(e==PromotedBodyError::ok)out.swap(next);return e;
}
}
