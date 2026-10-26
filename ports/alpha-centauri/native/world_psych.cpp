/* SPDX-License-Identifier: GPL-3.0-or-later. See world_psych.h. */
#include "world_psych.h"
#include "world_pacifism.h"
#include "world_unit_stats.h"
#include "technology_effects_ranges.h"
#include <stdlib.h>
namespace alpha { namespace {
using detail::effects_range_overlap;
bool array(const void* out,size_t n,const void* p,size_t count,size_t size) {
 return detail::effects_array_overlap(out,n,p,count,size);
}
bool sealed(const OriginalWorld& w,const Rules& r,const UnitDefinitions& u,const BasicRules& b,const WorldPsychContext& c,const WorldPsych& out) {
 const void* p=&out;const size_t n=sizeof out;
 return !(effects_range_overlap(p,n,&w,sizeof w)||effects_range_overlap(p,n,&r,sizeof r)||effects_range_overlap(p,n,&u,sizeof u)||effects_range_overlap(p,n,&b,sizeof b)||effects_range_overlap(p,n,&c,sizeof c)||effects_range_overlap(p,n,w.original_bytes(),w.original_size())||
 array(p,n,w.tile(0),w.game().map.info.tile_count,sizeof(OriginalMapTile))||array(p,n,w.vehicle(0),w.game().vehicle_count,sizeof(OriginalVehicle))||array(p,n,w.base(0),w.game().base_count,sizeof(OriginalBase))||array(p,n,w.prototype(0),512,sizeof(OriginalPrototype))||array(p,n,w.faction(0),8,sizeof(OriginalFactionState))||array(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition)));
}
int32_t root(uint32_t count) {
 if(count<=1)return (int32_t)count;
 uint32_t next=count/2,prior;
 do {prior=next;next=(next+count/next)/2;}while(next<prior);
 return (int32_t)prior;
}
}
WorldPsychError world_psych(const OriginalWorld& w,const Rules& rules,const UnitDefinitions& units,
 const BasicRules& basic,size_t base,const WorldPsychContext& context,WorldPsych& out) {
 if(!w.loaded())return WorldPsychError::bad_state;
 if(!sealed(w,rules,units,basic,context,out))return WorldPsychError::bad_buffer;
 const auto* b=w.base(base);
 if(!b || context.upkeep_stage>1 || context.snapshot!=PoliceSnapshotProfile::offline_validated)return WorldPsychError::bad_state;
 const auto* f=w.faction((size_t)b->faction_id);const auto* d=w.faction_definition((size_t)b->faction_id);
 if(!f || !d)return WorldPsychError::bad_state;
 BasePsychFacts facts{};facts.expansion=w.game().data_version>=11;
 facts.human=(w.game().preferences.FactionStatus[0]&(1u<<b->faction_id))!=0;
 facts.upkeep_stage=context.upkeep_stage;facts.population=b->pop_size;facts.specialists=b->specialist_total;
 facts.assimilation_turns=b->assimilation_turns_left;facts.difficulty=f->diff_level;facts.pending_efficiency=f->SE_effic_pending;
 facts.map_factor=root(w.game().map.info.tile_count);facts.base_id=(int32_t)base;facts.faction_base_count=f->base_count;
 facts.rule_drone=d->rule_drone;facts.rule_talent=d->rule_talent;
 if(facts.expansion) {
  if(d->faction_bonus_count<0 || d->faction_bonus_count>8)return WorldPsychError::bad_state;
  for(int32_t i=0;i<d->faction_bonus_count;++i)if(d->faction_bonus_id[i]==14){facts.has_nodrone=1;facts.nodrone=d->faction_bonus_val1[i];break;}
 }
 facts.pending_talent=f->SE_talent_pending;facts.pending_police=f->SE_police_pending;
 WorldPacifism pac;
 if(world_pacifism(w,units,base,pac)!=LogicError::ok)return WorldPsychError::bad_state;
 facts.pacifism_count=pac.count;facts.psych=context.current_psych;facts.genejack_drones=basic.drones_genejack_factory;
 facts.pending_economics=f->SE_Economics_pending;facts.nerve_staple_turns=b->nerve_staple_turns_left;
 for(unsigned i=0;i<8;++i)facts.facilities[i/4]|=(uint32_t)b->facilities_built[i]<<(8*(i%4));
 const unsigned projects[]={0,6,7,14,25,26};
 for(unsigned i:projects) {
  bool owned;if(world_has_project(w,i,b->faction_id,owned)!=LogicError::ok)return WorldPsychError::bad_state;
  if(owned)facts.owned_projects|=1u<<i;
 }
 facts.worm_police=(d->rule_flags&0x40u)!=0;
 // Original social pointer starts40 bytes before the8x9 projection matrix.
 facts.social_psych[0]=f->saved_queue_items[7][8];facts.social_psych[1]=f->saved_queue_items[7][9];
 for(unsigned i=0;i<8;++i)facts.social_psych[2+i]=f->social_support[i];
 for(unsigned i=0;i<72;++i)facts.social_psych[10+i]=f->social_psych[i/9][i%9];
 auto* g=(WorldPoliceGarrison*)malloc(sizeof(WorldPoliceGarrison));if(!g)return WorldPsychError::out_of_memory;
 const auto ge=world_police_garrison(w,units,rules,base,context.snapshot,*g);
 if(ge!=PoliceError::ok){free(g);return ge==PoliceError::out_of_memory?WorldPsychError::out_of_memory:WorldPsychError::bad_state;}
 facts.police_units=g->units;facts.police_count=g->count;
 WorldPsych next{};next.variant=g->variant;next.base=(int32_t)base;next.map_factor=facts.map_factor;next.pacifism=pac.count;next.garrison_count=g->count;
 const auto pe=base_psych(facts,next.population);free(g);
 if(pe!=PsychError::ok)return pe==PsychError::overflow?WorldPsychError::overflow:WorldPsychError::bad_state;
 out=next;return WorldPsychError::ok;
}
}
