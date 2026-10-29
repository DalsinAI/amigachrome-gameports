/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "energy_convoy_orders.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char* read(const char* path,size_t& n){FILE* f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long z=ftell(f);rewind(f);if(z<=0 || z>64*1024*1024){fclose(f);return nullptr;}auto* p=(unsigned char*)malloc((size_t)z);if(!p){fclose(f);return nullptr;}bool ok=fread(p,1,(size_t)z,f)==(size_t)z;fclose(f);if(!ok){free(p);return nullptr;}n=(size_t)z;return p;}
static int32_t word(const unsigned char* p){uint32_t n=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);int32_t s;memcpy(&s,&n,4);return s;}
static bool emit(int32_t n){uint32_t v=(uint32_t)n;unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)v;v>>=8;}return fwrite(b,1,4,stdout)==4;}
static uint32_t hash(const alpha::OriginalWorld& w){uint32_t h=2166136261;auto add=[&](const void* p,size_t n){const auto* b=(const unsigned char*)p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;};add(&w,sizeof w);if(w.loaded()){const auto& g=w.game();add(w.original_bytes(),w.original_size());add(w.current_monuments().data,w.current_monuments().size);add(w.tile(0),g.map.info.tile_count*sizeof(alpha::OriginalMapTile));add(w.vehicle(0),g.vehicle_count*sizeof(alpha::OriginalVehicle));add(w.base(0),g.base_count*sizeof(alpha::OriginalBase));add(w.prototype(0),512*sizeof(alpha::OriginalPrototype));add(w.faction(0),8*sizeof(alpha::OriginalFactionState));add(w.faction_definition(0),8*sizeof(alpha::OriginalFactionDefinition));}return h;}
static void fill(alpha::EnergyConvoyOrders& r){r.visited=r.changed_orders=0x31415926;memset(r.clear_order,0x5a,sizeof r.clear_order);}
static bool sentinel(const alpha::EnergyConvoyOrders& r){if(r.visited!=0x31415926 || r.changed_orders!=0x31415926)return false;for(auto b:r.clear_order)if(b!=0x5a)return false;return true;}
int main(int argc,char** argv){if(argc!=4 || (strcmp(argv[1],"base") && strcmp(argv[1],"expansion")))return 20;
 const auto profile=strcmp(argv[1],"base")?alpha::Variant::alien_crossfire:alpha::Variant::base;size_t n=0;auto* bytes=read(argv[2],n);if(!bytes)return 20;alpha::OriginalWorld w;bool ok=w.load(bytes,n,0xabcdef01)==alpha::WorldLoadError::ok;free(bytes);if(!ok)return 20;
 const auto before=hash(w);FILE* f=fopen(argv[3],"rb");if(!f)return 20;unsigned char record[8];
 for(;;){size_t got=fread(record,1,8,f);if(!got){ok=!ferror(f);break;}if(got!=8)return 20;alpha::EnergyConvoyOrders r{};fill(r);auto e=alpha::energy_convoy_orders(w,profile,(size_t)(uint32_t)word(record+4),word(record),r);
  if((e!=alpha::LogicError::ok && !sentinel(r)) || hash(w)!=before || w.runtime_seed()!=0xabcdef01)return 21;
  if(!emit((int32_t)e) || !emit((int32_t)r.visited) || !emit((int32_t)r.changed_orders) || fwrite(r.clear_order,1,4096,stdout)!=4096)return 20;
 }
 fclose(f);if(!ok)return 20;alpha::EnergyConvoyOrders r{};fill(r);alpha::OriginalWorld empty;
 if(alpha::energy_convoy_orders(empty,profile,0,-1,r)!=alpha::LogicError::bad_state || !sentinel(r))return 21;
 const void* aliases[]={&w,w.original_bytes(),w.tile(0),w.vehicle(0),w.base(0),w.prototype(0),w.faction(0),w.faction_definition(0),w.current_monuments().data};
 for(const void* p:aliases)if(p){auto& out=*reinterpret_cast<alpha::EnergyConvoyOrders*>(const_cast<void*>(p));if(alpha::energy_convoy_orders(w,profile,0,-1,out)!=alpha::LogicError::bad_state || hash(w)!=before)return 21;}
 if(alpha::energy_convoy_orders(w,profile==alpha::Variant::base?alpha::Variant::alien_crossfire:alpha::Variant::base,0,-1,r)!=alpha::LogicError::bad_state || !sentinel(r))return 21;
 return 0;
}
