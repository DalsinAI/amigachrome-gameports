/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "monument_effects.h"
#include <stdio.h>
#include <string.h>
static uint32_t get(const unsigned char* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int32_t s(uint32_t n){int32_t r;memcpy(&r,&n,4);return r;}
static bool emit(uint32_t n){unsigned char p[4];for(unsigned i=0;i<4;++i){p[i]=(unsigned char)n;n>>=8;}return fwrite(p,1,4,stdout)==4;}
static bool guards(){
 struct Storage {alpha::MonumentEffectResult result;unsigned char tail[alpha::monument_table_bytes];} storage{};
 alpha::MonumentEffectInput in{};in.variant=alpha::Variant::base;in.faction=1;in.local_faction=1;in.event=4;in.size=alpha::monument_table_bytes;
 memset(storage.result.record,0x5a,sizeof storage.result.record);storage.result.changed=true;storage.result.presentation_required=true;
 for(unsigned i=0;i<3;++i){
  in.monuments=i==0?nullptr:i==1?(const uint8_t*)&storage:(const uint8_t*)&storage+sizeof storage.result-1;
  if(alpha::monument_effect(in,storage.result)!=alpha::LogicError::bad_state)return false;
  for(auto c:storage.result.record)if(c!=0x5a)return false;
  if(!storage.result.changed || !storage.result.presentation_required)return false;
 }
 return true;
}
int main(int argc,char** argv){if(argc!=2)return 20;if(!guards())return 21;FILE* f=fopen(argv[1],"rb");if(!f)return 20;
 unsigned char b[28+24+alpha::monument_table_bytes];size_t count=0;
 for(;;){size_t n=fread(b,1,sizeof b,f);if(!n){bool ok=!ferror(f);fclose(f);return ok?0:20;}if(n!=sizeof b || ++count>100000)return 20;
 alpha::MonumentEffectInput in{};in.variant=get(b)?alpha::Variant::alien_crossfire:alpha::Variant::base;
 in.faction=s(get(b+4));in.local_faction=s(get(b+8));in.event=s(get(b+12));in.year=s(get(b+16));in.faction_rule_flags=get(b+20);
 in.size=get(b+24);in.monuments=b+52;memcpy(in.facility_base_name,b+28,24);
 alpha::MonumentEffectResult r;memset(r.record,0x5a,sizeof r.record);r.changed=true;r.presentation_required=true;
 auto err=get(b)>1?alpha::LogicError::bad_state:alpha::monument_effect(in,r);
 if(err!=alpha::LogicError::ok){for(auto c:r.record)if(c!=0x5a)return 21;if(!r.changed || !r.presentation_required)return 21;}
 if(!emit((uint32_t)err) || !emit(r.changed) || !emit(r.presentation_required) || fwrite(r.record,1,sizeof r.record,stdout)!=sizeof r.record)return 20;
 }
}
