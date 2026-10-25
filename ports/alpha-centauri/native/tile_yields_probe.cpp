/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "tile_yields.h"
#include <stdio.h>
#include <string.h>
using namespace alpha;
static uint32_t le(const unsigned char* p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void put(unsigned char* p,uint32_t n){for(unsigned i=0;i<4;++i)p[i]=(unsigned char)(n>>(8*i));}
int main(int argc,char** argv){
 if(argc!=3)return 20;
 TileYieldContext alias{};TileYieldContext alias_before;memcpy(&alias_before,&alias,sizeof alias);
 if(calculate_tile_yields(alias,*reinterpret_cast<TileYieldResult*>(&alias))!=TileYieldError::bad_buffer || memcmp(&alias,&alias_before,sizeof alias))return 21;
 FILE* existing=fopen(argv[2],"rb");if(existing){fclose(existing);return 20;}
 FILE* in=fopen(argv[1],"rb");if(!in)return 20;
 unsigned char header[12];if(fread(header,1,12,in)!=12 || memcmp(header,"ATYLD1\0\0",8) || le(header+8)>1000000){fclose(in);return 20;}
 FILE* out=fopen(argv[2],"wb");if(!out){fclose(in);return 20;}
 const uint32_t count=le(header+8);bool good=true;
 for(uint32_t row=0;row<count && good;++row){
  unsigned char raw[59*4],encoded[40];uint32_t v[59];if(fread(raw,1,sizeof raw,in)!=sizeof raw){good=false;break;}
  for(unsigned i=0;i<59;++i)v[i]=le(raw+4*i);
  bool valid=v[1]<=255 && v[2]<=255 && v[3]<=255 && !(v[20]&~15u);
  const unsigned booleans[]={7,8,15,16,17,21,22,23,25};for(unsigned i:booleans)if(v[i]>1)valid=false;
  TileYieldContext c{};c.variant=(Variant)v[0];c.tile.climate=(uint8_t)v[1];c.tile.val2=(uint8_t)v[2];c.tile.val3=(uint8_t)v[3];c.tile.items=v[4];c.tile.landmarks=v[5];c.bonus=v[6];c.expansion_enabled=v[7]!=0;c.terraform_preview=v[8]!=0;c.planet_pending=(int32_t)v[9];c.economy_pending=(int32_t)v[10];
  for(unsigned i=0;i<3;++i)c.fungus[i]=(int32_t)v[11+i];
  c.base_count=(int32_t)v[14];c.golden_age=v[15]!=0;c.manifold_owned=v[16]!=0;c.aquatic=v[17]!=0;c.facilities=(uint64_t)v[18]|(uint64_t)v[19]<<32;
  for(unsigned i=0;i<3;++i)c.allow_three[i]=(v[20]&(1u<<i))!=0;
  c.sea_mineral_technology=(v[20]&8u)!=0;c.governor=v[21]!=0;c.merchant_exchange=v[22]!=0;c.dust_cloud=v[23]!=0;c.solar_flares=v[24];c.control_upkeep=v[25]!=0;c.echelon_matches=v[26];c.mine_nutrient_effect=(int32_t)v[27];c.mine_without_road_limit=(int32_t)v[28];c.base_event_flags=v[29];c.terraform_reduction_before=(int32_t)v[30];c.base_energy_before=(int32_t)v[31];
  for(unsigned i=0;i<9;++i)for(unsigned j=0;j<3;++j)c.resources[i][j]=(int32_t)v[32+i*3+j];
  TileYieldResult result;memset(&result,0x5a,sizeof result);TileYieldResult before;memcpy(&before,&result,sizeof before);
  const auto error=valid ? calculate_tile_yields(c,result) : TileYieldError::bad_state;
  put(encoded,(uint32_t)error);
  if(error==TileYieldError::ok){const int32_t fields[]={result.nutrient,result.mineral,result.energy,result.terraform_reduction,result.base_energy,result.reduction_by_resource[0],result.reduction_by_resource[1],result.reduction_by_resource[2],result.base_energy_written ? 1 : 0};for(unsigned i=0;i<9;++i)put(encoded+4+i*4,(uint32_t)fields[i]);
  }else{if(memcmp(&result,&before,sizeof before)){good=false;break;}memset(encoded+4,0x5a,36);}
  if(fwrite(encoded,1,sizeof encoded,out)!=sizeof encoded)good=false;
 }
 if(fgetc(in)!=EOF)good=false;
 if(fclose(in))good=false;
 if(fclose(out))good=false;
 return good ? 0 : 20;
}
