/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_dispatch.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <initializer_list>
uint32_t get(const uint8_t*b){return(uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;}
void put(uint32_t v){for(unsigned k=0;k<4;++k)putchar((v>>(k*8))&255);}
bool rejection_controls(){
 alpha::TerrainDispatchContext c{123,1,1,42,false,false,false,true,true};alpha::TerrainDispatch out;memset(&out,99,sizeof out);unsigned char before[sizeof out];memcpy(before,&out,sizeof out);
 auto reject=[&](){return !alpha::terrain_dispatch(c,out)&&!memcmp(before,&out,sizeof out);};
 c.source_address_supplied=false;if(!reject())return false;c.source_address_supplied=true;
 c.rockiness=3;if(!reject())return false;c.rockiness=1;c.moisture=3;if(!reject())return false;c.moisture=1;
 unsigned char saved[sizeof c];memcpy(saved,&c,sizeof c);
 for(unsigned offset: {0u,4u,(unsigned)sizeof(c)-4}){auto&alias=*reinterpret_cast<alpha::TerrainDispatch*>(reinterpret_cast<unsigned char*>(&c)+offset);if(alpha::terrain_dispatch(c,alias)||memcmp(saved,&c,sizeof c))return false;}
 return true;
}
int main(){if(!rejection_controls())return 20;uint8_t b[32];for(;;){size_t n=fread(b,1,32,stdin);if(!n)return 0;if(n!=32)return 20;
 alpha::TerrainDispatchContext c{get(b),(uint8_t)get(b+4),(uint8_t)get(b+8),(uint8_t)get(b+12),get(b+16)!=0,get(b+20)!=0,get(b+24)!=0,get(b+28)!=0,true};alpha::TerrainDispatch p{};if(!alpha::terrain_dispatch(c,p))return 20;
 put(p.atlas_x);put(p.atlas_y);put(p.overlay_x);put(p.overlay_y);put(p.variant);put(p.rotation);put(p.variation);put((uint32_t)(int32_t)p.fog_colour);put(p.overlay?1:0);for(auto&t:p.uv)for(auto&v:t){put(v.u);put(v.v);}
}}
