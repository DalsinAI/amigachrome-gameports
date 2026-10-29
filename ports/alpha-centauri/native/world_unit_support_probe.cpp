/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_unit_support.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static alpha::Rules tech;static alpha::UnitDefinitions definitions;
static uint8_t* read(const char* name,size_t& size,size_t limit){FILE* f=fopen(name,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long n=ftell(f);rewind(f);if(n<=0 || (size_t)n>limit){fclose(f);return nullptr;}auto* p=(uint8_t*)malloc((size_t)n);if(!p){fclose(f);return nullptr;}bool ok=fread(p,1,(size_t)n,f)==(size_t)n;fclose(f);if(!ok){free(p);return nullptr;}size=(size_t)n;return p;}
static void hash(uint32_t& h,uint32_t n){for(unsigned i=0;i<4;++i){h=(h^(n&255u))*16777619u;n>>=8;}}
static void values(const alpha::UnitSupportTotals& a,int32_t* v){v[0]=a.support_rating;v[1]=a.free_units;v[2]=a.cost_per_unit;v[3]=a.supported_units;v[4]=a.maintained_units;v[5]=a.clean_exempt;v[6]=a.native_fungus_exempt;v[7]=a.mineral_cost;}
static bool unchanged(const alpha::UnitSupportTotals& a){int32_t v[8];values(a,v);for(auto x:v)if(x!=0x31415926)return false;return true;}
static uint32_t wire_hash(const alpha::OriginalWorld& w){uint32_t h=2166136261u;for(size_t i=0;i<w.original_size();++i)h=(h^w.original_bytes()[i])*16777619u;return h;}
int main(int argc,char** argv){
 if(argc!=4 || (strcmp(argv[1],"base") && strcmp(argv[1],"expansion")))return 20;
 const auto variant=!strcmp(argv[1],"base")?alpha::Variant::base:alpha::Variant::alien_crossfire;
 size_t size=0;auto* bytes=read(argv[2],size,1048576);if(!bytes)return 20;
 bool ok=alpha::read_technologies(bytes,size,tech)==alpha::Error::ok && alpha::read_unit_definitions(bytes,size,tech,variant,definitions)==alpha::Error::ok;free(bytes);if(!ok)return 20;
 bytes=read(argv[3],size,64u*1024u*1024u);if(!bytes)return 20;alpha::OriginalWorld w;const auto loaded=w.load(bytes,size,0xabcdef01u);free(bytes);if(loaded!=alpha::WorldLoadError::ok || (w.game().data_version<11)!=(variant==alpha::Variant::base))return 20;
 uint32_t digest=2166136261u;const uint32_t before=wire_hash(w);
 for(size_t base=0;base<w.game().base_count;++base){
  alpha::UnitSupportTotals result{0x31415926,0x31415926,0x31415926,0x31415926,0x31415926,0x31415926,0x31415926,0x31415926};
  const auto error=alpha::world_unit_support(w,definitions,tech,base,result);
  if(error!=alpha::LogicError::ok){if(!unchanged(result) || wire_hash(w)!=before || w.runtime_seed()!=0xabcdef01u)return 21;printf("ERROR support=%d base=%lu unchanged=1 rc=20\n",(int)error,(unsigned long)base);return 20;}
  int32_t row[8];values(result,row);hash(digest,(uint32_t)base);for(auto n:row)hash(digest,(uint32_t)n);
 }
 alpha::UnitSupportTotals bad{0x31415926,0x31415926,0x31415926,0x31415926,0x31415926,0x31415926,0x31415926,0x31415926};
 if(alpha::world_unit_support(w,definitions,tech,w.game().base_count,bad)!=alpha::LogicError::bad_state || !unchanged(bad))return 21;
 if(wire_hash(w)!=before || w.runtime_seed()!=0xabcdef01u)return 21;
 printf("bases=%lu digest=%08lx unchanged=1 seed=abcdef01 rc=0\n",(unsigned long)w.game().base_count,(unsigned long)digest);return 0;
}
