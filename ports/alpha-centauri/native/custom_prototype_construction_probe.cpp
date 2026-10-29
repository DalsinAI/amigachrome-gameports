/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "custom_prototype_construction.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static alpha::Rules tech;static alpha::UnitDefinitions defs;
static uint32_t get(const uint8_t* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static bool emit(uint32_t n){uint8_t b[4];for(unsigned i=0;i<4;++i){b[i]=(uint8_t)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
static uint8_t* read(const char* path,size_t& n){FILE* f=fopen(path,"rb");if(!f)return nullptr;fseek(f,0,SEEK_END);long k=ftell(f);rewind(f);if(k<=0 || k>64*1024*1024){fclose(f);return nullptr;}auto* b=(uint8_t*)malloc((size_t)k);if(!b){fclose(f);return nullptr;}n=(size_t)k;if(fread(b,1,n,f)!=n){free(b);b=nullptr;}fclose(f);return b;}
static uint32_t whole(const alpha::OriginalWorld& w,const alpha::CustomPrototypeRequest& q){uint32_t h=2166136261u;auto add=[&](const void* data,size_t n){const auto* p=(const uint8_t*)data;for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;};add(&w,sizeof w);add(w.original_bytes(),w.original_size());add(w.current_monuments().data,w.current_monuments().size);add(w.tile(0),w.game().map.info.tile_count*sizeof(alpha::OriginalMapTile));add(w.vehicle(0),w.game().vehicle_count*sizeof(alpha::OriginalVehicle));add(w.base(0),w.game().base_count*sizeof(alpha::OriginalBase));add(w.prototype(0),512*sizeof(alpha::OriginalPrototype));add(w.faction(0),8*sizeof(alpha::OriginalFactionState));add(w.faction_definition(0),8*sizeof(alpha::OriginalFactionDefinition));add(&tech,sizeof tech);add(&defs,sizeof defs);add(&q,sizeof q);return h;}
int main(int argc,char** argv){if(argc!=5)return 20;auto variant=(alpha::Variant)atoi(argv[4]);size_t n;auto* b=read(argv[2],n);if(!b)return 20;auto err=alpha::read_technologies(b,n,tech);if(err==alpha::Error::ok)err=alpha::read_unit_definitions(b,n,tech,variant,defs);free(b);if(err!=alpha::Error::ok)return 20;
 b=read(argv[1],n);if(!b)return 20;alpha::OriginalWorld w;if(w.load(b,n,123)!=alpha::WorldLoadError::ok)return 20;free(b);b=read(argv[3],n);if(!b || n!=24)return 20;alpha::CustomPrototypeRequest q{(int32_t)get(b),(int32_t)get(b+4),(int32_t)get(b+8),(int32_t)get(b+12),(int32_t)get(b+16),get(b+20)};free(b);const uint32_t before=whole(w,q);alpha::OriginalPrototype out;memset(&out,0x5a,sizeof out);auto error=alpha::world_construct_custom_prototype(w,variant,tech,defs,q,out);if(!emit((uint32_t)error))return 20;
 if(error==alpha::LogicError::ok){uint8_t wire[52]{};
#define WIRE_SCALAR(type,name,off) for(size_t j=0;j<sizeof(type);++j)wire[off+j]=(uint8_t)((uint32_t)out.name>>(j*8));
#define WIRE_STRING(name,count,off) memcpy(wire+off,out.name,count);
#define WIRE_ARRAY(type,name,count,off) for(unsigned i=0;i<count;++i)for(size_t j=0;j<sizeof(type);++j)wire[off+i*sizeof(type)+j]=(uint8_t)((uint32_t)out.name[i]>>(j*8));
#include "original_prototype_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_STRING
#undef WIRE_ARRAY
 if(fwrite(wire,1,52,stdout)!=52)return 20;
 const auto old=out;auto bad=q;bad.id=63;if(alpha::world_construct_custom_prototype(w,variant,tech,defs,bad,out)!=alpha::LogicError::bad_state || memcmp(&old,&out,sizeof out))return 21;
 }else{const auto* p=(const uint8_t*)&out;for(size_t i=0;i<sizeof out;++i)if(p[i]!=0x5a)return 21;}
 if(whole(w,q)!=before)return 21;
 const void* aliases[]={&w,&tech,&defs,&q,w.original_bytes(),w.current_monuments().data,w.tile(0),w.vehicle(0),w.base(0),w.prototype(0),w.faction(0),w.faction_definition(0)};
 for(const auto* a:aliases)if(a){auto& fake=*(alpha::OriginalPrototype*)const_cast<void*>(a);if(alpha::world_construct_custom_prototype(w,variant,tech,defs,q,fake)!=alpha::LogicError::bad_state)return 21;}
 return whole(w,q)==before?0:21;
}
