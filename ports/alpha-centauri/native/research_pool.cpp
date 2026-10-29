/* SPDX-License-Identifier: GPL-3.0-or-later
 * Owned original human candidate-pool interpretation. Category semantics from
 * OpenSMACX technology.cpp::tech_category (C)2013-2021 Brendan Casey,
 * pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c. Rotation/ordering checked privately
 * against original decision instructions; no proprietary code copied.
 */
#include "research_pool.h"
namespace alpha {
static uint8_t category(const Technology& t){
 /* Rules priorities retain input order power,tech,wealth,growth. */
 uint8_t result=0;int32_t value=t.priorities[3];
 if(t.priorities[1]>value){result=1;value=t.priorities[1];}
 if(t.priorities[2]>value){result=2;value=t.priorities[2];}
 return t.priorities[0]>value?3:result;
}
LogicError human_research_pool(const Rules& rules,const ResearchOptions& options,const HumanResearchContext& c,HumanResearchPool& output){
 if(rules.count!=technology_count || c.faction<0 || c.faction>=8 || c.turn<0)return LogicError::bad_state;
 for(size_t id=0;id<technology_count;++id)if((unsigned)options.technologies[id].availability>(unsigned)ResearchAvailability::planet)return LogicError::bad_state;
 HumanResearchPool next{};
 if(!c.human || c.blind_research || c.turn==0){next.path=ResearchPoolPath::automatic_selection_required;output=next;return LogicError::ok;}
 next.path=ResearchPoolPath::directed_human;
 for(unsigned group=0;group<4;++group)for(unsigned id=0;id<technology_count;++id){
  if(options.technologies[id].availability!=ResearchAvailability::available)continue;
  if(next.count){
   /* Match original ID-ranking then +faction operation order. */
   int64_t n=(int64_t)id-c.ranking;if(n<(-2147483647LL-1) || n>2147483647LL)return LogicError::overflow;
   n+=c.faction;if(n<(-2147483647LL-1) || n>2147483647LL)return LogicError::overflow;
   if(n%3==0 && (id>=7 || c.ranking!=0))continue;
  }
  if(category(rules.technologies[id])!=group)continue;
  next.ids[next.count]=(uint16_t)id;next.categories[next.count]=(uint8_t)group;++next.count;next.offered[id]=true;
 }
 output=next;return LogicError::ok;
}
LogicError world_human_research_pool(const OriginalWorld& world,const Rules& rules,Variant variant,int32_t faction,HumanResearchPool& output){
 if(!world.loaded() || faction<0 || faction>=8)return LogicError::bad_state;
 ResearchOptions options;auto error=world_research_options(world,rules,variant,faction,options);if(error!=LogicError::ok)return error;
 const auto* saved=world.faction((size_t)faction);if(!saved)return LogicError::bad_state;
 HumanResearchContext context{faction,saved->tech_ranking,world.game().preferences.CurrentTurn,
   (world.game().preferences.FactionStatus[0]&(1u<<faction))!=0,(world.game().preferences.GameRules&0x200)!=0};
 return human_research_pool(rules,options,context,output);
}
}
