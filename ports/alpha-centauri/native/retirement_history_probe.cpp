/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "retirement_history.h"
#include <stdio.h>
#include <string.h>
static uint32_t get(const unsigned char* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int32_t sign(uint32_t n){int32_t v;memcpy(&v,&n,4);return v;}
static bool emit(uint32_t n){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
static bool guards(){
 struct Storage{alpha::RetirementHistoryResult r;uint8_t extra[alpha::retirement_history_bytes];} s;memset(&s,0x5a,sizeof s);alpha::OriginalPrototype d{};
 for(unsigned mode=0;mode<4;++mode){const uint8_t* p=mode==0?nullptr:mode==1?(const uint8_t*)&s:mode==2?(const uint8_t*)&s+sizeof s.r-1:(const uint8_t*)(UINTPTR_MAX-3);
  if(alpha::retirement_history_stage(p,4096,1,0,d,s.r)!=alpha::LogicError::bad_state)return false;
  const unsigned char* q=(const unsigned char*)&s.r;for(size_t i=0;i<sizeof s.r;++i)if(q[i]!=0x5a)return false;}
 uint8_t table[4096]{};if(alpha::retirement_history_stage(table,4096,1,0,*(const alpha::OriginalPrototype*)&s,s.r)!=alpha::LogicError::bad_state)return false;
 const unsigned char* q=(const unsigned char*)&s.r;for(size_t i=0;i<sizeof s.r;++i)if(q[i]!=0x5a)return false;return true;
}
int main(int argc,char** argv){if(argc!=2 || !guards())return 20;FILE* f=fopen(argv[1],"rb");if(!f)return 20;unsigned char b[20+4096];
 for(;;){size_t n=fread(b,1,sizeof b,f);if(!n){bool ok=!ferror(f);fclose(f);return ok?0:20;}if(n!=sizeof b)return 20;alpha::OriginalPrototype d{};d.ability_flags=get(b+12);d.chassis_id=b[16];d.weapon_id=b[17];d.armor_id=b[18];d.reactor_id=b[19];alpha::RetirementHistoryResult r;memset(&r,0x5a,sizeof r);auto err=alpha::retirement_history_stage(b+20,get(b+8),sign(get(b)),sign(get(b+4)),d,r);
  if(err!=alpha::LogicError::ok){const unsigned char* q=(const unsigned char*)&r;for(size_t i=0;i<sizeof r;++i)if(q[i]!=0x5a)return 21;}
  if(!emit((uint32_t)err) || !emit((uint32_t)r.next_slot) || fwrite(r.table,1,4096,stdout)!=4096)return 20;
 }
}
