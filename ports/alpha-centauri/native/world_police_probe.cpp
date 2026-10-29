/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_police.h"
#include "local_actions_test_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static alpha::Rules rules,saved_rules;static alpha::UnitDefinitions units,saved_units;
static alpha::WorldPoliceGarrison out,before;
static uint8_t* read(const char* name,size_t& n){FILE* f=fopen(name,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long size=ftell(f);rewind(f);if(size<=0 || size>64*1024*1024){fclose(f);return nullptr;}auto* p=(uint8_t*)malloc((size_t)size);if(!p){fclose(f);return nullptr;}const bool ok=fread(p,1,(size_t)size,f)==(size_t)size;fclose(f);if(!ok){free(p);return nullptr;}n=(size_t)size;return p;}
static bool emit(uint32_t n) { uint8_t b[4];for (unsigned i=0;i<4;++i) { b[i]=(uint8_t)n;n>>=8; }return fwrite(b,1,4,stdout)==4; }
int main(int argc,char** argv) {
 using namespace alpha;if (argc!=4 || (strcmp(argv[1],"base") && strcmp(argv[1],"expansion"))) return 20;
 size_t n;auto* bytes=read(argv[2],n);if (!bytes) return 20;auto error=read_technologies(bytes,n,rules);
 if (error==Error::ok) error=read_unit_definitions(bytes,n,rules,!strcmp(argv[1],"base")?Variant::base:Variant::alien_crossfire,units);
 free(bytes);if (error!=Error::ok) return 20;memcpy(&saved_rules,&rules,sizeof rules);memcpy(&saved_units,&units,sizeof units);
 bytes=read(argv[3],n);if (!bytes) return 20;OriginalWorld w,baseline;
 if (w.load(bytes,n,123)!=WorldLoadError::ok || baseline.load(bytes,n,123)!=WorldLoadError::ok) { free(bytes);return 20; }
 for (size_t id=0;id<512;++id) {
  bool doubled=true;const auto e=world_double_police(w,units,rules,id,doubled);if (e!=PoliceError::ok && !doubled) return 21;
  if (!emit((uint32_t)e) || !emit(doubled)) return 21;
 }
 const size_t count=w.game().base_count<8?w.game().base_count:8;
 for (size_t base=0;base<count;++base) {
  memset(&out,0x5a,sizeof out);memcpy(&before,&out,sizeof out);
  const auto e=world_police_garrison(w,units,rules,base,PoliceSnapshotProfile::offline_validated,out);
  if (!emit((uint32_t)e)) return 21;
  if (e!=PoliceError::ok) { if (memcmp(&out,&before,sizeof out)) return 21;continue; }
  if (!emit((uint32_t)out.variant) || !emit((uint32_t)out.base) || !emit(out.count)) return 21;
  for (size_t i=0;i<out.count;++i) { const auto& f=out.units[i];if (!emit((uint32_t)out.vehicle_ids[i]) || !emit((uint32_t)f.prototype_id) || !emit((uint32_t)f.plan) || !emit((uint32_t)f.weapon_offense) || !emit((uint32_t)f.triad) || !emit(f.double_police)) return 21; }
  for (size_t i=out.count;i<2048;++i) if (out.vehicle_ids[i] || out.units[i].prototype_id || out.units[i].plan || out.units[i].weapon_offense || out.units[i].triad || out.units[i].double_police) return 21;
 }
 memset(&out,0x5a,sizeof out);memcpy(&before,&out,sizeof out);unsigned guards=0;
 auto bad=[&](const OriginalWorld& world,size_t base,PoliceSnapshotProfile profile) { ++guards;const auto e=world_police_garrison(world,units,rules,base,profile,out);return e!=PoliceError::ok && !memcmp(&out,&before,sizeof out); };
 bool doubled=true;guards+=2;if (world_double_police(w,units,rules,512,doubled)!=PoliceError::bad_state || !doubled) return 21;
 OriginalWorld absent;if (world_double_police(absent,units,rules,0,doubled)!=PoliceError::bad_buffer || !doubled) return 21;
 if (!bad(absent,0,PoliceSnapshotProfile::offline_validated) || !bad(w,w.game().base_count,PoliceSnapshotProfile::offline_validated) || !bad(w,SIZE_MAX,PoliceSnapshotProfile::offline_validated)) return 21;
 if (count) {
  if (!bad(w,0,(PoliceSnapshotProfile)99)) return 21;
  rules.count=88;if (!bad(w,0,PoliceSnapshotProfile::offline_validated)) return 21;rules.count=89;
  const void* ranges[]={&w,&rules,&units,w.original_bytes(),w.tile(0),w.vehicle(0),w.base(0),w.prototype(0),w.faction(0),w.faction_definition(0),(const uint8_t*)&rules+4,w.original_bytes()+4};
  for (const void* range:ranges) if (range) {
   ++guards;auto& alias=*reinterpret_cast<WorldPoliceGarrison*>(const_cast<void*>(range));if (world_police_garrison(w,units,rules,0,PoliceSnapshotProfile::offline_validated,alias)!=PoliceError::bad_buffer) return 21;
   ++guards;auto& flag=*reinterpret_cast<bool*>(const_cast<void*>(range));if (world_double_police(w,units,rules,64,flag)!=PoliceError::bad_buffer) return 21;
  }
 }
 if (!emit(guards) || !alpha_test::same_world(w,baseline) || w.runtime_seed()!=123 || memcmp(w.original_bytes(),bytes,n) || memcmp(&saved_rules,&rules,sizeof rules) || memcmp(&saved_units,&units,sizeof units)) return 21;
 free(bytes);return 0;
}
