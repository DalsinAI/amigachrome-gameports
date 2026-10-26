/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_material.h"
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;
 return an&&bn&&x<y+bn&&y<x+an;
}
}
bool prepare_terrain_material(const TerrainMaterialSource&in,uint8_t*pixels,size_t bytes,uint8_t*rgb,size_t rgb_bytes){
 const size_t n=in.pixel_count;
 if(!in.pixels||!in.rgb||!pixels||!rgb||!n||n>16777216||bytes<n||in.rgb_bytes<768||rgb_bytes<768)return false;
 if(overlap(pixels,n,rgb,768)||overlap(pixels,n,&in,sizeof in)||overlap(rgb,768,&in,sizeof in)||overlap(pixels,n,in.pixels,n)||overlap(pixels,n,in.rgb,768)||overlap(rgb,768,in.pixels,n)||overlap(rgb,768,in.rgb,768))return false;
 for(size_t i=0;i<n;++i)pixels[i]=(uint8_t)(in.pixels[i]+10);
 for(unsigned i=0;i<236*3;++i)rgb[30+i]=in.rgb[i];
 return true;
}
}
