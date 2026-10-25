/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "base_minerals.h"
#include "upstream_minerals_reference.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static uint32_t word(const uint8_t* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int32_t signed_word(const uint8_t* p){uint32_t n=word(p);return n<=0x7fffffffu?(int32_t)n:-1-(int32_t)(0xffffffffu-n);}
static void put(uint8_t* p,uint32_t n){for(unsigned i=0;i<4;++i){p[i]=(uint8_t)n;n>>=8;}}
static bool same(const MineralResult& a,const MineralResult& b){return a.intake==b.intake && a.consumption==b.consumption && a.surplus==b.surplus && a.final_surplus==b.final_surplus && a.eco_damage==b.eco_damage && a.bonus==b.bonus && a.inefficiency==b.inefficiency;}
static bool alias_controls(uint32_t& count){
 struct Frame{MineralResult before;MineralInput input;MineralResult after;};
 Frame frame;memset(&frame,0x5a,sizeof frame);MineralInput input{};memcpy(&frame.input,&input,sizeof input);
 unsigned char saved[sizeof frame];memcpy(saved,&frame,sizeof frame);
 for(int offset=-(int)sizeof(MineralResult)+4;offset<(int)sizeof(MineralInput);offset+=4){
  auto& output=*reinterpret_cast<MineralResult*>(reinterpret_cast<unsigned char*>(&frame)+offsetof(Frame,input)+offset);
  if(calculate_minerals(frame.input,output)!=LogicError::bad_state || memcmp(saved,&frame,sizeof frame))return false;
  ++count;
 }
 MineralResult expected{};if(calculate_minerals(input,expected)!=LogicError::ok)return false;
 if(calculate_minerals(frame.input,frame.before)!=LogicError::ok || !same(expected,frame.before) || memcmp(&frame.input,&input,sizeof input))return false;
 if(calculate_minerals(frame.input,frame.after)!=LogicError::ok || !same(expected,frame.after) || memcmp(&frame.input,&input,sizeof input))return false;
 return true;
}
int main(int argc,char** argv){
 if(argc!=3)return 20;
 uint32_t alias_rejections=0;if(!alias_controls(alias_rejections))return 21;
 FILE* exists=fopen(argv[2],"rb");if(exists){fclose(exists);return 20;}
 FILE* f=fopen(argv[1],"rb");if(!f)return 20;
 uint8_t header[8];if(fread(header,1,8,f)!=8 || memcmp(header,"MNR1",4)){fclose(f);return 20;}
 const uint32_t count=word(header+4);if(!count || count>65536){fclose(f);return 20;}
 auto* output=(uint8_t*)malloc((size_t)count*32);if(!output){fclose(f);return 20;}
 uint32_t rejected=0,digest=2166136261;
 for(uint32_t i=0;i<count;++i){uint8_t row[80];if(fread(row,1,80,f)!=80){free(output);fclose(f);return 20;}
  MineralInput in{};
  in.intake=signed_word(row+0);
  in.convoy_to=signed_word(row+4);
  in.convoy_from=signed_word(row+8);
  in.maintenance=signed_word(row+12);
  in.raw_eco_damage=signed_word(row+16);
  in.population=signed_word(row+20);
  in.clean_minerals_modifier=signed_word(row+24);
  in.satellites=signed_word(row+28);
  in.atrocities=signed_word(row+32);
  in.tectonic_detonations=signed_word(row+36);
  in.sea_level=signed_word(row+40);
  in.sea_level_rises=signed_word(row+44);
  in.tech_ranking=signed_word(row+48);
  in.transcendent_techs=signed_word(row+52);
  in.planet=signed_word(row+56);
  in.native_life=signed_word(row+60);
  in.faction_difficulty=signed_word(row+64);
  in.global_difficulty=signed_word(row+68);
  in.queue_id=signed_word(row+72);
  const uint32_t flags=word(row+76);
  in.expansion=(flags&(1u<<14))!=0;
  in.human=(flags&(1u<<0))!=0;
  in.ascending=(flags&(1u<<1))!=0;
  in.transcendence_enabled=(flags&(1u<<2))!=0;
  in.perihelion=(flags&(1u<<3))!=0;
  in.quantum_converter=(flags&(1u<<4))!=0;
  in.robotic_assembly=(flags&(1u<<5))!=0;
  in.genejack=(flags&(1u<<6))!=0;
  in.nanoreplicator=(flags&(1u<<7))!=0;
  in.singularity_inductor=(flags&(1u<<8))!=0;
  in.bulk_matter=(flags&(1u<<9))!=0;
  in.centauri_preserve=(flags&(1u<<10))!=0;
  in.temple=(flags&(1u<<11))!=0;
  in.pholus_mutagen=(flags&(1u<<12))!=0;
  in.space_elevator=(flags&(1u<<13))!=0;
  const MineralResult sentinel{0x31415926,0x31415926,0x31415926,0x31415926,0x31415926,0x31415926,0x31415926};
  MineralResult result=sentinel;const auto error=(flags&~0x7fffu)?LogicError::bad_state:calculate_minerals(in,result);
  if(error==LogicError::ok){if(!same(result,minerals_reference::calculate(in))){free(output);fclose(f);return 21;}}
  else{++rejected;if(!same(result,sentinel)){free(output);fclose(f);return 21;}}
  auto* p=output+(size_t)i*32;put(p,(uint32_t)error);
  put(p+4,(uint32_t)result.intake);
  put(p+8,(uint32_t)result.consumption);
  put(p+12,(uint32_t)result.surplus);
  put(p+16,(uint32_t)result.final_surplus);
  put(p+20,(uint32_t)result.eco_damage);
  put(p+24,(uint32_t)result.bonus);
  put(p+28,(uint32_t)result.inefficiency);
 }
 if(fgetc(f)!=EOF){free(output);fclose(f);return 20;}fclose(f);
 for(size_t i=0;i<(size_t)count*32;++i)digest=(digest^output[i])*16777619u;
 f=fopen(argv[2],"wb");if(!f){free(output);return 20;}
 const bool done=fwrite(output,32,count,f)==count;const bool closed=fclose(f)==0;free(output);if(!done || !closed)return 21;
 printf("mineral_cases=%lu rejected=%lu aliases=%lu adjacent=2 digest=%08lx rc=0\n",(unsigned long)count,(unsigned long)rejected,(unsigned long)alias_rejections,(unsigned long)digest);return 0;
}
