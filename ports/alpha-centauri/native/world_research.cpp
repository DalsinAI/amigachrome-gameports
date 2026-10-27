/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in research_progress.h. */
#include "world_research.h"
#include "world_unit_stats.h"
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){if(!a||!b||!an||!bn)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
bool aliases(const OriginalWorld&w,const BasicRules&b,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&b,sizeof b))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();return overlap(out,n,w.original_bytes(),w.original_size())||overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
bool put(int64_t n,int32_t&out){if(n<(-2147483647LL-1)||n>2147483647LL)return false;out=(int32_t)n;return true;}
bool add(int32_t&out,int32_t n){return put((int64_t)out+n,out);}
ResearchError overlay_effects(const OriginalWorld&w,const Rules&t,int faction,int target,TechnologyEffects&out){
 const auto*f=w.faction((size_t)faction);const auto*d=w.faction_definition((size_t)faction);if(!f||!d||d->faction_bonus_count<0||d->faction_bonus_count>8)return ResearchError::bad_state;
 TechnologyEffects next{};next.commerce=d->rule_commerce;
 for(int i=0;i<89;++i){bool known=false;if(world_has_technology(w,t,i,faction,known)!=LogicError::ok)return ResearchError::bad_state;if(i==target)known=true;if(!known)continue;
  const uint32_t flags=t.technologies[i].flags;if((flags&256u)&&!add(next.fungus[0],1))return ResearchError::overflow;if((flags&128u)&&!add(next.fungus[1],1))return ResearchError::overflow;if((flags&64u)&&!add(next.fungus[2],1))return ResearchError::overflow;if((flags&4u)&&!add(next.commerce,1))return ResearchError::overflow;
 }
 for(int i=0;i<d->faction_bonus_count;++i){const int id=d->faction_bonus_id[i];if(id>=7&&id<=9&&!add(next.fungus[id-7],d->faction_bonus_val1[i]))return ResearchError::overflow;}
 for(auto&value:next.fungus)if(value<0)value=0;
 if(!add(next.commerce,(f->SE_economy_pending>2)+(f->SE_economy_pending>3)+(f->SE_economy_pending>4)))return ResearchError::overflow;
 out=next;return ResearchError::ok;
}
}
ResearchError world_original_research_cost(const OriginalWorld&w,const BasicRules&b,int32_t faction,WorldResearchCost&out){
 if(aliases(w,b,&out,sizeof out))return ResearchError::bad_buffer;
 if(!w.loaded()||faction<1||faction>7)return ResearchError::bad_state;
 if(w.game().data_version>=11||!(w.game().preferences.FactionStatus[0]&(1u<<(unsigned)faction)))return ResearchError::unsupported;
 const auto*f=w.faction((size_t)faction);const auto*d=w.faction_definition((size_t)faction);if(!f||!d)return ResearchError::bad_state;
 WorldResearchCost next{};auto&facts=next.facts;facts.faction=faction;facts.difficulty=f->diff_level;facts.current_turn=w.game().preferences.CurrentTurn;facts.discovery_rate_pct=b.tech_discovery_rate_pct_std;facts.rule_techcost=d->rule_techcost;facts.research_base=f->SE_research_base;facts.map_tiles=(int32_t)w.game().map.info.tile_count;facts.ranking_debt=f->unk_27;facts.stagnation=((uint32_t)w.game().preferences.GameRules&0x20u)!=0;
 for(unsigned i=0;i<8;++i){const auto*other=w.faction(i);if(!other)return ResearchError::bad_state;facts.ranking[i]=other->tech_ranking;facts.deferred_techs[i]=other->earned_techs_saved;}
 const auto error=original_research_cost(facts,next.result);if(error!=ResearchError::ok)return error;out=next;return ResearchError::ok;
}
ResearchError world_base_research(const OriginalWorld&w,const Rules&t,const BasicRules&b,size_t id,const WorldResearchStage&s,WorldResearchResult&out){
 if(aliases(w,b,&out,sizeof out)||overlap(&out,sizeof out,&t,sizeof t)||overlap(&out,sizeof out,&s,sizeof s))return ResearchError::bad_buffer;
 if(!w.loaded()||t.count!=89)return ResearchError::bad_state;
 const auto*base=w.base(id);if(!base)return ResearchError::bad_state;const int faction=base->faction_id;
 if(faction<1||faction>7||faction!=w.game().player||faction!=w.game().preferences.CurrentFaction||w.game().data_version>=11||!(w.game().preferences.FactionStatus[0]&(1u<<(unsigned)faction)))return ResearchError::unsupported;
 const auto*f=w.faction((size_t)faction);if(!f)return ResearchError::bad_state;
 WorldResearchResult next{};next.target=f->tech_research_id;next.target_after=next.target;next.ranking_after=f->tech_ranking;next.ranking_debt_after=f->unk_27;next.achieved_after=f->tech_achieved;
 const uint32_t rules=(uint32_t)w.game().preferences.GameRules;next.blind_research=(rules&0x200u)!=0;
 ResearchContributionFacts facts{s.fresh_labs,f->SE_research_pending,w.game().preferences.CurrentTurn,f->diff_level,0,s.before,(rules&0x4000000u)!=0,((uint32_t)w.game().preferences.GamePreferences&0x20u)!=0,(base->facilities_built[23/8]&(1u<<(23%8)))!=0};
 auto error=research_labs(facts,next.contribution);if(error!=ResearchError::ok)return error;
 if(!next.contribution.research_points){out=next;return ResearchError::ok;}
 if(next.target<0)return ResearchError::needs_target;
 if(next.target>=78&&next.target<=86)return ResearchError::unsupported;
 if(next.target>=88)return ResearchError::unsupported;
 bool owned=false;if(world_has_technology(w,t,next.target,faction,owned)!=LogicError::ok)return ResearchError::bad_state;if(owned)return ResearchError::bad_state;
 const auto&target=t.technologies[next.target];for(unsigned j=0;j<2;++j){const int32_t pre=target.prerequisites[j];if(pre< -1||pre>=89)return ResearchError::bad_state;bool known=false;if(world_has_technology(w,t,pre,faction,known)!=LogicError::ok||!known)return ResearchError::bad_state;}
 error=world_original_research_cost(w,b,faction,next.cost);if(error!=ResearchError::ok)return error;next.cost_computed=true;facts.fresh_cost=next.cost.result.cost;
 error=research_contribution(facts,next.contribution);if(error!=ResearchError::ok)return error;
 next.owner_mask_after=w.game().preferences.TechOwners[next.target];
 const bool pending_secret=(f->player_flags&0x40u)!=0;
 if(next.contribution.completion||pending_secret){
  if(next.achieved_after<0)return ResearchError::bad_state;
  next.target_after=-1;next.acquisition_required=true;next.next_choice_required=true;next.first_secrets_required=!next.owner_mask_after&&(target.flags&1u);next.next_selection_automatic=next.blind_research;next.bonus_current_target=pending_secret&&!next.contribution.completion;next.bonus_next_target_required=next.contribution.completion&&(pending_secret||next.first_secrets_required);next.owner_mask_after|=1u<<(unsigned)faction;
  if(!add(next.achieved_after,1)||!add(next.ranking_after,2))return ResearchError::overflow;
  if(next.ranking_debt_after<0)return ResearchError::bad_state;
  next.ranking_debt_after=next.ranking_debt_after>=2?next.ranking_debt_after-2:0;
  error=overlay_effects(w,t,faction,next.target,next.effects_after);if(error!=ResearchError::ok)return error;
 }
 out=next;return ResearchError::ok;
}
}
