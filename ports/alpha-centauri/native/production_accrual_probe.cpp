/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "production_accrual.h"
#include <stdio.h>
#include <string.h>
static uint32_t word(const unsigned char* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int32_t signed_word(const unsigned char* p){const uint32_t n=word(p);int32_t s;memcpy(&s,&n,4);return s;}
static bool emit(uint32_t v){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)v;v>>=8;}return fwrite(b,1,4,stdout)==4;}
int main(int argc,char** argv){if(argc!=2)return 20;FILE* f=fopen(argv[1],"rb");if(!f)return 20;unsigned char record[32];size_t count=0;
 for(;;){size_t n=fread(record,1,sizeof record,f);if(!n){bool ok=!ferror(f);fclose(f);return ok?0:20;}if(n!=sizeof record || ++count>200000){fclose(f);return 20;}
  alpha::ProductionAccrualResult r{0x31415926u,0x31415926,0x31415926,true,true};alpha::LogicError error=alpha::LogicError::bad_state;const int32_t profile=signed_word(record);
  if(profile==0 || profile==1){const alpha::ProductionAccrualInput in{profile?alpha::Variant::alien_crossfire:alpha::Variant::base,word(record+4),signed_word(record+8),signed_word(record+12),signed_word(record+16),signed_word(record+20),signed_word(record+24),signed_word(record+28)};error=alpha::production_accrual(in,r);}
  if(error!=alpha::LogicError::ok && (r.state_flags!=0x31415926u || r.best_mineral_output!=0x31415926 || r.minerals_accumulated!=0x31415926 || !r.negative_food_abort || !r.accrual_branch)){fclose(f);return 21;}
  const uint32_t values[]={(uint32_t)error,r.state_flags,(uint32_t)r.best_mineral_output,(uint32_t)r.minerals_accumulated,(uint32_t)r.negative_food_abort,(uint32_t)r.accrual_branch};for(uint32_t v:values)if(!emit(v)){fclose(f);return 20;}
 }
}
