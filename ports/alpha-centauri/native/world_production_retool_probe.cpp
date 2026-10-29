/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_production_retool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t hash(const alpha::OriginalWorld& w){uint32_t h=2166136261;for(size_t i=0;i<w.original_size();++i)h=(h^w.original_bytes()[i])*16777619u;return h;}
int main(int argc,char** argv){
 if(argc!=7 || (strcmp(argv[1],"base") && strcmp(argv[1],"expansion")))return 20;
 FILE* f=fopen(argv[2],"rb");if(!f)return 20;if(fseek(f,0,SEEK_END)){fclose(f);return 20;}const long n=ftell(f);rewind(f);if(n<=0 || n>64*1024*1024){fclose(f);return 20;}
 auto* bytes=(uint8_t*)malloc((size_t)n);if(!bytes){fclose(f);return 20;}const bool read=fread(bytes,1,(size_t)n,f)==(size_t)n;fclose(f);if(!read){free(bytes);return 20;}
 alpha::OriginalWorld w;if(w.load(bytes,(size_t)n,0xabcdef01)!=alpha::WorldLoadError::ok){free(bytes);return 20;}free(bytes);const uint32_t before=hash(w);
 alpha::BasicRules rules{};rules.retool_strictness=atoi(argv[3]);rules.retool_pct_pen_prod_chg=atoi(argv[4]);rules.retool_exemption=atoi(argv[5]);const int32_t proposed=atoi(argv[6]);const auto variant=!strcmp(argv[1],"base")?alpha::Variant::base:alpha::Variant::alien_crossfire;
 uint32_t digest=2166136261;auto add=[&](int32_t x){uint32_t v=(uint32_t)x;for(unsigned i=0;i<4;++i){digest=(digest^(v&255))*16777619u;v>>=8;}};
 for(size_t i=0;i<w.game().base_count;++i){alpha::ProductionRetoolResult r{0x31415926,0x31415926,0x31415926};const auto e=alpha::world_production_retool(w,rules,variant,i,proposed,r);if(e!=alpha::LogicError::ok){if(r.previous_class!=0x31415926 || r.queued_class!=0x31415926 || r.mineral_loss!=0x31415926 || hash(w)!=before || w.runtime_seed()!=0xabcdef01)return 21;printf("error=%d unchanged=1 rc=20\n",(int)e);return 20;}add((int32_t)i);add(r.previous_class);add(r.queued_class);add(r.mineral_loss);}
 alpha::ProductionRetoolResult bad{0x31415926,0x31415926,0x31415926};if(alpha::world_production_retool(w,rules,variant,w.game().base_count,proposed,bad)!=alpha::LogicError::bad_state || bad.previous_class!=0x31415926 || bad.queued_class!=0x31415926 || bad.mineral_loss!=0x31415926 || hash(w)!=before || w.runtime_seed()!=0xabcdef01)return 21;
 printf("bases=%lu digest=%08lx unchanged=1 seed=abcdef01 rc=0\n",(unsigned long)w.game().base_count,(unsigned long)digest);return 0;
}
