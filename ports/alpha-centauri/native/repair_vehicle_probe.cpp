/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "repair_vehicle.h"
#include <stdio.h>
#include <string.h>
static uint32_t get(const uint8_t*p,unsigned n){uint32_t v=0;for(unsigned i=0;i<n;++i)v|=(uint32_t)p[i]<<(8*i);return v;}
static int32_t sint(uint32_t v){int32_t n;memcpy(&n,&v,4);return n;}
static bool put(uint32_t n,unsigned size=4){uint8_t b[4];for(unsigned i=0;i<size;++i){b[i]=(uint8_t)n;n>>=8;}return fwrite(b,1,size,stdout)==size;}
int main(int argc,char**argv){
 if(argc!=2)return 20;
 FILE*f=fopen(argv[1],"rb");if(!f)return 20;
 alignas(alpha::RepairVehicleResult) uint8_t storage[sizeof(alpha::RepairVehicleFacts)+sizeof(alpha::RepairVehicleResult)]{};
 auto& alias=*reinterpret_cast<alpha::RepairVehicleFacts*>(storage);
 for(unsigned off=0;off<=4;off+=4)if(alpha::repair_vehicle(alias,*reinterpret_cast<alpha::RepairVehicleResult*>(storage+off))!=alpha::LogicError::bad_state)return 21;
 uint8_t bytes[100];unsigned count=0;
 for(;;){const size_t n=fread(bytes,1,sizeof bytes,f);if(!n){bool ok=!ferror(f);fclose(f);return ok?0:20;}if(n!=sizeof bytes || ++count>200000)return 20;
  alpha::RepairVehicleFacts in{};const uint32_t variant=get(bytes,4);in.variant=(alpha::Variant)variant;
  in.phase_faction=get(bytes+4,4);in.local_faction=get(bytes+8,4);in.turn=get(bytes+12,4);in.vehicle_index=get(bytes+16,4);
  in.triad=sint(get(bytes+20,4));in.reactor=sint(get(bytes+24,4));in.weapon_offense=sint(get(bytes+28,4));in.tile_items=get(bytes+32,4);in.tile_climate=(uint8_t)get(bytes+36,4);in.tile_val2=(uint8_t)get(bytes+40,4);const uint32_t flags=get(bytes+44,4);
  in.friendly_territory=flags&1;in.base_present=flags&2;in.base_riots=flags&4;in.native_breed_bonus=flags&8;in.repair_facility=flags&16;in.repair_project=flags&32;in.cargo_capacity_nonzero=flags&64;in.other_stack_repair_bay=flags&128;in.nano_factory=flags&256;in.xenoempathy_dome=flags&512;in.human=flags&1024;
#define WIRE_SCALAR(type,name,offset) {uint32_t u=get(bytes+48+offset,sizeof(type));if(sizeof(type)==2 && u&0x8000u)u|=0xffff0000u;in.vehicle.name=(type)sint(u);}
#define WIRE_ARRAY(type,name,count,offset) for(unsigned j=0;j<count;++j){uint32_t u=get(bytes+48+offset+j*sizeof(type),sizeof(type));if(sizeof(type)==2 && u&0x8000u)u|=0xffff0000u;in.vehicle.name[j]=(type)sint(u);}
#define WIRE_STRING(name,count,offset)
#include "original_vehicle_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
  alpha::RepairVehicleResult result;memset(&result,90,sizeof result);
  const auto error=variant>1?alpha::LogicError::bad_state:alpha::repair_vehicle(in,result);
  if(!put((uint32_t)error))return 20;
  if(error!=alpha::LogicError::ok){const auto*p=(const uint8_t*)&result;for(size_t i=0;i<sizeof result;++i)if(p[i]!=90)return 21;for(unsigned i=0;i<72;++i)if(!put(90,1))return 20;continue;}
#define WIRE_SCALAR(type,name,offset) if(!put((uint32_t)result.vehicle.name,sizeof(type)))return 20;
#define WIRE_ARRAY(type,name,count,offset) for(auto v:result.vehicle.name)if(!put((uint32_t)v,sizeof(type)))return 20;
#define WIRE_STRING(name,count,offset)
#include "original_vehicle_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
  if(!put(result.processed) || !put(result.healing_attempted) || !put(result.draw_requested) || !put(result.healing) || !put(result.damage_floor))return 20;
 }
}
