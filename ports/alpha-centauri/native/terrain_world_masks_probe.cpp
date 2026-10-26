/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_world_masks.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char**argv){
 if(argc!=2&&argc!=5)return 20;
 FILE*f=fopen(argv[1],"rb");if(!f)return 20;if(fseek(f,0,2)){fclose(f);return 20;}long n=ftell(f);rewind(f);if(n<=0||n>64*1024*1024){fclose(f);return 20;}auto*p=(uint8_t*)malloc(n);if(!p){fclose(f);return 20;}const bool ok=fread(p,1,n,f)==(size_t)n;fclose(f);alpha::OriginalWorld w;const bool loaded=ok&&w.load(p,n,1)==alpha::WorldLoadError::ok;free(p);if(!loaded)return 20;
 const auto&m=w.game().map.info;alpha::TerrainMaskView view{0,true};alpha::TerrainWorldMasks out{};
 const void*ptrs[]={&w,w.original_bytes(),w.tile(0),w.vehicle(0),w.base(0),w.prototype(0),w.faction(0),w.faction_definition(0),&view};
 for(const void*ptr:ptrs)if(ptr){unsigned char before[sizeof out];memcpy(before,ptr,sizeof out);for(unsigned offset=0;offset<2;++offset){auto&alias=*reinterpret_cast<alpha::TerrainWorldMasks*>(const_cast<uint8_t*>(static_cast<const uint8_t*>(ptr))+offset);if(alpha::world_terrain_masks(w,0,0,view,alias)||memcmp(before,ptr,sizeof out))return 20;}}
 memset(&out,99,sizeof out);view.supplied=false;if(alpha::world_terrain_masks(w,0,0,view,out))return 20;for(uint8_t b:out.coast)if(b!=99)return 20;if(out.roughness!=99||out.fungus!=99||out.jungle!=99||out.forest!=99)return 20;view.supplied=true;
 if(argc==5){int x=atoi(argv[2]),y=atoi(argv[3]),mode=atoi(argv[4]);view.supplied=mode!=2;const auto*t=x>=0&&y>=0&&(unsigned)x<m.width&&(unsigned)y<m.height?w.tile(((size_t)y*m.width+(unsigned)x)/2):nullptr;view.displayed_items=mode==0&&t?t->items:0;alpha::TerrainWorldMasks answer{};bool valid=alpha::world_terrain_masks(w,x,y,view,answer);printf("%u",valid?1u:0u);for(uint8_t v:answer.coast)printf(" %u",(unsigned)v);printf(" %u %u %u\n",(unsigned)answer.roughness,(unsigned)answer.fungus,(unsigned)answer.jungle);return 0;}
 for(unsigned mode=0;mode<2;++mode)for(unsigned y=0;y<m.height;++y)for(unsigned x=y&1;x<m.width;x+=2){view.displayed_items=mode==0?w.tile(((size_t)y*m.width+x)/2)->items:0;alpha::TerrainWorldMasks answer{};if(!alpha::world_terrain_masks(w,x,y,view,answer))return 20;for(uint8_t v:answer.coast)putchar(v);putchar(answer.roughness);putchar(answer.fungus);putchar(answer.jungle);}
 return 0;
}
