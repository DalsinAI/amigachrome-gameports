/* SPDX-License-Identifier: GPL-3.0-or-later. See monument_effects.h. */
#include "monument_effects.h"
#include <stdint.h>
namespace alpha {
static uint32_t get(const uint8_t* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static void put(uint8_t* p,uint32_t n){for(unsigned i=0;i<4;++i){p[i]=(uint8_t)n;n>>=8;}}
static bool overlap(const void* a,size_t an,const void* b,size_t bn){const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<=y?y-x<an:x-y<bn;}
LogicError monument_effect(const MonumentEffectInput& in,MonumentEffectResult& out){
 if((in.variant!=Variant::base && in.variant!=Variant::alien_crossfire) || !in.monuments ||
 in.size!=monument_table_bytes || in.faction<0 || in.faction>7 || in.local_faction<0 || in.local_faction>7 ||
 (in.event!=2 && in.event!=4 && in.event!=12) || overlap(&out,sizeof out,in.monuments,in.size) || overlap(&out,sizeof out,&in,sizeof in))return LogicError::bad_state;
 MonumentEffectResult next{};const uint8_t* self=in.monuments+(size_t)in.faction*monument_record_bytes;
 for(size_t i=0;i<monument_record_bytes;++i)next.record[i]=self[i];
 const size_t e=(size_t)in.event*56;
 if(in.event==2){
  if(!(in.variant==Variant::alien_crossfire && (in.faction_rule_flags&0x80u))){
   if(get(self+e+16)){
    if(get(self+e+8)==UINT32_MAX)put(next.record+e+8,(uint32_t)in.year);
    else if(get(self+e+4)==UINT32_MAX)put(next.record+e+4,(uint32_t)in.year);
   }else{
    bool other=false;int32_t first=in.faction;
    for(int32_t i=0;i<8;++i){const uint8_t* p=in.monuments+(size_t)i*monument_record_bytes+e;
     if(i!=in.faction && get(p+16)){other=true;if(get(p+20))first=i;}
    }
    put(next.record+e,(uint32_t)in.year);put(next.record+e+4,UINT32_MAX);put(next.record+e+8,UINT32_MAX);
    put(next.record+e+12,(uint32_t)first);put(next.record+e+16,1);put(next.record+e+20,other?0u:1u);
   }
   put(next.record+0x498,2);next.presentation_required=in.faction==in.local_faction;
   for(size_t i=0;i<monument_record_bytes;++i)if(next.record[i]!=self[i])next.changed=true;
  }
  out=next;return LogicError::ok;
 }
 if(!get(self+e+16) && !(in.variant==Variant::alien_crossfire && (in.faction_rule_flags&0x80u))){
  bool other=false;int32_t first=in.faction;
  for(int32_t i=0;i<8;++i){const uint8_t* p=in.monuments+(size_t)i*monument_record_bytes+e;
   if(i!=in.faction && get(p+16)){other=true;if(get(p+20))first=i;}
  }
  put(next.record+e,(uint32_t)in.year);put(next.record+e+12,(uint32_t)first);
  put(next.record+e+16,1);put(next.record+e+20,other?0u:1u);put(next.record+0x498,(uint32_t)in.event);
  if(in.event==4){bool ended=false;for(unsigned i=0;i<24;++i){uint8_t c=ended?0:in.facility_base_name[i];next.record[0x4c4+i]=c;if(!c)ended=true;}}
  next.changed=true;next.presentation_required=in.faction==in.local_faction;
 }
 out=next;return LogicError::ok;
}
}
