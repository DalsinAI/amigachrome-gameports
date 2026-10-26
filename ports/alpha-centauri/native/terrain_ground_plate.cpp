/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_ground_plate.h"
#include <stdlib.h>
#include <string.h>
namespace alpha {namespace {
bool overlap(const void*a,size_t n,const void*b,size_t m){if(!n||!m)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(x<y+m&&y<x+n);}
void crop(const PreparedTerrainAtlas&a,unsigned x,unsigned y,uint8_t*out){for(unsigned row=0;row<56;++row)memcpy(out+56*row,a.pixels+(size_t)(y+row)*a.pitch+x,56);}
}
bool render_terrain_ground_plate(const TerrainGroundPlateInput&input,const PreparedTerrainAtlas&atlas,const TerrainTextureSurface&surface){
 if(!atlas.pixels||!atlas.width||!atlas.height||atlas.width>4096||atlas.height>4096||atlas.pitch<atlas.width||atlas.pitch>4096)return false;
 const size_t atlas_used=(size_t)(atlas.height-1)*atlas.pitch+atlas.width;
 if(atlas_used>atlas.bytes||!surface.pixels||!surface.width||!surface.height||surface.width>256||surface.height>256||surface.pitch<surface.width||surface.pitch>256)return false;
 const size_t used=(size_t)(surface.height-1)*surface.pitch+surface.width;
 if(used>surface.bytes||overlap(surface.pixels,used,&input,sizeof input)||overlap(surface.pixels,used,&atlas,sizeof atlas)||overlap(surface.pixels,used,&surface,sizeof surface)||overlap(surface.pixels,used,atlas.pixels,atlas_used))return false;
 TerrainMesh mesh{};TerrainDispatch plan{},overlay{};
 if(!build_terrain_mesh(input.mesh,mesh)||!terrain_dispatch(input.material,plan))return false;
 auto context=input.material;context.rockiness=0;context.dunes=false;context.farm=false;
 if(!terrain_dispatch(context,overlay))return false;
 if(plan.atlas_x+56u>atlas.width||plan.atlas_y+56u>atlas.height||(plan.overlay&&(plan.overlay_x+56u>atlas.width||plan.overlay_y+56u>atlas.height)))return false;
 uint8_t*memory=(uint8_t*)malloc(used+2*3136);if(!memory)return false;
 memcpy(memory,surface.pixels,used);uint8_t*primary=memory+used,*secondary=primary+3136;crop(atlas,plan.atlas_x,plan.atlas_y,primary);if(plan.overlay)crop(atlas,plan.overlay_x,plan.overlay_y,secondary);
 TerrainTextureSurface stage=surface;stage.pixels=memory;stage.bytes=used;bool ok=true;
 for(unsigned k=0;ok&&k<4;++k){
  TerrainTextureInput triangle{};triangle.texels=primary;triangle.texture_bytes=3136;triangle.texture_width=triangle.texture_height=56;
  for(unsigned v=0;v<3;++v){const auto&p=mesh.triangles[k][v];triangle.vertex[v]={p.x,p.y,plan.uv[k][v].u,plan.uv[k][v].v,input.light.point[p.corner]};}
  ok=draw_terrain_fog_texture(triangle,stage,plan.fog_colour);
  if(ok&&plan.overlay){triangle.texels=secondary;for(unsigned v=0;v<3;++v){triangle.vertex[v].u=overlay.uv[k][v].u;triangle.vertex[v].v=overlay.uv[k][v].v;}ok=draw_terrain_masked_texture(triangle,stage,9,plan.fog_colour);}
 }
 if(ok)memcpy(surface.pixels,memory,used);
 free(memory);return ok;
}
}
