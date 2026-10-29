/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_territorial_combat.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifdef __SANITIZE_ADDRESS__
#include <sanitizer/asan_interface.h>
#endif
using namespace alpha;
static void word(uint32_t v){for(unsigned i=0;i<4;++i){putchar(v&255u);v>>=8;}}
static uint32_t hash(uint32_t h,const void*p,size_t n){auto*b=(const unsigned char*)p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
static uint32_t fingerprint(const OriginalWorld&w){uint32_t h=2166136261u;h=hash(h,w.original_bytes(),w.original_size());h=hash(h,w.current_monuments().data,w.current_monuments().size);h=hash(h,w.game().bunglist.data,w.game().bunglist.size);h=hash(h,&w.game().preferences,sizeof w.game().preferences);h=hash(h,w.tile(0),w.game().map.info.tile_count*sizeof(OriginalMapTile));h=hash(h,w.vehicle(0),w.game().vehicle_count*sizeof(OriginalVehicle));h=hash(h,w.base(0),w.game().base_count*sizeof(OriginalBase));h=hash(h,w.prototype(0),512*sizeof(OriginalPrototype));h=hash(h,w.faction(0),8*sizeof(OriginalFactionState));return hash(h,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));}
static bool collateral(const OriginalWorld&w,const OriginalWorld&f,const WorldTerritorySummary&s){
 if(w.original_size()!=f.original_size()||w.original_bytes()==f.original_bytes()||w.runtime_seed()!=f.runtime_seed()||memcmp(w.original_bytes(),f.original_bytes(),w.original_size())||memcmp(w.current_monuments().data,f.current_monuments().data,w.current_monuments().size)||memcmp(w.game().bunglist.data,f.game().bunglist.data,w.game().bunglist.size)||memcmp(&w.game().preferences,&f.game().preferences,sizeof w.game().preferences)||memcmp(&w.game().map.info,&f.game().map.info,sizeof w.game().map.info)||memcmp(w.tile(0),f.tile(0),w.game().map.info.tile_count*sizeof(OriginalMapTile))||memcmp(w.vehicle(0),f.vehicle(0),w.game().vehicle_count*sizeof(OriginalVehicle))||memcmp(w.base(0),f.base(0),w.game().base_count*sizeof(OriginalBase))||memcmp(w.prototype(0),f.prototype(0),512*sizeof(OriginalPrototype))||memcmp(w.faction_definition(0),f.faction_definition(0),8*sizeof(OriginalFactionDefinition)))return false;
 for(size_t i=0;i<8;++i){auto fs=*f.faction(i);if(s.effect.applied){if(i==(size_t)s.owner)fs.diplo_friction[s.attacker_faction]=w.faction(i)->diplo_friction[s.attacker_faction];if(i==(size_t)s.attacker_faction)fs.diplo_gifts[s.owner]=w.faction(i)->diplo_gifts[s.owner];}if(memcmp(&fs,w.faction(i),sizeof fs))return false;}return true;
}
int main(int argc,char**argv){if(argc!=2)return 10;FILE*file=fopen(argv[1],"rb");if(!file)return 10;fseek(file,0,SEEK_END);long n=ftell(file);rewind(file);if(n<1)return 10;auto*data=(uint8_t*)malloc((size_t)n);if(!data||fread(data,1,(size_t)n,file)!=(size_t)n)return 10;fclose(file);OriginalWorld w;if(w.load(data,(size_t)n,0x12345678)!=WorldLoadError::ok)return 11;
 const auto original=fingerprint(w);WorldTerritorialCombatPlan out;size_t last_a=0,last_b=1;alignas(WorldTerritorialCombatPlan) TerritoryRuntime last{};unsigned accepted=0;
 for(unsigned mode=0;mode<3;++mode)for(size_t a=0;a<3;++a)for(size_t b=0;b<3;++b){const auto*av=w.vehicle(a);const auto*dv=w.vehicle(b);if(!av||!dv)return 12;const auto*t=w.tile(((size_t)dv->y*w.game().map.info.width+(uint32_t)dv->x)/2);TerritoryRuntime r{TerritoryObservation::supplied,w.game().data_version>=11?Variant::alien_crossfire:Variant::base,mode==0?(uint32_t)(int32_t)t->owner:0xffffffffu,mode==0?av->faction_id:0xffffffffu,mode==2?0u:0x80000000u,0,0};const auto*old=out.frame();WorldTerritorySummary saved{};if(out.summary())saved=*out.summary();const auto e=stage_world_territorial_combat(w,a,b,r,out);word((uint32_t)e);
 if(e==WorldTerritoryError::ok){++accepted;last_a=a;last_b=b;last=r;const auto&s=*out.summary();if(!collateral(w,*out.frame(),s))return 13;word(s.tile_index);word((uint32_t)s.owner);word((uint32_t)s.attacker_faction);word((uint32_t)s.defender_faction);word(s.effect.applied);word((uint32_t)s.effect.gift_before);word((uint32_t)s.effect.gift_after);word((uint32_t)s.effect.friction_before);word((uint32_t)s.effect.friction_after);word(s.effect.negotiation_before);word(s.effect.negotiation_after);for(size_t f=0;f<8;++f){for(auto v:out.frame()->faction(f)->diplo_gifts)word((uint32_t)v);for(auto v:out.frame()->faction(f)->diplo_friction)word((uint32_t)v);}const auto prior=*out.summary();const auto*p=out.frame();const auto fp=fingerprint(*p);if(stage_world_territorial_combat(w,a,b,r,out,1)!=WorldTerritoryError::out_of_memory||out.frame()!=p||memcmp(&prior,out.summary(),sizeof prior)||fingerprint(*p)!=fp)return 14;
 }else{for(unsigned i=0;i<139;++i)word(0);if(out.frame()!=old||(old&&memcmp(&saved,out.summary(),sizeof saved)))return 15;}if(fingerprint(w)!=original)return 16;}
 if(accepted){const auto*p=out.frame();const auto fp=fingerprint(*p);const auto saved=*out.summary();for(unsigned i=0;i<6;++i){auto r=last;auto wanted=WorldTerritoryError::bad_state;if(i==0){r.observation=TerritoryObservation::missing;wanted=WorldTerritoryError::missing_runtime;}if(i==1){r.edition=r.edition==Variant::base?Variant::alien_crossfire:Variant::base;wanted=WorldTerritoryError::unsupported;}if(i==2){r.multiplayer_active=1;wanted=WorldTerritoryError::unsupported;}if(i==3){r.pbem_active=1;wanted=WorldTerritoryError::unsupported;}if(i==4)r.multiplayer_active=2;if(i==5)r.pbem_active=2;if(stage_world_territorial_combat(w,last_a,last_b,r,out)!=wanted||out.frame()!=p||fingerprint(*p)!=fp||memcmp(&saved,out.summary(),sizeof saved))return 17;}
 const auto snapshot=last;const uintptr_t address=(uintptr_t)&last;
#ifdef __SANITIZE_ADDRESS__
 __asan_poison_memory_region(&last,sizeof last);
#endif
 const auto bad=stage_world_territorial_combat(w,last_a,last_b,last,*reinterpret_cast<WorldTerritorialCombatPlan*>(address));
#ifdef __SANITIZE_ADDRESS__
 __asan_unpoison_memory_region(&last,sizeof last);
#endif
 if(bad!=WorldTerritoryError::bad_state||memcmp(&last,&snapshot,sizeof last))return 18;
 if(stage_world_territorial_combat(*p,last_a,last_b,last,out)!=WorldTerritoryError::bad_state||fingerprint(*p)!=fp)return 19;
 w.clear();memset(data,0,(size_t)n);if(fingerprint(*p)!=fp)return 20;
 }free(data);fprintf(stderr,"GUARDS %u\n",accepted);return 0;}
