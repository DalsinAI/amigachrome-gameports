/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "tile_yields.h"
#include <stdio.h>
#include <string.h>
using namespace alpha;
static uint32_t le(const unsigned char* p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void put(unsigned char* p,uint32_t n){for(unsigned i=0;i<4;++i)p[i]=(unsigned char)(n>>(8*i));}
int main(int argc,char** argv){
 if(argc!=3)return 20;
 FILE* existing=fopen(argv[2],"rb");if(existing){fclose(existing);return 20;}
 FILE* in=fopen(argv[1],"rb");if(!in)return 20;unsigned char header[12];
 if(fread(header,1,12,in)!=12 || memcmp(header,"ABONU1\0\0",8) || le(header+8)>1000000){fclose(in);return 20;}
 FILE* out=fopen(argv[2],"wb");if(!out){fclose(in);return 20;}bool good=true;const uint32_t count=le(header+8);
 for(uint32_t i=0;i<count && good;++i){unsigned char raw[36],encoded[8];uint32_t v[9];if(fread(raw,1,sizeof raw,in)!=sizeof raw){good=false;break;}
  for(unsigned j=0;j<9;++j)v[j]=le(raw+4*j);
  OriginalMapInfo map{};map.width=v[0];map.height=v[1];map.random_seed=v[2];OriginalMapTile tile{};tile.climate=(uint8_t)v[6];tile.items=v[7];uint32_t result=0x5a5a5a5a;
  TileYieldError error=TileYieldError::bad_state;
  if(v[6]<=255 && v[8]<=2){
   if(v[8]==1){const uint32_t before=map.width;error=tile_resource_bonus(map,tile,(int32_t)v[3],(int32_t)v[4],(int32_t)v[5],map.width);if(error!=TileYieldError::bad_buffer || map.width!=before){good=false;break;}
   }else if(v[8]==2){const uint32_t before=tile.items;error=tile_resource_bonus(map,tile,(int32_t)v[3],(int32_t)v[4],(int32_t)v[5],tile.items);if(error!=TileYieldError::bad_buffer || tile.items!=before){good=false;break;}
   }else error=tile_resource_bonus(map,tile,(int32_t)v[3],(int32_t)v[4],(int32_t)v[5],result);
  }
  if(error!=TileYieldError::ok && result!=0x5a5a5a5a){good=false;break;}
  put(encoded,(uint32_t)error);put(encoded+4,result);if(fwrite(encoded,1,8,out)!=8)good=false;
 }
 if(fgetc(in)!=EOF)good=false;
 if(fclose(in))good=false;
 if(fclose(out))good=false;
 return good ? 0 : 20;
}
