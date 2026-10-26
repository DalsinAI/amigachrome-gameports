/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "production_loans.h"
#include <stdio.h>
#include <string.h>
static uint32_t get(const uint8_t*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int32_t sint(uint32_t v){int32_t n;memcpy(&n,&v,4);return n;}
static bool put(uint32_t v){uint8_t b[4];
  for(auto&x:b){x=(uint8_t)v;v>>=8;}return fwrite(b,1,4,stdout)==4;}
int main(int argc,char**argv){
 if(argc!=2)return 20;
 FILE*f=fopen(argv[1],"rb");if(!f)return 20;
 alignas(alpha::ProductionLoanResult) uint8_t storage[sizeof(alpha::ProductionLoanFacts)+sizeof(alpha::ProductionLoanResult)]{};
 for(unsigned off=0;off<=4;off+=4)if(alpha::production_loans(*reinterpret_cast<alpha::ProductionLoanFacts*>(storage),*reinterpret_cast<alpha::ProductionLoanResult*>(storage+off))!=alpha::LogicError::bad_state)return 21;
 uint8_t b[168];unsigned count=0;
 for(;;){const size_t n=fread(b,1,sizeof b,f);if(!n){bool ok=!ferror(f);fclose(f);return ok?0:20;}if(n!=sizeof b||++count>200000)return 20;
  alpha::ProductionLoanFacts in{};in.faction=sint(get(b));in.alive_mask=get(b+4);unsigned at=2;
  for(auto&v:in.credits)v=sint(get(b+4*at++));
  for(auto&v:in.balance)v=sint(get(b+4*at++));
  for(auto&v:in.payment)v=sint(get(b+4*at++));
  for(auto&v:in.diplomacy)v=get(b+4*at++);
  for(auto&v:in.sanctions)v=sint(get(b+4*at++));
  alpha::ProductionLoanResult result;memset(&result,90,sizeof result);auto error=alpha::production_loans(in,result);
  if(error!=alpha::LogicError::ok){const auto*p=(const uint8_t*)&result;
  for(size_t i=0;i<sizeof result;++i)if(p[i]!=90)return 21;}
  if(!put((uint32_t)error))return 20;
  for(auto v:result.credits)if(!put((uint32_t)v))return 20;
  for(auto v:result.balance)if(!put((uint32_t)v))return 20;
  if(!put((uint32_t)result.energy_before_bases))return 20;
 }
}
