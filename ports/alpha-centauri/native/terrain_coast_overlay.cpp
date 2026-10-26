/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_coast_overlay.h"
#include "terrain_coast.h"
#include <stdlib.h>
#include <string.h>
namespace alpha {namespace {
bool overlap(const void*a,size_t n,const void*b,size_t m){if(!n||!m)return false;uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(x<y+m&&y<x+n);}
}
bool render_terrain_coast_overlay(const TerrainCoastOverlayInput&in,const TerrainCoastMetadata&m,const TerrainCoastResources&r,const PreparedTerrainAtlas&a,const TerrainTextureSurface&s){
 if(!in.source_coast_context_supplied||(in.fog_colour!= -1&&in.fog_colour!=65)||!a.pixels||a.width<336||a.height<135||a.width>4096||a.height>4096||a.pitch<a.width||a.pitch>4096)return false;
 const size_t atlas_used=(size_t)(a.height-1)*a.pitch+a.width;
 if(atlas_used>a.bytes||!s.pixels||!s.width||!s.height||s.width>256||s.height>256||s.pitch<s.width||s.pitch>256)return false;
 const size_t used=(size_t)(s.height-1)*s.pitch+s.width;
 if(used>s.bytes||overlap(s.pixels,used,&in,sizeof in)||overlap(s.pixels,used,&m,sizeof m)||overlap(s.pixels,used,&r,sizeof r)||overlap(s.pixels,used,&a,sizeof a)||overlap(s.pixels,used,&s,sizeof s)||overlap(s.pixels,used,a.pixels,atlas_used)||overlap(s.pixels,used,m.classes,53)||overlap(s.pixels,used,m.rules,424))return false;
 TerrainMesh mesh{};if(!build_terrain_mesh(in.mesh,mesh))return false;
 TerrainCoastSector sector[4]{};for(unsigned k=0;k<4;++k){if(!prepare_terrain_coast_sector(mesh,k,in.shore_mask[k],in.map_x,in.map_y,m,r,sector[k]))return false;for(unsigned j=0;j<4;++j)if(in.light.point[k][j]< -127||in.light.point[k][j]>127)return false;}
 uint8_t*memory=(uint8_t*)malloc(used+3136);if(!memory)return false;memcpy(memory,s.pixels,used);uint8_t*primary=memory+used;for(unsigned y=0;y<56;++y)memcpy(primary+y*56,a.pixels+(size_t)(79+y)*a.pitch+280,56);
 TerrainTextureSurface stage=s;stage.pixels=memory;stage.bytes=used;bool ok=true;
 for(unsigned k=0;k<4&&ok;++k)if(in.shore_mask[k]){TerrainCoastInput draw{};draw.texels=primary;draw.texture_bytes=3136;draw.mask=r.mask;draw.mask_bytes=3136;draw.lookup=sector[k].lookup;draw.lookup_bytes=53;draw.width=draw.height=56;draw.fog_colour=in.fog_colour;for(unsigned v=0;v<4;++v)draw.vertex[v]={sector[k].xy[v][0],sector[k].xy[v][1],sector[k].uv[v].u,sector[k].uv[v].v,in.light.point[k][v]};ok=draw_terrain_coast(draw,stage);}
 if(ok)memcpy(s.pixels,memory,used);
 free(memory);return ok;
}
}
