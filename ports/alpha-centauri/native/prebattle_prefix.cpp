/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "prebattle_prefix.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
int32_t signed_word(uint32_t n){return n&0x80000000u?-(int32_t)(~n)-1:(int32_t)n;}
}
namespace {
bool alias(const OriginalWorld&w,const void*p,size_t n){using detail::effects_range_overlap;using detail::effects_array_overlap;const auto&g=w.game();return effects_range_overlap(p,n,&w,sizeof w)||w.overlaps_byte_storage(p,n)||effects_array_overlap(p,n,g.bunglist.data,g.bunglist.size,1)||effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))||effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))||effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))||effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))||effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))||effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
}
PrefixError PrebattlePrefixPlan::prepare(const OriginalWorld&w,const UnitDefinitions&d,const Rules&t,const BasicRules&r,size_t a,size_t b,const PrebattlePrefixRuntime&run,const AttackModifierLedger&prior,const OriginalLabelsView&labels,const CombatTextLedger&text,size_t limit){
 using E=PrefixError;
 if(run.observation!=BattleEntryObservation::supplied)return E::missing_runtime;
 if(run.pbem_active>1||run.current_faction>7)return E::bad_state;
 if(run.pbem_active)return E::unsupported;
 const WorldBattleReportingRuntime entry{run.observation,run.attack_flags,run.mode,run.execution_argument,run.multiplayer_active};
 auto te=inspect_world_combat_text(w,d,t,r,a,b,entry,prior,labels,text,summary_.reporting);
 if(te!=WorldCombatTextError::ok)return (E)(uint32_t)te;
 const WorldPrebattleNotificationRuntime nr{run.observation,run.current_faction,run.human_status,run.game_flags,run.popup_flag,run.turn,run.prior_warning};
 const auto ne=inspect_world_prebattle_notification(w,a,b,nr,summary_.notification);
 if(ne!=PrebattleNotificationError::ok)return (E)(uint32_t)ne;
 const auto route=summary_.notification.route;
 if(route==PrebattleNotificationRoute::current_defender||route==PrebattleNotificationRoute::remote_popup)return E::notification_required;
 const WorldPrebattleAggressionRuntime ar{run.observation,run.current_faction};
 switch(inspect_world_prebattle_aggression(w,a,b,ar,summary_.aggression)){
 case AggressionError::ok:break;
 case AggressionError::bad_state:return E::bad_state;
 case AggressionError::missing_runtime:return E::missing_runtime;
 case AggressionError::unsupported:return E::unsupported;
 case AggressionError::double_cross_required:return E::double_cross_required;
 case AggressionError::participant_ui_required:return E::participant_ui_required;
 }
 constexpr size_t extra=sizeof(PrebattlePrefixPlan)-sizeof(LandEngagementPlan);if(limit<extra)return E::out_of_memory;
 const auto captured=storage_.capture(w,limit-extra);if(captured!=EngagementError::ok)return captured==EngagementError::out_of_memory?E::out_of_memory:E::bad_state;
 auto&frame=storage_.frame_;auto&av=frame.vehicles_[a];auto&bv=frame.vehicles_[b];const auto&h=summary_.reporting.numeric.entry.hasty;
 av.state=h.attacker_state;av.moves_spent=(uint8_t)h.moves_spent;bv.state=h.defender_state;
 if(route==PrebattleNotificationRoute::silent_contact)frame.factions_[bv.faction_id].diplo_spoke[av.faction_id]=signed_word(summary_.notification.spoke);
 auto&af=frame.factions_[av.faction_id];auto&bf=frame.factions_[bv.faction_id];const auto&g=summary_.aggression;
 af.diplo_status[bv.faction_id]=g.forward_status;bf.diplo_status[av.faction_id]=g.reverse_status;
 af.diplo_combat_count[bv.faction_id]=g.forward_combat_count;bf.diplo_combat_count[av.faction_id]=g.reverse_combat_count;
 const WorldPrebattleTerritoryRuntime tr{run.observation,run.current_faction,run.negotiation_faction,run.negotiation_other,run.negotiation_accumulator};
 const auto e=inspect_world_prebattle_territory(frame,a,b,tr,summary_.territory);if(e!=PrebattleTerritoryError::ok)return (E)(uint32_t)e;
 const auto owner=(size_t)summary_.territory.owner;const auto&effect=summary_.territory.effect;
 frame.factions_[owner].diplo_status[bv.faction_id]=effect.owner_defender_status;
 bf.diplo_status[owner]=effect.defender_owner_status;bf.diplo_friction[owner]=effect.defender_owner_friction;
 ready_=true;return E::ok;
}
void PrebattlePrefixPlan::swap(PrebattlePrefixPlan&p){storage_.swap(p.storage_);auto s=summary_;summary_=p.summary_;p.summary_=s;bool b=ready_;ready_=p.ready_;p.ready_=b;}
PrefixError stage_prebattle_prefix(const OriginalWorld&w,const UnitDefinitions&d,const Rules&t,const BasicRules&r,size_t a,size_t b,const PrebattlePrefixRuntime&run,const AttackModifierLedger&prior,const OriginalLabelsView&labels,const CombatTextLedger&text,PrebattlePrefixPlan&out,size_t limit){
 using E=PrefixError;using detail::effects_range_overlap;
 if(!w.loaded()||a==b||alias(w,&out,sizeof out))return E::bad_state;
 const void*inputs[]={&d,&t,&r,&run,&prior,&labels,&text};const size_t sizes[]={sizeof d,sizeof t,sizeof r,sizeof run,sizeof prior,sizeof labels,sizeof text};
 for(unsigned i=0;i<7;++i){if(alias(w,inputs[i],sizes[i])||effects_range_overlap(inputs[i],sizes[i],&out,sizeof out))return E::bad_state;for(unsigned j=0;j<i;++j)if(effects_range_overlap(inputs[i],sizes[i],inputs[j],sizes[j]))return E::bad_state;}
 if(!labels.bytes||!labels.size||labels.size>0xffffffffu||alias(w,labels.bytes,labels.size)||effects_range_overlap(labels.bytes,labels.size,&out,sizeof out))return E::bad_state;
 for(unsigned i=0;i<7;++i)if(effects_range_overlap(inputs[i],sizes[i],labels.bytes,labels.size))return E::bad_state;
 const auto*owned=out.frame();for(unsigned i=0;i<7;++i)if(owned&&alias(*owned,inputs[i],sizes[i]))return E::bad_state;
 if(owned&&alias(*owned,labels.bytes,labels.size))return E::bad_state;
 PrebattlePrefixPlan next;const auto e=next.prepare(w,d,t,r,a,b,run,prior,labels,text,limit);if(e==E::ok)out.swap(next);return e;
}
}
