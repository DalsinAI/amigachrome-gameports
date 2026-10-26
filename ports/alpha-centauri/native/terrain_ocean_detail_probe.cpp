/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_ocean_detail.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static bool light_controls(){
 alignas(4) alpha::TerrainOceanDetails d{};int32_t table[80];for(unsigned i=0;i<80;++i)table[i]=(int)i-40;alpha::TerrainOceanLightTable t{table,80,true};alpha::TerrainOceanLights out{};for(unsigned s=0;s<4;++s)for(unsigned v=0;v<4;++v)d.value[s][v]=(uint8_t)(s*20+v*6);if(!alpha::light_ocean_details(d,t,out))return false;for(unsigned s=0;s<4;++s)for(unsigned v=0;v<4;++v)if(out.point[s][v]!=(int)d.value[s][v]-40)return false;auto before=out;
 t.supplied=false;if(alpha::light_ocean_details(d,t,out)||memcmp(&out,&before,sizeof out))return false;t.supplied=true;t.count=79;if(alpha::light_ocean_details(d,t,out))return false;t.count=80;table[79]=128;if(alpha::light_ocean_details(d,t,out))return false;table[79]=39;d.value[3][3]=80;if(alpha::light_ocean_details(d,t,out)||memcmp(&out,&before,sizeof out))return false;d.value[3][3]=78;
 const void*p[]={&d,&t,table+79};for(auto q:p)if(alpha::light_ocean_details(d,t,*reinterpret_cast<alpha::TerrainOceanLights*>((uintptr_t)q&~uintptr_t(3))))return false;return true;
}
int main(int argc,char**argv){if((argc!=2&&argc!=3&&argc!=4)||!light_controls())return 20;FILE*f=fopen(argv[1],"rb");if(!f)return 20;if(fseek(f,0,2)){fclose(f);return 20;}long n=ftell(f);rewind(f);if(n<=0||n>64*1024*1024){fclose(f);return 20;}auto*p=(uint8_t*)malloc(n);if(!p){fclose(f);return 20;}bool read=fread(p,1,n,f)==(size_t)n;fclose(f);alpha::OriginalWorld w;bool loaded=read&&w.load(p,n,1)==alpha::WorldLoadError::ok;free(p);if(!loaded)return 20;
 const auto&m=w.game().map.info;const void*ptrs[]={&w,w.original_bytes(),w.tile(0),w.vehicle(0),w.base(0),w.prototype(0),w.faction(0),w.faction_definition(0)};for(auto ptr:ptrs)if(ptr)for(unsigned k=0;k<2;++k)if(alpha::world_ocean_details(w,0,0,*reinterpret_cast<alpha::TerrainOceanDetails*>(const_cast<uint8_t*>(static_cast<const uint8_t*>(ptr))+k)))return 20;
 if(argc==4){alpha::TerrainOceanDetails d{};memset(&d,99,sizeof d);bool ok=alpha::world_ocean_details(w,atoi(argv[2]),atoi(argv[3]),d);printf("%u",ok?1u:0u);for(const auto&row:d.value)for(auto v:row)printf(" %u",(unsigned)v);putchar('\n');return 0;}
 int32_t values[80]{};if(argc==3){f=fopen(argv[2],"rb");if(!f)return 20;for(auto&v:values){uint8_t b[4];if(fread(b,1,4,f)!=4){fclose(f);return 20;}v=(int32_t)((uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24);}bool extra=fgetc(f)!=EOF;fclose(f);if(extra)return 20;}
 alpha::TerrainOceanLightTable table{values,80,true};for(unsigned y=0;y<m.height;++y)for(unsigned x=y&1;x<m.width;x+=2){alpha::TerrainOceanDetails d{};if(!alpha::world_ocean_details(w,x,y,d))return 20;alpha::TerrainOceanLights light{};if(argc==3&&!alpha::light_ocean_details(d,table,light))return 20;for(unsigned s=0;s<4;++s)for(unsigned v=0;v<4;++v){putchar(d.value[s][v]);if(argc==3)putchar((uint8_t)light.point[s][v]);}}
 return 0;
}
