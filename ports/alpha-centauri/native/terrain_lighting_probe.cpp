/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_lighting.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <initializer_list>
int main(int argc,char**argv){
 if(argc==1){
  for(unsigned k=0;k<4;++k)for(unsigned bad: {0u,8u}){uint8_t r[4]={4,4,4,4};r[k]=(uint8_t)bad;alpha::TerrainBrightness out;memset(&out,99,sizeof out);if(alpha::terrain_brightness(r,out))return 20;for(int8_t b:out.triangle)if(b!=99)return 20;}
  for(unsigned a=1;a<8;++a)for(unsigned b=1;b<8;++b)for(unsigned c=1;c<8;++c)for(unsigned d=1;d<8;++d){uint8_t r[4]={(uint8_t)a,(uint8_t)b,(uint8_t)c,(uint8_t)d};alpha::TerrainBrightness out{};if(!alpha::terrain_brightness(r,out))return 20;fwrite(out.triangle,1,4,stdout);}
  return 0;
 }
 if(argc!=2&&argc!=4)return 20;
 FILE*f=fopen(argv[1],"rb");if(!f)return 20;if(fseek(f,0,2)){fclose(f);return 20;}long n=ftell(f);rewind(f);if(n<=0||n>64*1024*1024){fclose(f);return 20;}auto*p=(uint8_t*)malloc(n);if(!p){fclose(f);return 20;}const bool ok=fread(p,1,n,f)==(size_t)n;fclose(f);alpha::OriginalWorld w;const bool loaded=ok&&w.load(p,n,1)==alpha::WorldLoadError::ok;free(p);if(!loaded)return 20;
 const auto&mi=w.game().map.info;
 bool checked=false;
 for(unsigned yy=0;!checked&&yy<mi.height;++yy)for(unsigned xx=yy&1;!checked&&xx<mi.width;xx+=2){
  alpha::TerrainPointLights value{};if(!alpha::world_terrain_lights(w,xx,yy,value))continue;
  const void*ptrs[]={&w,w.original_bytes(),w.tile(0),w.vehicle(0),w.base(0),w.prototype(0),w.faction(0),w.faction_definition(0)};
  for(const void*ptr:ptrs)if(ptr){unsigned char before[5];memcpy(before,ptr,5);for(unsigned offset=0;offset<2;++offset){auto&alias=*reinterpret_cast<alpha::TerrainPointLights*>(const_cast<uint8_t*>(static_cast<const uint8_t*>(ptr))+offset);if(alpha::world_terrain_lights(w,xx,yy,alias)||memcmp(before,ptr,5))return 20;}}
  checked=true;
 }
 if(argc==4){alpha::TerrainPointLights out{};bool valid=alpha::world_terrain_lights(w,atoi(argv[2]),atoi(argv[3]),out);printf("%u",valid?1u:0u);for(int8_t l:out.point)printf(" %d",(int)l);puts("");return 0;}
 const auto&m=w.game().map.info;for(unsigned y=0;y<m.height;++y)for(unsigned x=y&1;x<m.width;x+=2){alpha::TerrainPointLights out{};const bool valid=alpha::world_terrain_lights(w,x,y,out);putchar(valid?1:0);fwrite(out.point,1,5,stdout);}return 0;
}
