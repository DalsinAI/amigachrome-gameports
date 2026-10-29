/* SPDX-License-Identifier: GPL-3.0-or-later. See world_promotion.h. */
#include "world_promotion.h"
#include "world_unit_stats.h"
#include "saved_speed.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
bool alias(const OriginalWorld&w,const void*p,size_t n){using detail::effects_range_overlap;using detail::effects_array_overlap;const auto&g=w.game();return effects_range_overlap(p,n,&w,sizeof w)||w.overlaps_byte_storage(p,n)||effects_array_overlap(p,n,g.bunglist.data,g.bunglist.size,1)||effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))||effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))||effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))||effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))||effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))||effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
}
PromotionError WorldPromotionPlan::prepare(const OriginalWorld&w,const UnitDefinitions&units,const Rules&tech,size_t id,const PromotionRuntime&run,size_t limit){
 using E=PromotionError;
 if(run.observation!=EngagementObservation::supplied||run.current_player< -1||run.current_player>7||(run.edition!=Variant::base&&run.edition!=Variant::alien_crossfire))return E::bad_state;
 if((run.edition==Variant::alien_crossfire)!=(w.game().data_version>=11))return E::unsupported;
 const auto*v=w.vehicle(id);if(!v||v->unit_id<0||v->unit_id>=512||v->faction_id>7)return E::bad_state;
 const auto*p=w.prototype((size_t)v->unit_id);if(!p)return E::bad_state;
 // This subset has genuine whole-helper differential coverage. Additional
 // morale dependencies need their own complete promotion qualification.
 if(!v->faction_id||v->home_base_id!=-1||(uint8_t)p->plan==11)return E::unsupported;
 uint32_t ignored=0;const auto valid=saved_prototype_speed(units,*p,(size_t)v->unit_id,1,false,ignored);if(valid!=LogicError::ok||tech.count!=89)return E::bad_state;
 if((w.game().data_version<11)!=(units.weapon_count==21))return E::bad_state;
 if(units.chassis[p->chassis_id].missile<0||units.chassis[p->chassis_id].missile>255||units.armor[p->armor_id].defense< -128||units.armor[p->armor_id].defense>127||units.weapons[p->weapon_id].offense< -128||units.weapons[p->weapon_id].offense>127)return E::bad_state;
 summary_={v->morale,v->morale,0,false,false};
 // Source tests missile and unsigned plan/signed armor before morale queries.
 if(!units.chassis[p->chassis_id].missile&&((uint8_t)p->plan<8||units.armor[p->armor_id].defense>1)){
  uint32_t effective=0;const auto status=world_effective_morale(w,units,tech,id,true,effective);if(status!=LogicError::ok)return status==LogicError::overflow?E::overflow:E::bad_state;
  summary_.queried_morale=true;summary_.effective_before=effective;
  if(effective<6&&v->morale<6){
   if(effective>2)return E::random_required;
   summary_.raw_after=v->morale+1u;summary_.promoted=true;
  }
 }
 constexpr size_t extra=sizeof(WorldPromotionPlan)-sizeof(LandEngagementPlan);if(limit<extra)return E::out_of_memory;
 const auto captured=storage_.capture(w,limit-extra);if(captured!=EngagementError::ok)return captured==EngagementError::out_of_memory?E::out_of_memory:E::bad_state;
 if(summary_.promoted){
  storage_.frame_.vehicles_[id].morale=(uint8_t)summary_.raw_after;
  if(run.current_player==v->faction_id&&(w.game().preferences.FactionStatus[0]&(1u<<v->faction_id))){
   uint32_t after=0;const auto status=world_effective_morale(storage_.frame_,units,tech,id,true,after);if(status!=LogicError::ok)return status==LogicError::overflow?E::overflow:E::bad_state;
   if(after!=summary_.effective_before)return E::notification_required;
  }
 }
 ready_=true;return E::ok;
}
void WorldPromotionPlan::swap(WorldPromotionPlan&p){storage_.swap(p.storage_);auto s=summary_;summary_=p.summary_;p.summary_=s;bool b=ready_;ready_=p.ready_;p.ready_=b;}
PromotionError stage_world_promotion(const OriginalWorld&w,const UnitDefinitions&units,const Rules&tech,size_t id,const PromotionRuntime&run,WorldPromotionPlan&out,size_t limit){
 using detail::effects_range_overlap;
 if(!w.loaded()||!w.vehicle(id)||alias(w,&out,sizeof out))return PromotionError::bad_state;
 const void*inputs[]={&units,&tech,&run};const size_t sizes[]={sizeof units,sizeof tech,sizeof run};
 for(unsigned j=0;j<3;++j){if(alias(w,inputs[j],sizes[j])||effects_range_overlap(&out,sizeof out,inputs[j],sizes[j]))return PromotionError::bad_state;for(unsigned k=0;k<j;++k)if(effects_range_overlap(inputs[j],sizes[j],inputs[k],sizes[k]))return PromotionError::bad_state;}
 if(const auto*owned=out.frame())for(unsigned j=0;j<3;++j)if(alias(*owned,inputs[j],sizes[j]))return PromotionError::bad_state;
 WorldPromotionPlan next;const auto status=next.prepare(w,units,tech,id,run,limit);if(status==PromotionError::ok)out.swap(next);return status;
}
}
