/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_dispatch.h"
namespace alpha {namespace {
void transition(unsigned raw,unsigned&variant,unsigned&rotation){
 // Geometric blob classes: cardinal edges and only their enclosed corners.
 // These15 atlas roles describe rotations, not copied executable table bytes.
 const unsigned shapes[]={0,2,10,14,34,42,46,58,62,170,171,175,187,191,255};
 if(raw==202||raw==218){variant=11;rotation=2;return;}
 unsigned mask=raw;for(unsigned k=0;k<8;k+=2)if(!(mask&(1u<<((k+7)&7)))||!(mask&(1u<<((k+1)&7))))mask&=~(1u<<k);
 const unsigned order[]={0,3,2,1};for(unsigned r:order){const unsigned m=r?((mask<<(2*r))|(mask>>(8-2*r)))&255:mask;for(unsigned i=0;i<15;++i)if(m==shapes[i]){variant=i;rotation=r;return;}}
 variant=rotation=0;
}
void uv(unsigned k,TerrainUv*out){
 // Pixel-centred triangle seams in the owner's56x56 atlas format.
 out[0]={(int32_t)(27+(k>=2)),(int32_t)(k==0?28:k==3?29:27)};
 switch(k){case 0:out[1]={0,55};out[2]={0,1};break;case 1:out[1]={0,0};out[2]={54,0};break;case 2:out[1]={55,1};out[2]={55,53};break;default:out[1]={55,55};out[2]={2,55};break;}
}
}
bool terrain_dispatch(const TerrainDispatchContext&input,TerrainDispatch&out){
 const uintptr_t a=(uintptr_t)&input,b=(uintptr_t)&out;
 if(sizeof(input)>UINTPTR_MAX-a||sizeof(out)>UINTPTR_MAX-b||(a<b+sizeof(out)&&b<a+sizeof(input)))return false;
 const TerrainDispatchContext c=input;if(!c.source_address_supplied||c.rockiness>2||c.moisture>2)return false;
 TerrainDispatch result{};const uint32_t state=c.original_tile_address*UINT32_C(1664525)+UINT32_C(1013904223);result.variation=(uint8_t)(((state&65535u)*2u)>>16);
 unsigned variant=0,rotation=0;if(c.rockiness)transition(c.transition_mask,variant,rotation);if(c.farm)rotation=0;
 result.variant=(uint8_t)variant;result.rotation=(uint8_t)rotation;
 if(c.farm&&c.farm_rendering){result.atlas_x=(uint16_t)(775+57*c.rockiness);result.atlas_y=(uint16_t)(219+57*result.variation);}
 else if(c.dunes){result.atlas_x=229;result.atlas_y=1;result.rotation=0;}
 else if(c.rockiness){result.atlas_x=(uint16_t)(1+57*(variant%4));result.atlas_y=(uint16_t)((c.rockiness==1?115:343)+57*(variant/4));}
 else{result.atlas_x=1;result.atlas_y=58;}
 result.overlay=c.moisture>0;if(result.overlay){const unsigned index=2*c.moisture+result.variation-2;result.overlay_x=(uint16_t)(1+57*((index&1)*2+index/2));result.overlay_y=1;}
 result.fog_colour=c.visible?-1:65;
 for(unsigned k=0;k<4;++k)uv((k+result.rotation)&3,result.uv[k]);
 out=result;return true;
}
}
