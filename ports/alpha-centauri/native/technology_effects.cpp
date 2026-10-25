/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in technology_effects.h. */
#include "technology_effects.h"
namespace alpha { namespace {
bool add(int32_t& n,int32_t value){const int64_t result=(int64_t)n+value;if(result<INT32_MIN || result>INT32_MAX)return false;n=(int32_t)result;return true;}
}
LogicError recompute_technology_effects(const Rules& rules,const TechnologyEffectsFacts& facts,TechnologyEffectsResult& output){
 if(rules.count!=technology_count || facts.faction<0 || facts.faction>7 || facts.bonus_count<0 || facts.bonus_count>8)return LogicError::bad_state;
 for(size_t id=0;id<technology_count;++id)for(unsigned p=0;p<2;++p)
  if(rules.technologies[id].prerequisites[p]<-2 || rules.technologies[id].prerequisites[p]>=89)return LogicError::bad_state;
 TechnologyEffectsResult next{facts.rule_commerce,0,0,0,0};
 for(size_t id=0;id<88;++id){const auto& tech=rules.technologies[id];
  if(facts.faction==0 || tech.prerequisites[0]<-1 || (tech.prerequisites[1]<-1 && tech.prerequisites[0]!=-1) || !(facts.owners[id]&(1u<<(unsigned)facts.faction)))continue;
  // The three resource increments are bounded by88 but remain checked along
  // with commerce to document the complete original arithmetic boundary.
  if((tech.flags&0x40u) && !add(next.fungus_energy,1))return LogicError::overflow;
  if((tech.flags&0x80u) && !add(next.fungus_mineral,1))return LogicError::overflow;
  if((tech.flags&0x100u) && !add(next.fungus_nutrient,1))return LogicError::overflow;
  if((tech.flags&4u) && !add(next.commerce,1))return LogicError::overflow;
 }
 for(int32_t i=0;i<facts.bonus_count;++i){
  if(facts.bonus_kind[i]==7 && !add(next.fungus_nutrient,facts.bonus_value[i]))return LogicError::overflow;
  if(facts.bonus_kind[i]==8 && !add(next.fungus_mineral,facts.bonus_value[i]))return LogicError::overflow;
  if(facts.bonus_kind[i]==9 && !add(next.fungus_energy,facts.bonus_value[i]))return LogicError::overflow;
 }
 if(next.fungus_nutrient<0)next.fungus_nutrient=0;
 if(next.fungus_mineral<0)next.fungus_mineral=0;
 if(next.fungus_energy<0)next.fungus_energy=0;
 // Original clamps all four fungus cells, including the unused cell reset0.
 if(next.fungus_unused<0)next.fungus_unused=0;
 const int32_t economy=facts.pending_economy>4?3:facts.pending_economy>3?2:facts.pending_economy>2?1:0;
 if(!add(next.commerce,economy))return LogicError::overflow;
 output=next;return LogicError::ok;
}
LogicError world_technology_effects(const OriginalWorld& world,const Rules& rules,int32_t faction,TechnologyEffectsResult& output){
 if(!world.loaded() || faction<0 || faction>7)return LogicError::bad_state;
 const auto* state=world.faction((size_t)faction);const auto* definition=world.faction_definition((size_t)faction);
 if(!state || !definition)return LogicError::bad_state;
 TechnologyEffectsFacts facts{};facts.faction=faction;facts.rule_commerce=definition->rule_commerce;
 facts.pending_economy=state->SE_economy_pending;facts.bonus_count=definition->faction_bonus_count;
 for(size_t id=0;id<technology_count;++id)facts.owners[id]=world.game().preferences.TechOwners[id];
 for(unsigned i=0;i<8;++i){facts.bonus_kind[i]=definition->faction_bonus_id[i];facts.bonus_value[i]=definition->faction_bonus_val1[i];}
 return recompute_technology_effects(rules,facts,output);
}
}
