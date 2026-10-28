/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_promotion.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __SANITIZE_ADDRESS__
#include <sanitizer/asan_interface.h>
#endif
using namespace alpha;
alignas(WorldPromotionPlan) static UnitDefinitions units;alignas(WorldPromotionPlan) static Rules tech;
static bool rd(FILE*f,uint32_t&v){v=0;for(unsigned j=0;j<4;++j){int b=fgetc(f);if(b==EOF)return false;v|=(uint32_t)b<<(8*j);}return true;}
static int32_t signed32(uint32_t v){return v<=0x7fffffffu?(int32_t)v:-1-(int32_t)(~v);}
static uint32_t hashbytes(uint32_t h,const void*p,size_t n){auto*b=(const uint8_t*)p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
static uint32_t fingerprint(const OriginalWorld&w){uint32_t h=2166136261u;h=hashbytes(h,w.original_bytes(),w.original_size());h=hashbytes(h,w.current_monuments().data,w.current_monuments().size);h=hashbytes(h,w.game().bunglist.data,w.game().bunglist.size);h=hashbytes(h,&w.game().preferences,sizeof w.game().preferences);h=hashbytes(h,w.tile(0),w.game().map.info.tile_count*sizeof(OriginalMapTile));h=hashbytes(h,w.vehicle(0),w.game().vehicle_count*sizeof(OriginalVehicle));h=hashbytes(h,w.base(0),w.game().base_count*sizeof(OriginalBase));h=hashbytes(h,w.prototype(0),512*sizeof(OriginalPrototype));h=hashbytes(h,w.faction(0),8*sizeof(OriginalFactionState));return hashbytes(h,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));}
static bool collateral(const OriginalWorld&a,const OriginalWorld&b){if(a.original_size()!=b.original_size()||a.original_bytes()==b.original_bytes()||a.runtime_seed()!=b.runtime_seed()||memcmp(a.original_bytes(),b.original_bytes(),a.original_size())||a.current_monuments().data==b.current_monuments().data||a.game().bunglist.data==b.game().bunglist.data||memcmp(a.game().bunglist.data,b.game().bunglist.data,4096)||memcmp(&a.game().map.info,&b.game().map.info,sizeof a.game().map.info))return false;auto pref=a.game().preferences;pref.VehCount=(int32_t)b.game().vehicle_count;if(memcmp(&pref,&b.game().preferences,sizeof pref))return false;
for(size_t i=0;i<512;++i)if(memcmp(a.prototype(i),b.prototype(i),sizeof(OriginalPrototype)))return false;
for(size_t i=0;i<a.game().base_count;++i)if(memcmp(a.base(i),b.base(i),sizeof(OriginalBase)))return false;
for(size_t i=0;i<8;++i)if(memcmp(a.faction_definition(i),b.faction_definition(i),sizeof(OriginalFactionDefinition)))return false;
#define SPAN(name) if(a.game().name.size!=b.game().name.size||(a.game().name.size&&memcmp(a.game().name.data,b.game().name.data,a.game().name.size)))return false;
SPAN(factions) SPAN(faction_definitions) SPAN(rankings) SPAN(faction_turn_might) SPAN(replay) SPAN(continents) SPAN(prototypes) SPAN(vehicles) SPAN(bases) SPAN(scenario) SPAN(scenario_name) SPAN(objectives) SPAN(monuments) SPAN(tectonic)
#undef SPAN
return true;}
static bool word(uint32_t n,unsigned bytes=4){for(unsigned i=0;i<bytes;++i){if(fputc((int)(n&255),stdout)==EOF)return false;n>>=8;}return true;}
static bool vehicle(const OriginalVehicle&v){
#define WIRE_SCALAR(type,name,offset) if(!word((uint32_t)v.name,sizeof(type)))return false;
#define WIRE_ARRAY(type,name,count,offset) for(size_t j=0;j<count;++j)if(!word((uint32_t)v.name[j],sizeof(type)))return false;
#define WIRE_STRING(name,count,offset) if(fwrite(v.name,1,count,stdout)!=count)return false;
#include "original_vehicle_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
return true;}

