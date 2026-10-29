/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_unit_stats.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static UnitDefinitions units;static Rules tech;static BasicRules basic;static WorldRules worldrules;
static unsigned char*read(const char*p,size_t&n){FILE*f=fopen(p,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long z=ftell(f);rewind(f);if(z<1||z>64*1024*1024){fclose(f);return nullptr;}auto*b=(unsigned char*)malloc((size_t)z);if(!b){fclose(f);return nullptr;}bool ok=fread(b,1,(size_t)z,f)==(size_t)z;fclose(f);if(!ok){free(b);return nullptr;}n=(size_t)z;return b;}
static bool emit(uint32_t n){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
template<class T>static T&alias(uintptr_t p){return *reinterpret_cast<T*>(p);}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"failed line %d\n",__LINE__);return 21;}++guards;}while(0)
int main(int argc,char**argv){if(argc!=3)return 20;size_t n=0,nt=0;auto*data=read(argv[2],n);auto*text=read(argv[1],nt);if(!data||!text)return 20;OriginalWorld w;if(w.load(data,n,0x12345678)!=WorldLoadError::ok||!w.game().vehicle_count)return 20;const auto variant=w.game().data_version<11?Variant::base:Variant::alien_crossfire;if(read_technologies(text,nt,tech)!=Error::ok||read_unit_definitions(text,nt,tech,variant,units)!=Error::ok||read_basic_rules(text,nt,tech,variant,basic,worldrules)!=Error::ok)return 20;unsigned guards=0;
 uint32_t m0=123,m1=123,s0=123,s1=123;
 for(size_t i=0;i<w.game().vehicle_count;++i){
  if(world_effective_morale(w,units,tech,i,false,m0)!=LogicError::ok||world_effective_morale(w,units,tech,i,true,m1)!=LogicError::ok||world_vehicle_speed(w,units,tech,i,basic.move_rate_roads,false,s0)!=LogicError::ok||world_vehicle_speed(w,units,tech,i,basic.move_rate_roads,true,s1)!=LogicError::ok)return 22;
  if(!emit(m0)||!emit(m1)||!emit(s0)||!emit(s1))return 23;
 }
 CHECK(w.runtime_seed()==0x12345678&&w.original_size()==n&&!memcmp(w.original_bytes(),data,n));if(!emit(guards))return 24;free(data);free(text);return 0;}
