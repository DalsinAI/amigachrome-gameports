/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_production_accrual.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t hash(const alpha::OriginalWorld& w){uint32_t h=2166136261;auto add=[&](const void* p,size_t n){const auto* b=(const unsigned char*)p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;};add(&w,sizeof w);if(w.loaded()){const auto& g=w.game();add(w.original_bytes(),w.original_size());add(w.tile(0),g.map.info.tile_count*sizeof(alpha::OriginalMapTile));add(w.vehicle(0),g.vehicle_count*sizeof(alpha::OriginalVehicle));add(w.base(0),g.base_count*sizeof(alpha::OriginalBase));add(w.prototype(0),512*sizeof(alpha::OriginalPrototype));add(w.faction(0),8*sizeof(alpha::OriginalFactionState));add(w.faction_definition(0),8*sizeof(alpha::OriginalFactionDefinition));}return h;}
static bool emit(uint32_t v){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)v;v>>=8;}return fwrite(b,1,4,stdout)==4;}
static alpha::ProductionAccrualResult sentinel(){return {0x31415926u,0x31415926,0x31415926,true,true};}
static bool untouched(const alpha::ProductionAccrualResult& r){return r.state_flags==0x31415926u && r.best_mineral_output==0x31415926 && r.minerals_accumulated==0x31415926 && r.negative_food_abort && r.accrual_branch;}
int main(int argc,char** argv){if(argc!=3 || (strcmp(argv[1],"base") && strcmp(argv[1],"expansion")))return 20;
 FILE* f=fopen(argv[2],"rb");if(!f)return 20;if(fseek(f,0,SEEK_END)){fclose(f);return 20;}long n=ftell(f);rewind(f);if(n<=0 || n>64*1024*1024){fclose(f);return 20;}auto* b=(uint8_t*)malloc((size_t)n);if(!b){fclose(f);return 20;}bool ok=fread(b,1,(size_t)n,f)==(size_t)n;fclose(f);if(!ok){free(b);return 20;}alpha::OriginalWorld w;ok=w.load(b,(size_t)n,0xabcdef01)==alpha::WorldLoadError::ok;free(b);if(!ok)return 20;
 const auto profile=strcmp(argv[1],"base")?alpha::Variant::alien_crossfire:alpha::Variant::base;const uint32_t before=hash(w);
 for(size_t i=0;i<w.game().base_count;++i){auto r=sentinel();auto e=alpha::world_production_accrual(w,profile,i,r);if((e!=alpha::LogicError::ok && !untouched(r)) || hash(w)!=before || w.runtime_seed()!=0xabcdef01)return 21;const uint32_t values[]={(uint32_t)e,r.state_flags,(uint32_t)r.best_mineral_output,(uint32_t)r.minerals_accumulated,(uint32_t)r.negative_food_abort,(uint32_t)r.accrual_branch};for(uint32_t v:values)if(!emit(v))return 20;}
 const void* aliases[]={&w,w.original_bytes(),w.tile(0),w.vehicle(0),w.base(0),w.prototype(0),w.faction(0),w.faction_definition(0)};
 for(const void* p:aliases)if(p){auto& r=*reinterpret_cast<alpha::ProductionAccrualResult*>(const_cast<void*>(p));if(alpha::world_production_accrual(w,profile,0,r)!=alpha::LogicError::bad_state || hash(w)!=before)return 21;}
 auto r=sentinel();alpha::OriginalWorld empty;
 if(alpha::world_production_accrual(empty,profile,0,r)!=alpha::LogicError::bad_state || !untouched(r))return 21;
 if(alpha::world_production_accrual(w,profile,w.game().base_count,r)!=alpha::LogicError::bad_state || !untouched(r))return 21;
 if(alpha::world_production_accrual(w,profile==alpha::Variant::base?alpha::Variant::alien_crossfire:alpha::Variant::base,0,r)!=alpha::LogicError::bad_state || !untouched(r))return 21;
 if(hash(w)!=before || w.runtime_seed()!=0xabcdef01)return 21;
 return 0;
}
