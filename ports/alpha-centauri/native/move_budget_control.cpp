/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "move_budget.h"
#include <stdio.h>
#include <initializer_list>
static void word(uint32_t& hash,uint32_t n){for(unsigned i=0;i<32;i+=8)hash=(hash^((n>>i)&255u))*16777619u;}
int main() {
  uint32_t hash=2166136261u,count=0;
  for(uint32_t speed:{0u,1u,2u,3u,6u,255u})
  for(uint32_t spent:{0u,1u,2u,3u,6u,255u})
  for(uint32_t cost:{0u,1u,2u,3u,6u,9u,100u,255u,500u})
  for(uint32_t bits=0;bits<512;++bits) {
    alpha::MoveBudget before{speed,spent,0x40u|0x100u|((bits&1u)?0x8000u:0u),bits*0x1234567u};
    alpha::MoveBudgetInput input{cost,bits&2u?100u:3u,(bits&4u)!=0,(bits&8u)!=0,
      (bits&16u)!=0,(bits&32u)!=0,(bits&64u)!=0,(bits&128u)!=0,(bits&256u)!=0,true};
    for(unsigned execute=0;execute<2;++execute){input.execute=execute!=0;
      alpha::MoveBudgetResult out{{123,124,125,126},alpha::MoveBudgetOutcome::preview,true,127};
      const auto error=alpha::resolve_move_budget(before,input,out);
      word(hash,(uint32_t)error);word(hash,out.budget.speed);word(hash,out.budget.spent);
      word(hash,out.budget.state);word(hash,out.budget.random_seed);word(hash,(uint32_t)out.outcome);
      word(hash,out.drew_random);word(hash,out.random_value);++count;
    }
  }
  uint32_t seed=0x12345678u;
  for(unsigned i=0;i<6;++i)for(auto bounds:{0u,10u,20u,7u,0xffffffffu}){
    uint32_t value=alpha::integer_random(seed,bounds,bounds==0xffffffffu?0u:bounds+100u);
    word(hash,value);word(hash,seed);
  }
  const alpha::MoveBudget good{3,0,0,1};
  const alpha::MoveBudgetInput input{1,3,true,false,false,false,false,true,true,true};
  unsigned invalid=0;
  for(unsigned which=0;which<5;++which){auto before=good;auto in=input;
    if(which==0)before.speed=256;
    if(which==1)before.spent=256;
    if(which==2)in.cost=501;
    if(which==3)in.road_rate=0;
    if(which==4)in.road_rate=101;
    alpha::MoveBudgetResult out{{123,124,125,126},alpha::MoveBudgetOutcome::preview,true,127};
    if(alpha::resolve_move_budget(before,in,out)!=alpha::LogicError::bad_state ||
      out.budget.speed!=123 || out.budget.spent!=124 || out.budget.state!=125 ||
      out.budget.random_seed!=126 || out.outcome!=alpha::MoveBudgetOutcome::preview ||
      !out.drew_random || out.random_value!=127)return 1;
    ++invalid;
  }
#ifndef ALPHA_CONTROL_RAW
  printf("budget=%u rng=30 invalid=%u digest=%08x rc=0\n",count,invalid,hash);
#endif
  return hash==0xddc41e3fu && count==331776 && invalid==5?0:2;
}
