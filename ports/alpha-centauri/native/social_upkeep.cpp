/* SPDX-License-Identifier: GPL-3.0-or-later. See social_upkeep.h. */
#include "social_upkeep.h"
#include "technology_effects_ranges.h"
namespace alpha { namespace {
bool add(int64_t& n,int64_t value){n+=value;return n>=INT32_MIN && n<=INT32_MAX;}
}
LogicError social_upkeep(const SocialRules& rules,const SocialUpkeepFacts& in,SocialUpkeepResult& out){
 if(detail::effects_range_overlap(&out,sizeof out,&rules,sizeof rules) ||
    detail::effects_range_overlap(&out,sizeof out,&in,sizeof in))return LogicError::bad_state;
 if((in.variant!=Variant::base && in.variant!=Variant::alien_crossfire) || in.bonus_count<0 || in.bonus_count>8)return LogicError::bad_state;
 for(auto selected:in.selected)if(selected>=4)return LogicError::bad_state;
 for(int i=0;i<in.bonus_count;++i){
  const int k=in.bonus_kind[i],first=in.bonus_first[i],second=in.bonus_second[i];
  if(k<0 || k>18 || ((k==4 || k==10) && (first<0 || first>=11)) ||
     ((k==5 || k==6) && (first<0 || first>=4 || second<0 || second>=4)))return LogicError::bad_state;
 }
 int64_t values[11]={};
 for(unsigned cat=0;cat<4;++cat)for(unsigned eff=0;eff<11;++eff){
  const unsigned model=in.selected[cat];const int64_t value=rules.models[cat][model].effects[eff];
  if(value<0){
   if((cat==3 && ((model==1 && in.network_backbone) || (model==3 && in.cloning_vats))) ||
      (cat==2 && model==1 && in.cloning_vats))continue;
   for(int i=0;i<in.bonus_count;++i)if(in.bonus_first[i]==(int)cat && in.bonus_second[i]==(int)model){
    if(in.bonus_kind[i]==5 && !add(values[eff],-value))return LogicError::overflow;
    if(in.bonus_kind[i]==6 && !add(values[eff],value))return LogicError::overflow;
   }
  }
  if(!add(values[eff],value))return LogicError::overflow;
 }
 if(in.ascetic_virtues && !add(values[5],1))return LogicError::overflow;
 if(in.living_refinery && !add(values[2],2))return LogicError::overflow;
 if(in.temple){
  if(!add(values[7],1))return LogicError::overflow;
  if(in.variant==Variant::alien_crossfire && (in.rule_flags&0x80u) && !add(values[10],1))return LogicError::overflow;
 }
 for(unsigned i=0;i<11;++i)if(!add(values[i],in.base_effects[i]))return LogicError::overflow;
 for(int i=0;i<in.bonus_count;++i){
  if(in.bonus_kind[i]==4){auto& n=values[in.bonus_first[i]];n=n<0?0:n>999?999:n;}
  else if(in.bonus_kind[i]==10){auto& n=values[in.bonus_first[i]];if(n<0)n/=2;}
 }
 SocialUpkeepResult next{};
 for(unsigned i=0;i<4;++i)next.current[i]=in.selected[i];
 for(unsigned i=0;i<11;++i)next.pending[i]=next.active[i]=next.temporary[i]=(int32_t)values[i];
 out=next;return LogicError::ok;
}
}
