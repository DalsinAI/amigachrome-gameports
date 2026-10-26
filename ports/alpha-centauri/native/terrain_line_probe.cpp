/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_line.h"
#include <stdio.h>
#include <string.h>
static int32_t get(const uint8_t*p){return(int32_t)((uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24);}
static bool controls(){uint8_t p[4096];memset(p,199,sizeof p);alpha::TerrainLineInput in{1,1,50,50,37};alpha::TerrainTextureSurface s{p,sizeof p,64,64,64};auto clean=[&](){for(auto b:p)if(b!=199)return false;return true;};auto reject=[&](){return !alpha::draw_terrain_line(in,s)&&clean();};in.x0=-4097;if(!reject())return false;in.x0=1;in.y1=4097;if(!reject())return false;in.y1=50;in.colour=256;if(!reject())return false;in.colour=37;s.bytes=4095;if(!reject())return false;s.bytes=4096;s.pitch=63;if(!reject())return false;s.pitch=64;s.width=0;if(!reject())return false;s.width=64;const void*q[]={&in,&s};for(auto ptr:q){s.pixels=(uint8_t*)const_cast<void*>(ptr);if(!reject())return false;s.pixels=(uint8_t*)((uintptr_t)ptr-4095);if(!reject())return false;}return true;}
int main(){if(!controls())return 20;uint8_t p[4096],b[20];for(;;){size_t n=fread(b,1,sizeof b,stdin);if(!n)break;if(n!=sizeof b)return 20;alpha::TerrainLineInput in{get(b),get(b+4),get(b+8),get(b+12),(unsigned)get(b+16)};memset(p,199,sizeof p);alpha::TerrainTextureSurface s{p,sizeof p,64,64,64};if(!alpha::draw_terrain_line(in,s))return 20;fwrite(p,1,sizeof p,stdout);}return 0;}
