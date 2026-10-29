/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "prototype_retirement_queue.h"
#include <stdio.h>
#include <string.h>
static uint32_t get(const unsigned char* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int32_t signed32(uint32_t n){int32_t v;memcpy(&v,&n,4);return v;}
static bool emit(uint32_t n){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
static bool checks(){
 alpha::OriginalBase b{};b.queue_size=1;b.queue_items[0]=64;b.queue_items[1]=64;
 alpha::RetirementQueueProposal p;
 if(alpha::retirement_queue_stage(alpha::Variant::base,64,5,&b,1,p)!=alpha::RetirementQueueError::ok)return false;
 const alpha::OriginalBase saved=*p.base(0);const uint16_t flags=p.unit_flags();
 for(unsigned mode=0;mode<8;++mode){
  auto v=mode==0?(alpha::Variant)99:alpha::Variant::base;int id=mode==1?-1:mode==2?512:64;
  const alpha::OriginalBase* a=mode==3?nullptr:mode==4?(const alpha::OriginalBase*)&p:mode==6?(const alpha::OriginalBase*)(UINTPTR_MAX-3):&b;
  size_t count=mode==5?513:1;
  b.queue_size=mode==7?10:1;
  if(alpha::retirement_queue_stage(v,id,5,a,count,p)!=alpha::RetirementQueueError::bad_state || !p.base(0) || memcmp(&saved,p.base(0),sizeof saved) || p.unit_flags()!=flags)return false;
 }
 if(alpha::retirement_queue_stage(alpha::Variant::base,64,p.unit_flags(),p.base(0),p.count(),p)!=alpha::RetirementQueueError::ok)return false;
 p.clear();p.clear();return !p.count() && !p.base(0);
}
int main(int argc,char** argv){if(argc!=2 || !checks())return 20;FILE* f=fopen(argv[1],"rb");if(!f)return 20;alpha::OriginalBase b[512];unsigned char h[16],r[48];
 for(;;){size_t n=fread(h,1,16,f);if(!n){int e=ferror(f);fclose(f);return e?20:0;}if(n!=16 || get(h+12)>512)return 20;const size_t count=get(h+12);
  for(size_t i=0;i<count;++i){if(fread(r,1,48,f)!=48)return 20;memset(b+i,0xa5,sizeof b[i]);b[i].faction_id=(int8_t)get(r);b[i].queue_size=signed32(get(r+4));for(unsigned j=0;j<10;++j)b[i].queue_items[j]=signed32(get(r+8+4*j));}
  alpha::RetirementQueueProposal p;auto err=alpha::retirement_queue_stage((alpha::Variant)get(h),signed32(get(h+4)),(uint16_t)get(h+8),b,count,p);
  if(!emit((uint32_t)err))return 20;
  if(err!=alpha::RetirementQueueError::ok){if(p.count() || p.base(0))return 21;continue;}
  if(!emit(p.unit_flags()) || !emit(p.changed()) || !emit(p.count()))return 20;
  for(size_t i=0;i<count;++i){const auto* q=p.base(i);alpha::OriginalBase copy=*q;copy.queue_size=b[i].queue_size;for(unsigned j=0;j<10;++j)copy.queue_items[j]=b[i].queue_items[j];if(memcmp(&copy,b+i,sizeof copy))return 21;
   if(!emit((uint32_t)q->faction_id) || !emit((uint32_t)q->queue_size))return 20;
   for(unsigned j=0;j<10;++j)if(!emit((uint32_t)q->queue_items[j]))return 20;}
 }
}
