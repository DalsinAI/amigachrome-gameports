/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_commerce.h"
#include "local_actions_test_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static alpha::Rules rules,saved_rules;
static uint8_t* read(const char* name,size_t& n){FILE* f=fopen(name,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long size=ftell(f);rewind(f);if(size<=0 || size>64*1024*1024){fclose(f);return nullptr;}auto* p=(uint8_t*)malloc((size_t)size);if(!p){fclose(f);return nullptr;}const bool ok=fread(p,1,(size_t)size,f)==(size_t)size;fclose(f);if(!ok){free(p);return nullptr;}n=(size_t)size;return p;}
static bool emit(uint32_t n) { uint8_t b[4];for (unsigned i=0;i<4;++i) { b[i]=(uint8_t)n;n>>=8; }return fwrite(b,1,4,stdout)==4; }
static bool result(alpha::LogicError error,const alpha::WorldCommerceResult& out) {
 if (!emit((uint32_t)error)) return false;
 if (error!=alpha::LogicError::ok) { for (unsigned i=0;i<27;++i) if (!emit(0x5a5a5a5a)) return false;return true; }
 if (!emit((uint32_t)out.variant) || !emit((uint32_t)out.base) || !emit((uint32_t)out.rank) || !emit((uint32_t)out.global_technology_count)) return false;
 for (auto n:out.partner) if (!emit((uint32_t)n)) return false;
 for (auto n:out.imports) if (!emit((uint32_t)n)) return false;
 for (auto n:out.exports) if (!emit((uint32_t)n)) return false;
 return emit((uint32_t)out.total) && emit(out.qualified_foreign_commerce);
}
int main(int argc,char** argv) {
 using namespace alpha;if (argc!=3) return 20;
 size_t n;auto* bytes=read(argv[1],n);if (!bytes) return 20;auto parsed=read_technologies(bytes,n,rules);free(bytes);if (parsed!=Error::ok) return 20;
 memcpy(&saved_rules,&rules,sizeof rules);bytes=read(argv[2],n);if (!bytes) return 20;OriginalWorld w,baseline;
 if (w.load(bytes,n,123)!=WorldLoadError::ok || baseline.load(bytes,n,123)!=WorldLoadError::ok) { free(bytes);return 20; }
 CommerceBonuses bonuses{};bonuses.variant=w.game().data_version<11?Variant::base:Variant::alien_crossfire;
 for (unsigned i=0;i<8;++i) bonuses.faction[i]=w.faction(i)->tech_commerce_bonus;
 const auto saved_bonuses=bonuses;
 const size_t count=w.game().base_count<8?w.game().base_count:8;
 for (size_t base=0;base<count;++base) for (unsigned mode=0;mode<2;++mode) {
  WorldCommerceResult out,before;memset(&out,0x5a,sizeof out);memcpy(&before,&out,sizeof out);
  auto error=mode ? fresh_world_commerce(w,rules,base,out) : world_commerce(w,rules,bonuses,base,out);
  if (error!=LogicError::ok && memcmp(&out,&before,sizeof out)) return 21;
  if (!result(error,out)) return 21;
 }
 WorldCommerceResult out,before;memset(&out,0x5a,sizeof out);memcpy(&before,&out,sizeof out);unsigned guards=0;
 auto bad=[&](const OriginalWorld& world,size_t base) { ++guards;return world_commerce(world,rules,bonuses,base,out)==LogicError::bad_state && !memcmp(&out,&before,sizeof out); };
 OriginalWorld absent;if (!bad(absent,0) || !bad(w,w.game().base_count) || !bad(w,SIZE_MAX)) return 21;
 if (count) {
  rules.count=88;if (!bad(w,0)) return 21;rules.count=89;
  const auto edition=bonuses.variant;bonuses.variant=(Variant)99;if (!bad(w,0)) return 21;bonuses.variant=edition;
  const auto prerequisite=rules.technologies[88].prerequisites[1];rules.technologies[88].prerequisites[1]=-3;if (!bad(w,0)) return 21;rules.technologies[88].prerequisites[1]=prerequisite;
  const void* ranges[]={&w,&rules,&bonuses,w.original_bytes(),w.tile(0),w.vehicle(0),w.base(0),w.prototype(0),w.faction(0),w.faction_definition(0),(const uint8_t*)&rules+4,w.original_bytes()+4};
  for (const void* range:ranges) if (range) {
   ++guards;auto& alias=*reinterpret_cast<WorldCommerceResult*>(const_cast<void*>(range));
   if (world_commerce(w,rules,bonuses,0,alias)!=LogicError::bad_state) return 21;
   if (range!=&bonuses) { ++guards;if (fresh_world_commerce(w,rules,0,alias)!=LogicError::bad_state) return 21; }
  }
 }
 if (!emit(guards) || !alpha_test::same_world(w,baseline) || w.runtime_seed()!=123 || memcmp(w.original_bytes(),bytes,n) || memcmp(&saved_rules,&rules,sizeof rules) || memcmp(&saved_bonuses,&bonuses,sizeof bonuses)) return 21;
 free(bytes);return 0;
}
