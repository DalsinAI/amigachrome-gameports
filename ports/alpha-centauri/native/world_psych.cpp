/* SPDX-License-Identifier: GPL-3.0-or-later. See world_psych.h. */
#include "world_psych_internal.h"
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
}
WorldPsychError world_psych(const OriginalWorld& w,const Rules& rules,const UnitDefinitions& units,
 const BasicRules& basic,size_t base,const WorldPsychContext& context,WorldPsych& out) {
 if(!w.loaded())return WorldPsychError::bad_state;
 if(!sealed(w,rules,units,basic,context,out))return WorldPsychError::bad_buffer;
 auto* g=(WorldPoliceGarrison*)malloc(sizeof(WorldPoliceGarrison));if(!g)return WorldPsychError::out_of_memory;
 BasePsychFacts facts{};const auto error=detail::resolve_world_psych_facts(w,rules,units,basic,base,context.snapshot,context.upkeep_stage,facts,*g);
 if(error!=WorldPsychError::ok){free(g);return error;}
 facts.psych=context.current_psych;
 WorldPsych next{};next.variant=g->variant;next.base=(int32_t)base;next.map_factor=facts.map_factor;next.pacifism=facts.pacifism_count;next.garrison_count=g->count;
 const auto pe=base_psych(facts,next.population);free(g);
 if(pe!=PsychError::ok)return pe==PsychError::overflow?WorldPsychError::overflow:WorldPsychError::bad_state;
 out=next;return WorldPsychError::ok;
}
}
