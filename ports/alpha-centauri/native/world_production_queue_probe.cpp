/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_production_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct Tables{alpha::Rules tech;alpha::UnitDefinitions units;alpha::FacilityDefinitions facilities;};
static unsigned char* read(const char* path,size_t& n){FILE* f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long z=ftell(f);rewind(f);if(z<=0 || z>64*1024*1024){fclose(f);return nullptr;}auto* p=(unsigned char*)malloc((size_t)z);if(!p){fclose(f);return nullptr;}bool ok=fread(p,1,(size_t)z,f)==(size_t)z;fclose(f);if(!ok){free(p);return nullptr;}n=(size_t)z;return p;}
static int32_t word(const unsigned char* p){uint32_t n=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);int32_t s;memcpy(&s,&n,4);return s;}
static bool emit(int32_t n){uint32_t v=(uint32_t)n;unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)v;v>>=8;}return fwrite(b,1,4,stdout)==4;}
static uint32_t hash(const alpha::OriginalWorld& w,const Tables& t){uint32_t h=2166136261;auto add=[&](const void* p,size_t n){const auto* b=(const unsigned char*)p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;};add(&w,sizeof w);add(&t,sizeof t);if(w.loaded()){const auto& g=w.game();add(w.original_bytes(),w.original_size());add(w.tile(0),g.map.info.tile_count*sizeof(alpha::OriginalMapTile));add(w.vehicle(0),g.vehicle_count*sizeof(alpha::OriginalVehicle));add(w.base(0),g.base_count*sizeof(alpha::OriginalBase));add(w.prototype(0),512*sizeof(alpha::OriginalPrototype));add(w.faction(0),8*sizeof(alpha::OriginalFactionState));add(w.faction_definition(0),8*sizeof(alpha::OriginalFactionDefinition));}return h;}
static void fill(alpha::WorldProductionQueue& r){r.iterations=0x31415926;r.available=r.project_notification_required=true;r.queue.queue_size=0x31415926;r.queue.shifted=true;for(auto& x:r.queue.items)x=0x31415926;memset(r.queue.units_queue,0x5a,512);}
static bool sentinel(const alpha::WorldProductionQueue& r){if(r.iterations!=0x31415926 || !r.available || !r.project_notification_required || r.queue.queue_size!=0x31415926 || !r.queue.shifted)return false;for(auto x:r.queue.items)if(x!=0x31415926)return false;for(auto x:r.queue.units_queue)if(x!=0x5a)return false;return true;}
int main(int argc,char** argv){if(argc!=5 || (strcmp(argv[1],"base") && strcmp(argv[1],"expansion")))return 20;
 auto* t=new Tables{};size_t n=0;auto* bytes=read(argv[2],n);if(!bytes)return 20;const auto profile=strcmp(argv[1],"base")?alpha::Variant::alien_crossfire:alpha::Variant::base;
 bool ok=alpha::read_technologies(bytes,n,t->tech)==alpha::Error::ok && alpha::read_unit_definitions(bytes,n,t->tech,profile,t->units)==alpha::Error::ok && alpha::read_facility_definitions(bytes,n,t->tech,profile,t->facilities)==alpha::Error::ok;free(bytes);if(!ok)return 20;
 bytes=read(argv[3],n);if(!bytes)return 20;alpha::OriginalWorld w;ok=w.load(bytes,n,0xabcdef01)==alpha::WorldLoadError::ok;free(bytes);if(!ok)return 20;
 const uint32_t before=hash(w,*t);FILE* f=fopen(argv[4],"rb");if(!f)return 20;unsigned char record[8];size_t count=0;
 for(;;){size_t got=fread(record,1,8,f);if(!got){ok=!ferror(f);break;}if(got!=8 || ++count>10000){ok=false;break;}
  alpha::WorldProductionQueue result{};fill(result);auto e=alpha::LogicError::bad_state;const int32_t exp=word(record);
  if(exp==0 || exp==1)e=alpha::world_production_queue(w,t->tech,t->units,t->facilities,profile,exp!=0,(size_t)(uint32_t)word(record+4),result);
  if((e!=alpha::LogicError::ok && !sentinel(result)) || hash(w,*t)!=before || w.runtime_seed()!=0xabcdef01)return 21;
  if(!emit((int32_t)e) || !emit((int32_t)result.iterations) || !emit(result.available) || !emit(result.project_notification_required) || !emit(result.queue.queue_size))return 20;
  for(auto x:result.queue.items)if(!emit(x))return 20;
  if(fwrite(result.queue.units_queue,1,512,stdout)!=512 || !emit(result.queue.shifted))return 20;
 }
 fclose(f);if(!ok)return 20;alpha::WorldProductionQueue result{};fill(result);alpha::OriginalWorld empty;
 if(alpha::world_production_queue(empty,t->tech,t->units,t->facilities,profile,false,0,result)!=alpha::LogicError::bad_state || !sentinel(result))return 21;
 const void* aliases[]={&w,&t->units,&t->tech,&t->facilities,w.original_bytes(),w.tile(0),w.vehicle(0),w.base(0),w.prototype(0),w.faction(0),w.faction_definition(0)};
 for(const void* p:aliases)if(p){auto& output=*reinterpret_cast<alpha::WorldProductionQueue*>(const_cast<void*>(p));if(alpha::world_production_queue(w,t->tech,t->units,t->facilities,profile,false,0,output)!=alpha::LogicError::bad_state || hash(w,*t)!=before)return 21;}
 if(alpha::world_production_queue(w,t->tech,t->units,t->facilities,profile==alpha::Variant::base?alpha::Variant::alien_crossfire:alpha::Variant::base,false,0,result)!=alpha::LogicError::bad_state || !sentinel(result))return 21;
 delete t;return 0;
}
