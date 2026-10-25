/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include "game_logic.h"
#include "upstream_logic_reference.h"
namespace logic_control {
static void word(uint32_t& h, int32_t value) {
  for (unsigned shift=0; shift<32; shift+=8) h=(h^(((uint32_t)value>>shift)&255u))*16777619u;
}
int social_case(const alpha::SocialRules& rules, const alpha::Faction& faction,
                const alpha::SocialContext& context, uint32_t& digest) {
  using namespace source_reference;
  for (size_t c=0;c<4;++c) for (size_t m=0;m<4;++m) for (size_t e=0;e<11;++e)
    SocialCategories[c].model_effect[m].values[e]=rules.models[c][m].effects[e];
  Players[1].faction_bonus_count=(int32_t)faction.bonus_count;
  for (size_t i=0;i<faction.bonus_count;++i) {
    Players[1].faction_bonus_id[i]=(int32_t)faction.bonuses[i].kind;
    Players[1].faction_bonus_val1[i]=faction.bonuses[i].value;
    Players[1].faction_bonus_val2[i]=faction.bonuses[i].modifier;
  }
  projects=(context.network_backbone?1u:0u)|(context.cloning_vats?2u:0u)|
    (context.ascetic_virtues?4u:0u)|(context.living_refinery?8u:0u);
  temple=context.temple;alien=context.expansion && (faction.rule_flags&0x80u);
  SocialCategory category;
  for (size_t c=0;c<4;++c) category.choice[c]=context.selected[c];
  compute_faction_modifiers(1);
  SocialEffect expected;
  social_calc(&category,&expected,1,false,context.quick);
  alpha::SocialResult actual;
  if (alpha::calculate_social(rules,faction,context,actual)!=alpha::LogicError::ok) return 1;
  for (size_t i=0;i<11;++i) {
    if (actual.effects[i]!=expected.values[i]) return 2;
    word(digest,actual.effects[i]);
  }
  return 0;
}
int nutrient_case(const alpha::NutrientInput& input,uint32_t& digest) {
  using namespace source_reference;
  base.faction_id_current=1;base.state=input.golden_age?1u:0u;
  base.nutrient_intake_2=input.intake;base.nutrients_accumulated=input.accumulated;
  base.population_size=input.population;basic.nutrient_req_citizen=input.nutrient_per_citizen;
  BaseCurrentConvoyTo[0]=input.convoy_to;BaseCurrentConvoyFrom[0]=input.convoy_from;
  PlayersData[1].soc_effect_pending.values[6]=input.pending_growth;
  PlayersData[1].nutrient_surplus_total=input.faction_surplus_total;
  upkeep=input.upkeep_stage;creche=input.children_creche;
  base_nutrient();
  alpha::NutrientResult actual;
  if (alpha::calculate_nutrients(input,actual)!=alpha::LogicError::ok) return 3;
  const int32_t expected[]={growth,base.nutrient_intake_2,base.nutrient_consumption,
    base.nutrient_surplus,base.nutrients_accumulated,PlayersData[1].nutrient_surplus_total};
  const int32_t values[]={actual.growth,actual.intake,actual.consumption,actual.surplus,
    actual.accumulated,actual.faction_surplus_total};
  for (size_t i=0;i<6;++i) {
    if (values[i]!=expected[i]) return 4;
    word(digest,values[i]);
  }
  return 0;
}
static bool untouched(const alpha::SocialResult& out) {
  for (int32_t value:out.effects) if(value!=12345) return false;
  return true;
}
static bool untouched(const alpha::NutrientResult& out) {
  return out.growth==12345 && out.intake==12345 && out.consumption==12345 &&
    out.surplus==12345 && out.accumulated==12345 && out.faction_surplus_total==12345;
}
int synthetic(uint32_t& digest) {
  static alpha::SocialRules rules;
  for (size_t c=0;c<4;++c) for (size_t m=0;m<4;++m) for (size_t e=0;e<11;++e)
    rules.models[c][m].effects[e]=(int32_t)((c*11+m*3+e)%9)-4;
  alpha::Faction faction{};
  faction.rule_flags=0x80u;faction.bonus_count=8;
  faction.bonuses[0]={alpha::FactionBonusKind::social,6,2};
  faction.bonuses[1]={alpha::FactionBonusKind::social,0,-1};
  faction.bonuses[2]={alpha::FactionBonusKind::robust,2,1};
  faction.bonuses[3]={alpha::FactionBonusKind::immunity,1,1};
  faction.bonuses[4]={alpha::FactionBonusKind::impunity,0,2};
  faction.bonuses[5]={alpha::FactionBonusKind::penalty,1,3};
  faction.bonuses[6]={alpha::FactionBonusKind::robust,7,1};
  faction.bonuses[7]={alpha::FactionBonusKind::social,10,-3};
  for (uint32_t choice=0;choice<256;++choice) for (uint32_t flags=0;flags<32;++flags) for(unsigned quick=0;quick<2;++quick) {
    alpha::SocialContext ctx{};
    for(size_t c=0;c<4;++c) ctx.selected[c]=(choice>>(c*2))&3u;
    ctx.network_backbone=flags&1u;ctx.cloning_vats=flags&2u;ctx.ascetic_virtues=flags&4u;
    ctx.living_refinery=flags&8u;ctx.temple=flags&16u;ctx.expansion=(choice&1u);ctx.quick=quick;
    int rc=social_case(rules,faction,ctx,digest);if(rc) return rc;
  }
  uint32_t seed=0x12345678u;
  for(unsigned i=0;i<2000;++i) {
    seed=seed*1664525u+1013904223u;
    alpha::NutrientInput input{(int32_t)(seed%10),2,(int32_t)(seed%31),(int32_t)(seed%20)-5,
      (int32_t)(seed%5),(int32_t)(seed%3),(int32_t)(seed%7)-3,(int32_t)(seed%3),
      (int32_t)(seed%101),(seed&32u)!=0,(seed&64u)!=0};
    int rc=nutrient_case(input,digest);if(rc) return rc;
  }
  alpha::NutrientInput high{2,2,200,0,3,1,1,1,100,true,true};
  int rc=nutrient_case(high,digest);if(rc) return rc;
  // Bad state/overflow must leave the supplied result unchanged.
  alpha::SocialResult social_out;
  for(auto& v:social_out.effects)v=12345;
  alpha::SocialContext ctx{};
  ctx.selected[0]=4;
  if(alpha::calculate_social(rules,faction,ctx,social_out)!=alpha::LogicError::bad_state || !untouched(social_out)) return 5;
  ctx.selected[0]=0;faction.bonus_count=9;
  if(alpha::calculate_social(rules,faction,ctx,social_out)!=alpha::LogicError::bad_state || !untouched(social_out)) return 6;
  faction.bonus_count=1;faction.bonuses[0]={alpha::FactionBonusKind::social,11,1};
  if(alpha::calculate_social(rules,faction,ctx,social_out)!=alpha::LogicError::bad_state || !untouched(social_out)) return 7;
  faction.bonuses[0]={alpha::FactionBonusKind::impunity,4,0};
  if(alpha::calculate_social(rules,faction,ctx,social_out)!=alpha::LogicError::bad_state || !untouched(social_out)) return 8;
  faction.bonuses[0]={(alpha::FactionBonusKind)99,0,0};
  if(alpha::calculate_social(rules,faction,ctx,social_out)!=alpha::LogicError::bad_state || !untouched(social_out)) return 9;
  faction.bonus_count=0;
  for(size_t c=0;c<4;++c)for(size_t m=0;m<4;++m)for(size_t e=0;e<11;++e)rules.models[c][m].effects[e]=0;
  rules.models[0][0].effects[0]=2147483647;rules.models[1][0].effects[0]=1;
  if(alpha::calculate_social(rules,faction,ctx,social_out)!=alpha::LogicError::overflow || !untouched(social_out)) return 10;
  // Immunity may not conceal an earlier original-domain signed overflow.
  faction.bonus_count=1;faction.bonuses[0]={alpha::FactionBonusKind::immunity,0,1};
  if(alpha::calculate_social(rules,faction,ctx,social_out)!=alpha::LogicError::overflow || !untouched(social_out)) return 11;
  alpha::NutrientResult nutrient_out{12345,12345,12345,12345,12345,12345};
  alpha::NutrientInput invalid=high;invalid.population=128;
  if(alpha::calculate_nutrients(invalid,nutrient_out)!=alpha::LogicError::bad_state || !untouched(nutrient_out)) return 12;
  invalid=high;invalid.convoy_to=-1;
  if(alpha::calculate_nutrients(invalid,nutrient_out)!=alpha::LogicError::bad_state || !untouched(nutrient_out)) return 13;
  invalid=high;invalid.nutrient_per_citizen=101;
  if(alpha::calculate_nutrients(invalid,nutrient_out)!=alpha::LogicError::bad_state || !untouched(nutrient_out)) return 14;
  invalid=high;invalid.pending_growth=2147483647;
  if(alpha::calculate_nutrients(invalid,nutrient_out)!=alpha::LogicError::overflow || !untouched(nutrient_out)) return 15;
  invalid=high;invalid.intake=2147483647;
  if(alpha::calculate_nutrients(invalid,nutrient_out)!=alpha::LogicError::overflow || !untouched(nutrient_out)) return 16;
  invalid=high;invalid.faction_surplus_total=2147483647;
  if(alpha::calculate_nutrients(invalid,nutrient_out)!=alpha::LogicError::overflow || !untouched(nutrient_out)) return 17;
  return 0;
}
}
#ifndef ALPHA_CONTROL_NO_MAIN
int main() {
  uint32_t digest=2166136261u;
  int rc=logic_control::synthetic(digest);
#ifndef ALPHA_CONTROL_RAW
  printf("social=16384 nutrients=2001 invalid=13 digest=%08lx rc=%d\n",(unsigned long)digest,rc);
#endif
  return rc;
}
#endif
