/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_feature_layers.h"
#include "terrain_keyed_quad.h"
#include "terrain_forest_quad.h"
#include <stdlib.h>
#include <string.h>
namespace alpha {namespace {
bool overlap(const void*a,size_t n,const void*b,size_t m){if(!n||!m)return false;uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(x<y+m&&y<x+n);}
// Same independently recovered blob geometry as the existing ground-material
// stage. This pure selector has no tile-pointer RNG or gameplay-RNG dependency.
void transition(unsigned raw,unsigned&variant,unsigned&rotation){
 const unsigned shapes[]={0,2,10,14,34,42,46,58,62,170,171,175,187,191,255};
 if(raw==202||raw==218){variant=11;rotation=2;return;}
 unsigned mask=raw;for(unsigned k=0;k<8;k+=2)if(!(mask&(1u<<((k+7)&7)))||!(mask&(1u<<((k+1)&7))))mask&=~(1u<<k);
 const unsigned order[]={0,3,2,1};for(unsigned r:order){const unsigned m=r?((mask<<(2*r))|(mask>>(8-2*r)))&255:mask;for(unsigned i=0;i<15;++i)if(m==shapes[i]){variant=i;rotation=r;return;}}
 variant=rotation=0;
}
}
bool render_terrain_feature_layers(const TerrainFeatureInput&in,const PreparedTerrainAtlas&a,const TerrainTextureSurface&s){
 if(!in.source_context_supplied||in.altitude>6||!a.pixels||!a.width||!a.height||a.width>4096||a.height>4096||a.pitch<a.width||a.pitch>4096||!s.pixels||!s.width||!s.height||s.width>256||s.height>256||s.pitch<s.width||s.pitch>256)return false;
 const size_t au=(size_t)(a.height-1)*a.pitch+a.width,used=(size_t)(s.height-1)*s.pitch+s.width;if(au>a.bytes||used>s.bytes||overlap(s.pixels,used,&in,sizeof in)||overlap(s.pixels,used,&a,sizeof a)||overlap(s.pixels,used,&s,sizeof s)||overlap(s.pixels,used,a.pixels,au))return false;
 TerrainMesh mesh{};if(!build_terrain_mesh(in.mesh,mesh))return false;
 const bool jungle=in.jungle&&in.altitude>=3&&!in.concealed,fungus=in.fungus&&in.altitude>=2&&!in.concealed;
 const bool forest=in.forest&&in.altitude>=3&&!in.concealed;if(in.masks.forest>15)return false;
 if(!jungle&&!forest&&!fungus)return true;
 auto*memory=(uint8_t*)malloc(used+3136);if(!memory)return false;memcpy(memory,s.pixels,used);auto stage=s;stage.pixels=memory;stage.bytes=used;uint8_t*tex=memory+used;bool ok=true;
 for(unsigned layer=0;layer<3&&ok;++layer){if(layer==0?!jungle:layer==1?!forest:!fungus)continue;
  if(layer==1){const unsigned x=526+57*(in.masks.forest%4),y=6+57*(in.masks.forest/4);if(x+56>a.width||y+56>a.height){ok=false;break;}for(unsigned row=0;row<56;++row)memcpy(tex+row*56,a.pixels+(size_t)(y+row)*a.pitch+x,56);const int u[4]={0,55,55,0},v[4]={0,0,55,55};TerrainForestQuadInput draw{};draw.texels=tex;draw.texture_bytes=3136;draw.texture_width=draw.texture_height=56;for(unsigned k=0;k<4;++k){const auto&p=mesh.triangles[k][1];draw.vertex[k]={p.x,p.y,u[k],v[k],in.lights.point[k+1]};}ok=draw_terrain_forest_quad(draw,stage);continue;}
  unsigned variant=0,rotation=0;transition(layer==2?in.masks.fungus:in.masks.jungle,variant,rotation);
  const unsigned x=(layer==2?(in.altitude>=3?280:508):526)+57*(variant%4),y=(layer==2?516:259)+57*(variant/4);if(x+56>a.width||y+56>a.height){ok=false;break;}for(unsigned row=0;row<56;++row)memcpy(tex+row*56,a.pixels+(size_t)(y+row)*a.pitch+x,56);
  const int u[4]={0,0,55,55},v[4]={55,0,1,55};TerrainKeyedQuadInput draw{};draw.texels=tex;draw.texture_bytes=3136;draw.width=draw.height=56;
  for(unsigned k=0;k<4;++k){const auto&p=mesh.triangles[k][1];const unsigned uv=(k+rotation)&3;draw.vertex[k]={p.x,p.y,u[uv],v[uv]};}
  ok=draw_terrain_keyed_quad(draw,stage,9,layer==2&&!in.visible?65:-1);
 }
 if(ok)memcpy(s.pixels,memory,used);
 free(memory);return ok;
}
}
