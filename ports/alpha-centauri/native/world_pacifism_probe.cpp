/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_pacifism.h"
#include "local_actions_test_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static alpha::Rules rules;static alpha::UnitDefinitions units,saved_units;
static uint8_t* read(const char* name,size_t& n){FILE* f=fopen(name,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long size=ftell(f);rewind(f);if(size<=0 || size>64*1024*1024){fclose(f);return nullptr;}auto* p=(uint8_t*)malloc((size_t)size);if(!p){fclose(f);return nullptr;}const bool ok=fread(p,1,(size_t)size,f)==(size_t)size;fclose(f);if(!ok){free(p);return nullptr;}n=(size_t)size;return p;}
static bool emit(uint32_t n) { uint8_t b[4];for (unsigned i=0;i<4;++i) { b[i]=(uint8_t)n;n>>=8; }return fwrite(b,1,4,stdout)==4; }
int main(int argc,char** argv) {
 using namespace alpha;if (argc!=4 || (strcmp(argv[1],"base") && strcmp(argv[1],"expansion"))) return 20;
 size_t n;auto* bytes=read(argv[2],n);if (!bytes) return 20;auto error=read_technologies(bytes,n,rules);
 if (error==Error::ok) error=read_unit_definitions(bytes,n,rules,!strcmp(argv[1],"base")?Variant::base:Variant::alien_crossfire,units);
 free(bytes);if (error!=Error::ok) return 20;memcpy(&saved_units,&units,sizeof units);
 bytes=read(argv[3],n);if (!bytes) return 20;OriginalWorld w,baseline;
 if (w.load(bytes,n,123)!=WorldLoadError::ok || baseline.load(bytes,n,123)!=WorldLoadError::ok) { free(bytes);return 20; }
 const size_t count=w.game().base_count<8?w.game().base_count:8;
 for (size_t base=0;base<count;++base) {
  WorldPacifism out,before;memset(&out,0x5a,sizeof out);memcpy(&before,&out,sizeof out);const auto e=world_pacifism(w,units,base,out);
  if (!emit((uint32_t)e)) return 21;
  if (e!=LogicError::ok) { if (memcmp(&out,&before,sizeof out)) return 21;for (unsigned i=0;i<7;++i) if (!emit(0x5a5a5a5a)) return 21; }
  else if (!emit((uint32_t)out.variant) || !emit((uint32_t)out.base) || !emit((uint32_t)out.count) || !emit((uint32_t)out.home_combat) || !emit((uint32_t)out.away) || !emit((uint32_t)out.air) || !emit((uint32_t)out.both)) return 21;
 }
 WorldPacifism out,before;memset(&out,0x5a,sizeof out);memcpy(&before,&out,sizeof out);unsigned guards=0;
 auto bad=[&](const OriginalWorld& world,size_t base) { ++guards;return world_pacifism(world,units,base,out)==LogicError::bad_state && !memcmp(&out,&before,sizeof out); };
 OriginalWorld absent;if (!bad(absent,0) || !bad(w,w.game().base_count) || !bad(w,SIZE_MAX)) return 21;
 if (count) {
  const auto weapons=units.weapon_count;units.weapon_count=0;if (!bad(w,0)) return 21;units.weapon_count=weapons;
  const void* ranges[]={&w,&units,w.original_bytes(),w.tile(0),w.vehicle(0),w.base(0),w.prototype(0),w.faction(0),w.faction_definition(0),(const uint8_t*)&units+4,w.original_bytes()+4};
  for (const void* range:ranges) if (range) { ++guards;auto& alias=*reinterpret_cast<WorldPacifism*>(const_cast<void*>(range));if (world_pacifism(w,units,0,alias)!=LogicError::bad_state) return 21; }
 }
 if (!emit(guards) || !alpha_test::same_world(w,baseline) || w.runtime_seed()!=123 || memcmp(w.original_bytes(),bytes,n) || memcmp(&saved_units,&units,sizeof units)) return 21;
 free(bytes);return 0;
}
