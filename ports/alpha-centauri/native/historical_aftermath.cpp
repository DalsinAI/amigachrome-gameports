/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "historical_aftermath.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
bool alias(const OriginalWorld&w,const void*p,size_t n){using detail::effects_range_overlap;using detail::effects_array_overlap;const auto&g=w.game();return effects_range_overlap(p,n,&w,sizeof w)||w.overlaps_byte_storage(p,n)||effects_array_overlap(p,n,g.bunglist.data,g.bunglist.size,1)||effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))||effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))||effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))||effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))||effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))||effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}

}
PromotedAftermathError HistoricalBattleAftermathPlan::prepare(const OriginalWorld&w,const UnitDefinitions&units,const Rules&tech,const BasicRules&basic,size_t a,size_t b,const LandResolutionFacts&facts,const BattleAftermathRuntime&run,const TerritoryRuntime&diplomacy,size_t limit){
 using E=PromotedAftermathError;const auto*av=w.vehicle(a);const auto*dv=w.vehicle(b);if(av->unit_id<0||av->unit_id>=512||dv->unit_id<0||dv->unit_id>=512||av->faction_id>7||dv->faction_id>7)return E::bad_state;
 // Existing body rejects native attackers and every base tile before success.
 // These are the exact prerequisites excluding the sole original interlude setter.
 const CombatHistoryFacts witness{EngagementObservation::supplied,w.game().data_version>=11?Variant::alien_crossfire:Variant::base,(uint32_t)av->unit_id,(uint32_t)dv->unit_id,av->faction_id,dv->faction_id,0};
 constexpr size_t extra=sizeof(HistoricalBattleAftermathPlan)-sizeof(TerritorialBattleAftermathPlan);if(limit<extra)return E::out_of_memory;auto e=stage_territorial_aftermath(w,units,tech,basic,a,b,facts,run,diplomacy,body_,limit-extra);if(e!=E::ok)return e;
 summary_=*body_.summary();territorial_=*body_.territorial_summary();diplomacy_=*body_.diplomacy_runtime();runtime_=*body_.runtime();pools_=*body_.accumulators();book_=*body_.bookkeeping();
 const size_t heap=body_.frame()->memory_bytes()-sizeof(OriginalWorld);constexpr size_t outside=sizeof(HistoricalBattleAftermathPlan)-sizeof(WorldCombatHistoryPlan);if(heap>limit||outside>limit-heap)return E::out_of_memory;const auto changed=stage_world_combat_history(*body_.frame(),witness,history_,limit-heap-outside);if(changed!=CombatHistoryError::ok)return changed==CombatHistoryError::out_of_memory?E::out_of_memory:changed==CombatHistoryError::unsupported?E::unsupported:E::bad_state;
 {TerritorialBattleAftermathPlan empty;body_.swap(empty);}ready_=true;return E::ok;
}
void HistoricalBattleAftermathPlan::swap(HistoricalBattleAftermathPlan&p){body_.swap(p.body_);history_.swap(p.history_);auto s=summary_;summary_=p.summary_;p.summary_=s;auto t=territorial_;territorial_=p.territorial_;p.territorial_=t;auto d=diplomacy_;diplomacy_=p.diplomacy_;p.diplomacy_=d;auto r=runtime_;runtime_=p.runtime_;p.runtime_=r;auto pool=pools_;pools_=p.pools_;p.pools_=pool;auto book=book_;book_=p.book_;p.book_=book;bool b=ready_;ready_=p.ready_;p.ready_=b;}
PromotedAftermathError stage_historical_aftermath(const OriginalWorld&w,const UnitDefinitions&units,const Rules&tech,const BasicRules&basic,size_t a,size_t b,const LandResolutionFacts&facts,const BattleAftermathRuntime&run,const TerritoryRuntime&diplomacy,HistoricalBattleAftermathPlan&out,size_t limit){
 using detail::effects_range_overlap;if(!w.loaded()||a==b||!w.vehicle(a)||!w.vehicle(b)||alias(w,&out,sizeof out))return PromotedAftermathError::bad_state;
 const void*inputs[]={&units,&tech,&basic,&facts,&run,&diplomacy};const size_t sizes[]={sizeof units,sizeof tech,sizeof basic,sizeof facts,sizeof run,sizeof diplomacy};for(unsigned j=0;j<6;++j){if(alias(w,inputs[j],sizes[j])||effects_range_overlap(&out,sizeof out,inputs[j],sizes[j]))return PromotedAftermathError::bad_state;for(unsigned k=0;k<j;++k)if(effects_range_overlap(inputs[j],sizes[j],inputs[k],sizes[k]))return PromotedAftermathError::bad_state;}
 if(const auto*owned=out.frame())for(unsigned j=0;j<6;++j)if(alias(*owned,inputs[j],sizes[j]))return PromotedAftermathError::bad_state;
 HistoricalBattleAftermathPlan next;const auto e=next.prepare(w,units,tech,basic,a,b,facts,run,diplomacy,limit);if(e==PromotedAftermathError::ok)out.swap(next);return e;
}
}
