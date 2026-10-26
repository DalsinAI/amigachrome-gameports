/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_rivers.h"
#include "terrain_forest_quad.h"
#include <stdlib.h>
#include <string.h>
namespace alpha {namespace {
bool overlap(const void*a,size_t n,const void*b,size_t m){if(!n||!m)return false;uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(x<y+m&&y<x+n);}
}
bool render_terrain_rivers(const TerrainRiverInput&in,const PreparedTerrainAtlas&a,const TerrainTextureSurface&s){
 if(!in.source_context_supplied||in.neighbour_mask>15||in.fog_colour< -1||in.fog_colour>255||!a.pixels||!a.width||!a.height||a.width>4096||a.height>4096||a.pitch<a.width||a.pitch>4096||!s.pixels||!s.width||!s.height||s.width>256||s.height>256||s.pitch<s.width||s.pitch>256)return false;
 const size_t au=(size_t)(a.height-1)*a.pitch+a.width,used=(size_t)(s.height-1)*s.pitch+s.width;
 if(au>a.bytes||used>s.bytes||overlap(s.pixels,used,&in,sizeof in)||overlap(s.pixels,used,&a,sizeof a)||overlap(s.pixels,used,&s,sizeof s)||overlap(s.pixels,used,a.pixels,au))return false;
 for(auto v:in.lights.point)if(v< -4||v>4)return false;
 TerrainMesh mesh{};if(!build_terrain_mesh(in.mesh,mesh))return false;
 if(!in.river)return true;
 auto*mem=(uint8_t*)malloc(used+3136);if(!mem)return false;memcpy(mem,s.pixels,used);auto stage=s;stage.pixels=mem;stage.bytes=used;uint8_t*tex=mem+used;bool ok=true;
 for(unsigned k=0;k<(in.source?2u:1u)&&ok;++k){const unsigned role=k?0:in.neighbour_mask,x=280+57*(role%4),y=259+57*(role/4);if(x+56>a.width||y+56>a.height){ok=false;break;}for(unsigned row=0;row<56;++row)memcpy(tex+row*56,a.pixels+(size_t)(y+row)*a.pitch+x,56);
  TerrainForestQuadInput q{};q.texels=tex;q.texture_bytes=3136;q.texture_width=q.texture_height=56;const int u[4]={0,55,55,0},v[4]={0,0,55,55};for(unsigned j=0;j<4;++j){const auto&pt=mesh.triangles[j][1];q.vertex[j]={pt.x,pt.y,u[j],v[j],in.lights.point[j+1]};}ok=draw_terrain_shaded_keyed_quad(q,stage,9,in.fog_colour);
 }
 if(ok)memcpy(s.pixels,mem,used);
 free(mem);return ok;
}
}
