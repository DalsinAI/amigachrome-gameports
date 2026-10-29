/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "social_upkeep.h"
#include <stdio.h>
#include <string.h>
static uint32_t get(const uint8_t*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int32_t signed_word(uint32_t n){int32_t v;memcpy(&v,&n,4);return v;}
static bool put(uint32_t n){uint8_t b[4];for(auto& v:b){v=(uint8_t)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
int main(int argc,char**argv){
 if(argc!=2)return 20;
 FILE*f=fopen(argv[1],"rb");if(!f)return 20;
 static alpha::SocialRules rules;
 alpha::SocialUpkeepFacts alias_facts{};
 const void* aliases[]={&rules,(const uint8_t*)&rules+4,&alias_facts,(const uint8_t*)&alias_facts+4};
 for(const void* at:aliases){auto& result=*reinterpret_cast<alpha::SocialUpkeepResult*>(const_cast<void*>(at));
  if(alpha::social_upkeep(rules,alias_facts,result)!=alpha::LogicError::bad_state)return 21;
 }
 uint8_t bytes[876];unsigned count=0;
 for(;;){size_t n=fread(bytes,1,sizeof bytes,f);if(!n){bool ok=!ferror(f);fclose(f);return ok?0:20;}if(n!=sizeof bytes || ++count>200000)return 20;
  unsigned at=0;auto u=[&](){uint32_t v=get(bytes+4*at);++at;return v;};auto s=[&](){return signed_word(u());};
  const uint32_t variant=u();alpha::SocialUpkeepFacts in{};in.variant=(alpha::Variant)variant;
  for(auto& v:in.selected)v=u();
  for(auto& v:in.base_effects)v=s();
  in.bonus_count=s();for(auto& v:in.bonus_kind)v=s();for(auto& v:in.bonus_first)v=s();for(auto& v:in.bonus_second)v=s();in.rule_flags=u();const uint32_t flags=u();
  in.network_backbone=flags&1;in.cloning_vats=flags&2;in.ascetic_virtues=flags&4;in.living_refinery=flags&8;in.temple=flags&16;
  for(auto& cat:rules.models)for(auto& model:cat)for(auto& value:model.effects)value=s();
  if(at!=219)return 21;
  alpha::SocialUpkeepResult result;memset(&result,90,sizeof result);const auto error=variant>1?alpha::LogicError::bad_state:alpha::social_upkeep(rules,in,result);
  if(error!=alpha::LogicError::ok){const auto* p=(const uint8_t*)&result;for(size_t i=0;i<sizeof result;++i)if(p[i]!=90)return 21;}
  if(!put((uint32_t)error))return 20;
  for(auto v:result.current)if(!put(v))return 20;
  for(auto v:result.pending)if(!put((uint32_t)v))return 20;
  for(auto v:result.active)if(!put((uint32_t)v))return 20;
  for(auto v:result.temporary)if(!put((uint32_t)v))return 20;
  if(!put((uint32_t)result.upheaval_cost_paid))return 20;
 }
}