static bool unchanged(const OriginalWorld&a,const OriginalWorld&b,size_t id,uint32_t raw){
 if(!collateral(a,b)||memcmp(a.current_monuments().data,b.current_monuments().data,0x27a0)||memcmp(a.tile(0),b.tile(0),a.game().map.info.tile_count*sizeof(OriginalMapTile))||memcmp(a.faction(0),b.faction(0),8*sizeof(OriginalFactionState)))return false;
 for(size_t j=0;j<a.game().vehicle_count;++j){auto expected=*a.vehicle(j);if(j==id)expected.morale=(uint8_t)raw;if(memcmp(&expected,b.vehicle(j),sizeof expected))return false;}
 return true;
}
int main(int argc,char**argv){if(argc!=2)return 20;FILE*f=fopen(argv[1],"rb");uint32_t count;if(!f||!rd(f,count))return 20;
for(uint32_t row=0;row<count;++row){uint32_t len,missile,defense,offense,current;if(!rd(f,len)||!rd(f,missile)||!rd(f,defense)||!rd(f,offense)||!rd(f,current)||len>64u*1024u*1024u)return 21;auto*data=(uint8_t*)malloc(len);if(!data||fread(data,1,len,f)!=len)return 21;OriginalWorld w;if(w.load(data,len,0xabcdef01)!=WorldLoadError::ok)return 22;free(data);
 units={};tech={};tech.count=89;bool expansion=w.game().data_version>=11;units.weapon_count=expansion?26:21;units.armor_count=expansion?14:10;units.ability_count=expansion?29:24;units.chassis[0].speed=1;units.chassis[0].missile=1;units.armor[0].defense=signed32(defense);units.weapons[0].offense=signed32(offense);
 // CURRENT records intentionally differ from the immutable wire baseline.
 const_cast<OriginalMapTile*>(w.tile(0))->contour^=1;const_cast<OriginalBase*>(w.base(0))->state_flags^=0x400;const_cast<OriginalPrototype*>(w.prototype(510))->name[0]^=1;const_cast<OriginalFactionState*>(w.faction(7))->energy_credits^=0x234;const_cast<uint8_t*>(w.current_monuments().data)[7]^=0xa5;
 alignas(WorldPromotionPlan) PromotionRuntime run{EngagementObservation::supplied,expansion?Variant::alien_crossfire:Variant::base,-1};WorldPromotionPlan out;
 // Seed output using an independent valid world, so every later rejection must
 // preserve a previously owned frame rather than an empty sentinel only.
 OriginalWorld seed;const uint8_t*src=w.original_bytes();if(seed.load(src,w.original_size(),0xabcdef01)!=WorldLoadError::ok)return 23;auto*sv=const_cast<OriginalVehicle*>(seed.vehicle(0));sv->faction_id=1;sv->home_base_id=-1;auto*sp=const_cast<OriginalPrototype*>(seed.prototype((size_t)sv->unit_id));sp->plan=0;sp->chassis_id=0;sp->weapon_id=0;sp->armor_id=0;sp->reactor_id=1;
 if(stage_world_promotion(seed,units,tech,0,run,out)!=PromotionError::ok)return 24;
 const auto old=fingerprint(*out.frame());const auto before=fingerprint(w);units.chassis[0].missile=(int32_t)missile;run.current_player=signed32(current);
 const auto status=stage_world_promotion(w,units,tech,0,run,out);if(fingerprint(w)!=before)return 25;word((uint32_t)status);
 if(status==PromotionError::ok){const auto&s=*out.summary();if(!unchanged(w,*out.frame(),0,s.raw_after))return 26;word(s.raw_before);word(s.raw_after);word(s.effective_before);word(s.queried_morale);word(s.promoted);for(size_t j=0;j<w.game().vehicle_count;++j)if(!vehicle(*out.frame()->vehicle(j)))return 27;
  const auto stable=fingerprint(*out.frame());WorldPromotionPlan next;run.current_player=-1;const auto again=stage_world_promotion(*out.frame(),units,tech,0,run,next);word((uint32_t)again);word(again==PromotionError::ok?next.summary()->raw_after:0xffffffffu);if(fingerprint(*out.frame())!=stable)return 28;
  const auto original=run;run.observation=EngagementObservation::missing;if(stage_world_promotion(w,units,tech,0,run,out)!=PromotionError::bad_state||fingerprint(*out.frame())!=stable)return 29;run=original;
  if(stage_world_promotion(w,units,tech,0,run,out,0)!=PromotionError::out_of_memory||fingerprint(*out.frame())!=stable)return 30;
  run.current_player=-2;if(stage_world_promotion(w,units,tech,0,run,out)!=PromotionError::bad_state||fingerprint(*out.frame())!=stable)return 31;run=original;
  run.edition=expansion?Variant::base:Variant::alien_crossfire;if(stage_world_promotion(w,units,tech,0,run,out)!=PromotionError::unsupported||fingerprint(*out.frame())!=stable)return 32;run=original;
  if(stage_world_promotion(w,units,tech,0,*reinterpret_cast<const PromotionRuntime*>(out.frame()->vehicle(0)),out)!=PromotionError::bad_state||fingerprint(*out.frame())!=stable)return 33;
  if(stage_world_promotion(w,units,tech,w.game().vehicle_count,run,out)!=PromotionError::bad_state||fingerprint(*out.frame())!=stable)return 38;
  const auto oldcount=tech.count;tech.count=88;if(stage_world_promotion(w,units,tech,0,run,out)!=PromotionError::bad_state||fingerprint(*out.frame())!=stable)return 39;tech.count=oldcount;
  const auto oldmissile=units.chassis[0].missile;units.chassis[0].missile=256;if(stage_world_promotion(w,units,tech,0,run,out)!=PromotionError::bad_state||fingerprint(*out.frame())!=stable)return 40;units.chassis[0].missile=oldmissile;
  const auto oldarmor=units.armor[0].defense;units.armor[0].defense=128;if(stage_world_promotion(w,units,tech,0,run,out)!=PromotionError::bad_state||fingerprint(*out.frame())!=stable)return 41;units.armor[0].defense=oldarmor;
  const auto oldoff=units.weapons[0].offense;units.weapons[0].offense=-129;if(stage_world_promotion(w,units,tech,0,run,out)!=PromotionError::bad_state||fingerprint(*out.frame())!=stable)return 42;units.weapons[0].offense=oldoff;
  const size_t exact=out.frame()->memory_bytes()-sizeof(OriginalWorld)+sizeof(WorldPromotionPlan);WorldPromotionPlan tight;
  if(stage_world_promotion(w,units,tech,0,run,tight,exact-1)!=PromotionError::out_of_memory||tight.ready()||fingerprint(*out.frame())!=stable)return 43;
  if(stage_world_promotion(w,units,tech,0,run,tight,exact)!=PromotionError::ok||!unchanged(w,*tight.frame(),0,s.raw_after))return 44;
  void*inputs[]={&units,&tech,&run};const size_t sizes[]={sizeof units,sizeof tech,sizeof run};
  for(unsigned j=0;j<3;++j){const auto h=hashbytes(2166136261u,inputs[j],sizes[j]);
#ifdef __SANITIZE_ADDRESS__
   __asan_poison_memory_region(inputs[j],sizes[j]);
#endif
   const auto rejected=stage_world_promotion(w,units,tech,0,run,*reinterpret_cast<WorldPromotionPlan*>(inputs[j]));
#ifdef __SANITIZE_ADDRESS__
   __asan_unpoison_memory_region(inputs[j],sizes[j]);
#endif
   if(rejected!=PromotionError::bad_state||hashbytes(2166136261u,inputs[j],sizes[j])!=h||fingerprint(*out.frame())!=stable)return 37;
  }
  // Source lifetime ends here; frame must retain original bytes, current
  // monuments, mutated decoded records and rebased span/history ownership.
  w.clear();seed.clear();if(fingerprint(*out.frame())!=stable)return 34;
 }else if(fingerprint(*out.frame())!=old)return 35;
}
return ferror(stdout)?36:0;}
