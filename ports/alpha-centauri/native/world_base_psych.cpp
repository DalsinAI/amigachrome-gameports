/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in base_psych.h.
 * Integer map quick_root: complete OpenSMACX general.cpp005290E0,
 * (C)2013-2021 Brendan Casey, pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 */
#include "world_base_psych.h"
#include "world_base_support.h"
#include "world_unit_stats.h"
#include <stdlib.h>
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){if(!a||!b||!an||!bn)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
bool aliases(const OriginalWorld&w,const UnitDefinitions&u,const BasicRules&b,const WorldPsychStage&s,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&u,sizeof u)||overlap(out,n,&b,sizeof b)||overlap(out,n,&s,sizeof s))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();return overlap(out,n,w.original_bytes(),w.original_size())||overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
bool put(int64_t n,int32_t&out){if(n<(-2147483647LL-1)||n>2147483647LL)return false;out=(int32_t)n;return true;}
bool facility(const OriginalBase&b,unsigned id){return (b.facilities_built[id/8]&(1u<<(id%8)))!=0;}
int32_t root(int32_t n){if(n<=1)return n;int32_t next=n>>1,current;do{current=next;next=(next+n/next)>>1;}while(next<current);return current;}
void police(PsychFacts&f,int32_t effect){for(unsigned i=0;i<3;++i)if(effect>f.police_effects[i]){const int32_t old=f.police_effects[i];f.police_effects[i]=effect;effect=old;}}
struct States {uint32_t*p;~States(){free(p);}};
}
PsychError world_base_psych_policy(const OriginalWorld&w,const UnitDefinitions&u,const BasicRules&rules,size_t id,const WorldPsychStage&s,WorldPsychResult&out){
 if(aliases(w,u,rules,s,&out,sizeof out))return PsychError::bad_buffer;
 if(!w.loaded()||id>511)return PsychError::bad_state;
 const auto*base=w.base(id);if(!base||base->faction_id<1||base->faction_id>7||base->pop_size<0||s.specialists<0||s.specialists>base->pop_size||s.fresh_psych<0)return PsychError::bad_state;
 const auto&g=w.game();if(g.data_version>=11||!(g.preferences.FactionStatus[0]&(1u<<(unsigned)base->faction_id)))return PsychError::unsupported;
 const auto*owner=w.faction(base->faction_id);const auto*definition=w.faction_definition(base->faction_id);if(!owner||!definition||definition->faction_bonus_count<0||definition->faction_bonus_count>8||g.preferences.DiffLevel<0||g.preferences.DiffLevel>5||owner->diff_level<0||owner->diff_level>5)return PsychError::bad_state;
 WorldPsychResult next{};auto&f=next.facts;f.population=base->pop_size;f.specialists=s.specialists;f.base_index=(int32_t)id;f.content_population=6-g.preferences.DiffLevel;f.difficulty=owner->diff_level;f.assimilation_turns=base->assimilation_turns_left;
 for(size_t i=0;i<g.base_count;++i){const auto*b=w.base(i);if(!b||b->faction_id<0||b->faction_id>7)return PsychError::bad_state;if(b->faction_id==base->faction_id)++f.faction_base_count;}
 int32_t efficiency,product;
 if(!put(4LL+owner->SE_effic_pending,efficiency))return PsychError::overflow;
 if(efficiency<4)efficiency=4;
 if(!put((int64_t)(f.content_population+2)*efficiency,product)||!put((int64_t)product*root((int32_t)g.map.info.tile_count),product))return PsychError::overflow;
 f.base_limit=(product/56)/2;f.rule_drone=definition->rule_drone;f.rule_talent=definition->rule_talent;
 for(int32_t i=0;i<definition->faction_bonus_count;++i)if(definition->faction_bonus_id[i]==14){f.no_drone_bonus=definition->faction_bonus_val1[i];break;}
 f.pending_talent=owner->SE_talent_pending;f.pending_police=owner->SE_police_pending;f.pending_economics=owner->SE_Economics_pending;f.fresh_psych=s.fresh_psych;f.genejack_drones=rules.drones_genejack_factory;
 f.genejack=facility(*base,22);f.recreation_commons=facility(*base,6);f.hologram_or_virtual=facility(*base,11);f.research_hospital=facility(*base,17);f.nanohospital=facility(*base,18);f.paradise_garden=facility(*base,12);f.nerve_stapled=base->nerve_staple_turns_left!=0;f.punishment_sphere=facility(*base,23);
 bool virtual_world=false,self_aware=false;
 if(world_has_project(w,0,base->faction_id,f.human_genome)!=LogicError::ok||world_has_project(w,6,base->faction_id,virtual_world)!=LogicError::ok||world_has_project(w,7,base->faction_id,f.planetary_transit)!=LogicError::ok||world_has_project(w,14,base->faction_id,f.longevity_vaccine)!=LogicError::ok||world_has_project(w,25,base->faction_id,self_aware)!=LogicError::ok||world_has_project(w,26,base->faction_id,f.clinical_immortality)!=LogicError::ok||world_has_project(w,30,base->faction_id,f.telepathic_matrix)!=LogicError::ok)return PsychError::bad_state;
 f.hologram_or_virtual=f.hologram_or_virtual||(virtual_world&&facility(*base,8));
 const size_t count=g.vehicle_count;States states{count?(uint32_t*)malloc(count*sizeof(uint32_t)):nullptr};if(count&&!states.p)return PsychError::out_of_memory;SupportResult support{};
 const auto support_error=world_base_support_policy(w,u,id,support,states.p,count);if(support_error!=SupportError::ok)return support_error==SupportError::overflow?PsychError::overflow:support_error==SupportError::out_of_memory?PsychError::out_of_memory:support_error==SupportError::unsupported?PsychError::unsupported:PsychError::bad_state;
 f.pacifism_units=support.pacifism_units;
 if(f.pending_police>=-1){const int32_t value=1+(f.pending_police>=3);if(self_aware)police(f,value);
  for(size_t i=0;i<count;++i){const auto*v=w.vehicle(i);if(!v||v->unit_id<0||v->unit_id>=512)return PsychError::bad_state;if(v->x!=base->x||v->y!=base->y)continue;
   const auto*p=w.prototype((size_t)v->unit_id);if(!p||p->chassis_id>=9||p->weapon_id>=u.weapon_count||p->plan<0||p->plan>14)return PsychError::bad_state;
   const int32_t triad=u.chassis[p->chassis_id].triad;if(triad<0||triad>2)return PsychError::bad_state;if(p->plan>3||triad==1)continue;
   const bool native=v->unit_id<64&&u.weapons[p->weapon_id].offense<0;
   police(f,value+((p->ability_flags&0x400000u)!=0)+(((uint32_t)definition->rule_flags&0x40u)&&triad==0&&native));
  }
 }
 const auto error=base_psych_policy(f,next.psych);if(error!=PsychError::ok)return error;out=next;return PsychError::ok;
}
}
