/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_citizen_energy.h"
#include "local_actions_test_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static alpha::Rules rules,saved_rules;
static alpha::CitizenDefinitions definitions,saved_definitions;
static uint8_t* read(const char* name,size_t& n){FILE* f=fopen(name,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long size=ftell(f);rewind(f);if(size<=0 || size>64*1024*1024){fclose(f);return nullptr;}auto* p=(uint8_t*)malloc((size_t)size);if(!p){fclose(f);return nullptr;}const bool ok=fread(p,1,(size_t)size,f)==(size_t)size;fclose(f);if(!ok){free(p);return nullptr;}n=(size_t)size;return p;}
static bool emit(uint32_t n) { uint8_t bytes[4];for (unsigned i=0;i<4;++i) { bytes[i]=(uint8_t)n;n>>=8; }return fwrite(bytes,1,4,stdout)==4; }
static int guards(const alpha::OriginalWorld& w) {
 using namespace alpha;
 int count=0;WorldCitizenEnergy out,before;memset(&out,0x5a,sizeof out);memcpy(&before,&out,sizeof out);
 auto bad=[&](const OriginalWorld& world,int32_t faction) { ++count;return world_citizen_energy(world,rules,definitions,faction,out)==LogicError::bad_state && !memcmp(&out,&before,sizeof out); };
 if (!bad(w,-1) || !bad(w,8)) return -1;
 OriginalWorld absent;if (!bad(absent,1)) return -1;
 rules.count=88;if (!bad(w,1)) return -1;rules.count=89;
 definitions.count=9;if (!bad(w,1)) return -1;definitions.count=10;
 const Variant edition=definitions.variant;
 definitions.variant=edition==Variant::base ? Variant::alien_crossfire : Variant::base;
 if (!bad(w,1)) return -1;
 definitions.variant=(Variant)99;if (!bad(w,1)) return -1;definitions.variant=edition;
 for (unsigned col=0;col<2;++col) {
  auto& value=rules.technologies[88].prerequisites[col];const auto saved=value;
  value=-3;if (!bad(w,1)) return -1;value=89;if (!bad(w,1)) return -1;value=saved;
 }
 int32_t* ids[]={&definitions.rows[6].prerequisite,&definitions.rows[6].obsolete};
 for (auto* value:ids) { const auto saved=*value;*value=-3;if (!bad(w,1)) return -1;*value=89;if (!bad(w,1)) return -1;*value=saved; }
 auto alias=[&](const void* memory) { if (!memory) return true;++count;return world_citizen_energy(w,rules,definitions,1,*reinterpret_cast<WorldCitizenEnergy*>(const_cast<void*>(memory)))==LogicError::bad_state; };
 const void* ranges[]={&w,&rules,&definitions,w.original_bytes(),w.tile(0),w.vehicle(0),w.base(0),w.prototype(0),w.faction(0),w.faction_definition(0),(const uint8_t*)&rules+4,(const uint8_t*)&definitions+4,w.original_bytes()+4,(const uint8_t*)w.prototype(0)+4};
 for (auto* range:ranges) if (!alias(range)) return -1;
 return count;
}
int main(int argc,char** argv) {
 using namespace alpha;
 if (argc!=5 || (strcmp(argv[1],"base") && strcmp(argv[1],"expansion"))) return 20;
 size_t n=0;auto* bytes=read(argv[2],n);if (!bytes) return 20;
 auto error=read_technologies(bytes,n,rules);
 if (error==Error::ok) error=read_citizen_definitions(bytes,n,rules,!strcmp(argv[1],"base")?Variant::base:Variant::alien_crossfire,definitions);
 free(bytes);if (error!=Error::ok) return 20;
 memcpy(&saved_rules,&rules,sizeof rules);memcpy(&saved_definitions,&definitions,sizeof definitions);
 OriginalWorld w;
 for (unsigned phase=0;phase<2;++phase) {
 bytes=read(argv[3+phase],n);if (!bytes) return 20;
 OriginalWorld baseline;if (w.load(bytes,n,123)!=WorldLoadError::ok || baseline.load(bytes,n,123)!=WorldLoadError::ok) { free(bytes);return 20; }
 for (int32_t f=0;f<8;++f) {
  WorldCitizenEnergy out,before;memset(&out,0x5a,sizeof out);memcpy(&before,&out,sizeof out);
  const auto result=world_citizen_energy(w,rules,definitions,f,out);
  if (result!=LogicError::ok) { if (memcmp(&out,&before,sizeof out)) return 21;free(bytes);return result==LogicError::bad_state?22:21; }
  if (!emit((uint32_t)out.variant) || !emit((uint32_t)out.faction)) return 21;
  for (const auto& c:out.citizens) if (!emit(c.prerequisite_available) || !emit(c.obsolete_available) || !emit((uint32_t)c.economy) || !emit((uint32_t)c.psych) || !emit((uint32_t)c.labs)) return 21;
 }
 const int count=guards(w);if (count<0 || !emit((uint32_t)count)) return 21;
 if (!alpha_test::same_world(w,baseline) || w.runtime_seed()!=123 || w.original_size()!=n || memcmp(w.original_bytes(),bytes,n) || memcmp(&rules,&saved_rules,sizeof rules) || memcmp(&definitions,&saved_definitions,sizeof definitions)) return 21;
 free(bytes);
 }
 return 0;
}
