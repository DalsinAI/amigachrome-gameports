/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution and policy in header. */
#include "world_population.h"
#include "world_cost_factor.h"
#include "world_unit_stats.h"
#include "movement.h"
#include <string.h>
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){
 if(!a||!b||!an||!bn)return false;
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;
 return x<=y?y-x<an:x-y<bn;
}
bool aliases(const OriginalWorld&w,const Rules&t,const BasicRules&b,
 const ResourceInfo&r,const WorldPopulationStage&s,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&t,sizeof t)||overlap(out,n,&b,sizeof b)||
  overlap(out,n,&r,sizeof r)||overlap(out,n,&s,sizeof s))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();
 return overlap(out,n,w.original_bytes(),w.original_size())||overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||
  overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||
  overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState))||
  overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
bool built(const OriginalBase&b,unsigned id){return (b.facilities_built[id/8]&(1u<<(id%8)))!=0;}
bool project(const OriginalWorld&w,unsigned id,int32_t faction,bool&out){
 return w.game().preferences.SecretProjects[id]>=-2&&world_has_project(w,id,faction,out)==LogicError::ok;
}
StableIntakeError objective(const OriginalWorld&w,size_t id,bool&out){
 const auto&p=w.game().preferences;const auto*b=w.base(id);
 if(((uint32_t)p.GameRules&0x20000000u)||((uint32_t)b->event_flags&0x8000u)){out=true;return StableIntakeError::ok;}
 if((uint32_t)p.GameRules&0x40000000u){
  for(unsigned i=0;i<33;++i){const int32_t at=p.SecretProjects[i];if(at< -2||(at>=0&&!w.base((size_t)at)))return StableIntakeError::bad_state;if(at==(int32_t)id){out=true;return StableIntakeError::ok;}}
 }
 if((uint32_t)p.GameState&0x4000000u){
  const int32_t facility=p.ScnVictFacilityObj;if(facility<0)return StableIntakeError::bad_state;
  if(facility<34&&built(*b,(unsigned)facility)){out=true;return StableIntakeError::ok;}
 }
 out=false;return StableIntakeError::ok;
}
StableIntakeError cancellations(const OriginalWorld&w,size_t id,WorldPopulationProposal&n){
 if(!n.starvation||!n.facts.full_turn||!n.food.convoys.from[0])return StableIntakeError::ok;
 const auto&g=w.game();const MapGeometry geometry{(int32_t)g.map.info.width,(int32_t)g.map.info.height,g.map.info.flat};
 for(size_t i=0;i<g.vehicle_count;++i){const auto*v=w.vehicle(i);const auto*p=w.prototype((size_t)v->unit_id);
  if(!p)return StableIntakeError::bad_state;
  // Preserve the source's all-channel cancellation once food exports exist.
  if(v->home_base_id!=(int32_t)id||p->plan!=10||v->order!=3||v->moves_spent)continue;
  size_t square;if(movement_tile_index(geometry,v->x,v->y,square)!=LogicError::ok)return StableIntakeError::bad_state;
  if(w.tile(square)->items&1u){n.convoy_cancellations[i/32]|=1u<<(i%32);++n.convoy_cancellation_count;}
 }
 return StableIntakeError::ok;
}
bool same_world_except_assignment(const OriginalWorld&a,const OriginalWorld&b,size_t id,const WorldPopulationProposal&proposal){
 const auto&pending=proposal.transition;
 if(!a.loaded()||!b.loaded()||a.original_size()!=b.original_size()||a.runtime_seed()!=b.runtime_seed())return false;
 const auto&ga=a.game();const auto&gb=b.game();
 if(ga.base_count!=gb.base_count||ga.vehicle_count!=gb.vehicle_count||ga.map.info.tile_count!=gb.map.info.tile_count||ga.data_version!=gb.data_version)return false;
 const auto*base=b.base(id);if(!base||base->pop_size!=pending.population||base->nutrients_accumulated!=pending.accumulated)return false;
 // Both raw containers may contain old or already serialized proposed fields;
 // every opaque byte and every other wire field must remain exactly equal.
 const size_t stride=ga.data_version<11?304:308;
 const size_t offset=(size_t)(ga.bases.data-a.original_bytes())+id*stride;
 const size_t vehicles=(size_t)(ga.vehicles.data-a.original_bytes());
 for(size_t i=0;i<a.original_size();++i){
  if(i==offset+6||(i>=offset+0x3c&&i<offset+0x40)||(i>=offset+0x78&&i<offset+0x7c))continue;
  if(i>=vehicles&&i<vehicles+(size_t)ga.vehicle_count*52&&(i-vehicles)%52==17&&(proposal.convoy_cancellations[((i-vehicles)/52)/32]&(1u<<(((i-vehicles)/52)%32))))continue;
  if(a.original_bytes()[i]!=b.original_bytes()[i])return false;
 }
 for(size_t i=0;i<ga.base_count;++i){auto expected=*a.base(i);if(i==id){expected.pop_size=(int8_t)pending.population;expected.nutrients_accumulated=pending.accumulated;expected.worked_tiles=base->worked_tiles;}if(memcmp(&expected,b.base(i),sizeof expected))return false;}
 for(size_t i=0;i<ga.vehicle_count;++i){auto expected=*a.vehicle(i);if(proposal.convoy_cancellations[i/32]&(1u<<(i%32)))expected.order=0;if(memcmp(&expected,b.vehicle(i),sizeof expected))return false;}
 if(memcmp(&ga.preferences,&gb.preferences,sizeof ga.preferences)||memcmp(&ga.map.info,&gb.map.info,sizeof ga.map.info)||ga.player!=gb.player||ga.tick_seed!=gb.tick_seed||ga.legacy_marker_version!=gb.legacy_marker_version||ga.canvas_width!=gb.canvas_width||ga.canvas_height!=gb.canvas_height||memcmp(ga.map_cursor,gb.map_cursor,sizeof ga.map_cursor)||memcmp(ga.views,gb.views,sizeof ga.views)||memcmp(a.tile(0),b.tile(0),(size_t)ga.map.info.tile_count*sizeof(OriginalMapTile))||
  memcmp(a.prototype(0),b.prototype(0),512*sizeof(OriginalPrototype))||memcmp(a.faction(0),b.faction(0),8*sizeof(OriginalFactionState))||
  memcmp(a.faction_definition(0),b.faction_definition(0),8*sizeof(OriginalFactionDefinition)))return false;
 return true;
}
}
StableIntakeError world_population_begin(const OriginalWorld&w,const Rules&t,
 const BasicRules&b,const ResourceInfo&r,size_t id,const WorldPopulationStage&s,WorldPopulationProposal&out){
 if(aliases(w,t,b,r,s,&out,sizeof out))return StableIntakeError::bad_buffer;
 if(!w.loaded())return StableIntakeError::bad_state;
 if(w.game().data_version>=11||s.intake.variant!=Variant::base||s.intake.expansion_enabled)return StableIntakeError::unsupported_stage;
 const auto*base=w.base(id);if(!base||base->pop_size<1||base->faction_id<1||base->faction_id>7)return StableIntakeError::bad_state;
 const auto*definition=w.faction_definition((size_t)base->faction_id);
 WorldPopulationProposal n{};
 auto error=world_convoy_nutrients(w,t,b,r,id,s.intake,n.food);if(error!=StableIntakeError::ok)return error;
 int32_t factor=0;const auto cost=world_base_cost_factor(w,b,id,0,factor);
 if(cost==CostFactorError::overflow)return StableIntakeError::overflow;
 if(cost!=CostFactorError::ok||factor<1)return StableIntakeError::unsupported_stage;
 bool ascetic=false,cloning=false,obj=false;
 if(!project(w,13,base->faction_id,ascetic)||!project(w,24,base->faction_id,cloning))return StableIntakeError::bad_state;
 error=objective(w,id,obj);if(error!=StableIntakeError::ok)return error;
 const int64_t limit=(int64_t)(built(*base,24)?b.pop_limit_sans_hab_dome:b.pop_limit_sans_hab_complex)+(ascetic?2:0)-definition->rule_population;
 if(limit<(-2147483647LL-1)||limit>2147483647LL)return StableIntakeError::overflow;
 // Wider modded limits do not change comparisons in the signed-byte domain.
 const int32_t bounded_limit=limit< -127?-127:limit>255?255:(int32_t)limit;
 n.facts={base->pop_size,n.food.fresh.nutrients.stored,n.food.fresh.nutrients.surplus,factor,n.food.fresh.nutrients.growth,bounded_limit,b.nutrient_req_citizen,built(*base,25),cloning,s.full_turn,obj};
 const auto growth=begin_population_growth(n.facts,n.transition);
 if(growth!=LogicError::ok)return growth==LogicError::overflow?StableIntakeError::overflow:StableIntakeError::bad_state;
 n.starvation=n.facts.accumulated<0;
 error=cancellations(w,id,n);if(error!=StableIntakeError::ok)return error;
 n.recompute_required=!n.transition.removed&&(n.transition.recompute||n.transition.population!=n.facts.population||n.convoy_cancellation_count);
 out=n;return StableIntakeError::ok;
}
StableIntakeError world_population_finish(const OriginalWorld&before,const OriginalWorld&after,
 const Rules&t,const BasicRules&b,const ResourceInfo&r,size_t id,const WorldPopulationStage&s,WorldPopulationProposal&out){
 if(aliases(before,t,b,r,s,&out,sizeof out)||aliases(after,t,b,r,s,&out,sizeof out))return StableIntakeError::bad_buffer;
 WorldPopulationProposal n{};auto error=world_population_begin(before,t,b,r,id,s,n);if(error!=StableIntakeError::ok)return error;
 if(!n.recompute_required||n.transition.removed||!same_world_except_assignment(before,after,id,n))return StableIntakeError::bad_state;
 ConvoyNutrientResult fresh{};error=world_convoy_nutrients(after,t,b,r,id,s.intake,fresh);if(error!=StableIntakeError::ok)return error;
 if(n.transition.recompute){PopulationTransition finished{};const auto e=finish_population_growth(n.transition,fresh.fresh.nutrients.stored,fresh.fresh.nutrients.surplus,finished);if(e!=LogicError::ok)return e==LogicError::overflow?StableIntakeError::overflow:StableIntakeError::bad_state;n.transition=finished;}
 n.food=fresh;n.recompute_required=false;out=n;return StableIntakeError::ok;
}
}
