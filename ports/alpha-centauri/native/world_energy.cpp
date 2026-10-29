/* SPDX-License-Identifier: GPL-3.0-or-later. See world_energy.h/base_energy.h. */
#include "world_energy.h"
#include "world_psych_internal.h"
#include "world_unit_stats.h"
#include "technology_effects_ranges.h"
#include <stdlib.h>
namespace alpha {namespace {
using detail::effects_range_overlap;
bool array(const void* out,size_t n,const void* p,size_t count,size_t size){return detail::effects_array_overlap(out,n,p,count,size);}
bool sealed(const OriginalWorld&w,const Rules&r,const BasicRules&b,const WorldRules&wr,const ResourceInfo&ri,const UnitDefinitions&u,const CitizenDefinitions&cd,const WorldEnergyStage&s,const WorldEnergyResult&out){
 const void*p=&out;const size_t n=sizeof out;
 return !(effects_range_overlap(p,n,&w,sizeof w)||effects_range_overlap(p,n,&r,sizeof r)||effects_range_overlap(p,n,&b,sizeof b)||effects_range_overlap(p,n,&wr,sizeof wr)||effects_range_overlap(p,n,&ri,sizeof ri)||effects_range_overlap(p,n,&u,sizeof u)||effects_range_overlap(p,n,&cd,sizeof cd)||effects_range_overlap(p,n,&s,sizeof s)||w.overlaps_byte_storage(p,n)||array(p,n,w.tile(0),w.game().map.info.tile_count,sizeof(OriginalMapTile))||array(p,n,w.vehicle(0),w.game().vehicle_count,sizeof(OriginalVehicle))||array(p,n,w.base(0),w.game().base_count,sizeof(OriginalBase))||array(p,n,w.prototype(0),512,sizeof(OriginalPrototype))||array(p,n,w.faction(0),8,sizeof(OriginalFactionState))||array(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition)));
}
StableIntakeError logic(LogicError e){return e==LogicError::overflow?StableIntakeError::overflow:StableIntakeError::bad_state;}
StableIntakeError psych(WorldPsychError e){return e==WorldPsychError::out_of_memory?StableIntakeError::out_of_memory:e==WorldPsychError::overflow?StableIntakeError::overflow:StableIntakeError::bad_state;}
bool fac(const OriginalBase&b,unsigned i){return (b.facilities_built[i/8]&(1u<<(i%8)))!=0;}
int32_t headquarters(const OriginalWorld&w,const OriginalBase&b){
 int32_t best=999;const auto&map=w.game().map.info;
 for(size_t i=0;i<w.game().base_count;++i){const auto&h=*w.base(i);if(h.faction_id!=b.faction_id||!fac(h,1))continue;
  int32_t dx=h.x-b.x;if(dx<0)dx=-dx;if(!map.flat&&dx>(int32_t)map.width/2)dx=(int32_t)map.width-dx;
  int32_t dy=h.y-b.y;if(dy<0)dy=-dy;const int32_t hi=dx>dy?dx:dy,lo=dx<dy?dx:dy;
  const int32_t distance=hi-(((dx+dy)/2-lo+1)/2);if(distance<best)best=distance;
 }
 return best==999?16:best;
}
}
StableIntakeError world_energy(const OriginalWorld&w,const Rules&r,const BasicRules&basic,
 const WorldRules&wr,const ResourceInfo&ri,const UnitDefinitions&units,const CitizenDefinitions&citizens,
 size_t base,const WorldEnergyStage&stage,WorldEnergyResult&out){
 if(!w.loaded())return StableIntakeError::bad_state;
 if(!sealed(w,r,basic,wr,ri,units,citizens,stage,out))return StableIntakeError::bad_buffer;
 if(!w.base(base)||stage.upkeep_stage>1||stage.commerce!=CommerceEnergyStage::current_first_field_fresh_technology||stage.snapshot!=PoliceSnapshotProfile::offline_validated||(uint32_t)stage.alien_grid_policy>1)return StableIntakeError::bad_state;
 WorldEnergyResult next{};auto e=world_fresh_minerals(w,r,basic,wr,ri,units,base,stage.resources,next.resources);if(e!=StableIntakeError::ok)return e;
 const auto&b=*w.base(base);const auto&f=*w.faction((size_t)b.faction_id);const auto&d=*w.faction_definition((size_t)b.faction_id);
 auto le=fresh_world_commerce(w,r,base,next.commerce);if(le!=LogicError::ok)return logic(le);
 WorldCitizenEnergy citizen;le=world_citizen_energy(w,r,citizens,b.faction_id,citizen);if(le!=LogicError::ok)return logic(le);
 auto*g=(WorldPoliceGarrison*)malloc(sizeof(WorldPoliceGarrison));if(!g)return StableIntakeError::out_of_memory;
 BaseEnergyFacts facts{}; // population.psych is assigned from fresh spending by base_energy.
 const auto pe=detail::resolve_world_psych_facts(w,r,units,basic,base,stage.snapshot,stage.upkeep_stage,facts.population,*g);
 if(pe!=WorldPsychError::ok){free(g);return psych(pe);}
 auto&a=facts.allocation;a.intake=next.resources.food.fresh.intake.intake[2];a.convoy_to=next.resources.food.convoys.to[2];a.convoy_from=next.resources.food.convoys.from[2];
 for(unsigned i=0;i<7;++i)a.commerce[i]=next.commerce.imports[i];
 a.allocation_psych=f.SE_alloc_psych;a.allocation_labs=f.SE_alloc_labs;a.pending_efficiency=f.SE_effic_pending;
 next.hq_distance=headquarters(w,b);a.hq_distance=next.hq_distance;a.creche=fac(b,2);a.upkeep_stage=stage.upkeep_stage;
 for(unsigned i=0;i<9;++i)a.counters[i]=f.social_effic[i];
 a.faction_surplus=f.energy_surplus_total;
 for(unsigned i=0;i<7;++i)facts.citizens[i]=citizen.citizens[i];
 facts.minimum_specialist_size=basic.min_base_size_specialists;
 for(unsigned i=0;i<2;++i)facts.specialist_types[i]=(uint32_t)b.specialist_types[i];
 const auto&p=w.game().preferences;
 for(unsigned i=0;i<32;++i)if(p.SecretProjects[i]==(int32_t)base)facts.projects_here|=1u<<i;
 facts.rule_research=d.rule_research;
 for(size_t i=0;i<w.game().base_count;++i)if(fac(*w.base(i),8))++facts.network_node_count;
 bool grid=false;if(facts.population.expansion&&world_has_project(w,36,b.faction_id,grid)!=LogicError::ok){free(g);return StableIntakeError::bad_state;}
 facts.energy_grid_project_owned=grid;facts.alien=facts.population.expansion&&(d.rule_flags&0x80u);facts.expansion_enabled=stage.resources.intake.expansion_enabled;facts.sanctioned=f.sanction_turns!=0;
 for(unsigned i=1;i<8;++i){if(facts.population.expansion&&(w.faction_definition(i)->rule_flags&0x80u))facts.partner_aliens|=1u<<(i-1);if(w.faction(i)->sanction_turns)facts.partner_sanctions|=1u<<(i-1);}
 facts.qualified_foreign_commerce=next.commerce.qualified_foreign_commerce;facts.facility_bits64_to69=b.facilities_built[8]&63u;
 for(unsigned i=0;i<64;++i)if(p.SecretProjects[i]==(int32_t)base)++facts.projects_built_here;
 facts.alien_grid_policy=stage.alien_grid_policy;facts.current_item=b.queue_items[0];facts.previous_item=b.production_id_last;facts.mineral_surplus=next.resources.minerals.surplus;
 const auto result=base_energy(facts,next.energy);free(g);
 if(result!=PsychError::ok)return result==PsychError::overflow?StableIntakeError::overflow:StableIntakeError::bad_state;
 out=next;return StableIntakeError::ok;
}
}
