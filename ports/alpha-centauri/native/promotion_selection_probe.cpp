/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "promotion_selection.h"
#include <stdio.h>
#include <string.h>
#ifdef __SANITIZE_ADDRESS__
#include <sanitizer/asan_interface.h>
#endif
using namespace alpha;
static uint32_t load(const unsigned char*p){return(uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int32_t signed32(uint32_t v){return v<=0x7fffffffu?(int32_t)v:-1-(int32_t)(~v);}
static bool word(uint32_t v){for(unsigned j=0;j<4;++j){if(fputc((int)(v&255u),stdout)==EOF)return false;v>>=8;}return true;}
template<class T>static T&address_ref(uintptr_t p){return *reinterpret_cast<T*>(p);}
int main(int argc,char**argv){if(argc!=2)return 20;FILE*f=fopen(argv[1],"rb");if(!f)return 20;unsigned char b[52];
while(fread(b,1,52,f)==52){alignas(PromotionSelectionResult) PromotionSelectionContext c{SelectionObservation::supplied,(SelectionOutcome)load(b),{load(b+4),load(b+8),load(b+12)},{load(b+16),load(b+20),load(b+24)},signed32(load(b+28)),signed32(load(b+32)),load(b+36),load(b+40),load(b+44),signed32(load(b+48))>=0};PromotionSelectionResult r{PromotionIntent::none,0xa5a5a5a5u,0x31415926,0x31415926};const auto before=r;const auto status=inspect_promotion_selection(c,r);
 if(status!=LogicError::ok&&memcmp(&r,&before,sizeof r))return 21;
 if(!word((uint32_t)status)||!word((uint32_t)r.intent)||!word(r.recipient)||!word((uint32_t)r.attacker_strength)||!word((uint32_t)r.defender_strength))return 22;
 const auto observed_result=r;c.observation=SelectionObservation::missing;
 if(inspect_promotion_selection(c,r)!=LogicError::bad_state||memcmp(&r,&observed_result,sizeof r))return 25;
 c.observation=SelectionObservation::supplied;
 const auto original=c;
#ifdef __SANITIZE_ADDRESS__
 __asan_poison_memory_region(&c,sizeof c);
#endif
 const auto alias=inspect_promotion_selection(c,address_ref<PromotionSelectionResult>((uintptr_t)&c));
#ifdef __SANITIZE_ADDRESS__
 __asan_unpoison_memory_region(&c,sizeof c);
#endif
 if(alias!=LogicError::bad_state||memcmp(&c,&original,sizeof c))return 23;
}
return ferror(f)||ferror(stdout)?24:0;}
