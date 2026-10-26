/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_grid.h"
#include "terrain_line.h"
#include <stdlib.h>
#include <string.h>
namespace alpha {namespace {
bool overlap(const void*a,size_t n,const void*b,size_t m){if(!n||!m)return false;uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(x<y+m&&y<x+n);}
bool alias(const OriginalWorld&w,const void*p,size_t n){const auto&m=w.game().map.info;return overlap(p,n,&w,sizeof w)||overlap(p,n,w.original_bytes(),w.original_size())||overlap(p,n,w.tile(0),(size_t)m.tile_count*sizeof(OriginalMapTile))||overlap(p,n,w.vehicle(0),(size_t)w.game().vehicle_count*sizeof(OriginalVehicle))||overlap(p,n,w.base(0),(size_t)w.game().base_count*sizeof(OriginalBase))||overlap(p,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(p,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(p,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));}
}
bool world_terrain_grid(const OriginalWorld&w,int x,int y,const TerrainGridView&in,TerrainGridPlan&out){
 const auto&m=w.game().map.info;if(!w.loaded()||!in.supplied||x<0||y<0||x>=(int)m.width||y>=(int)m.height||((x+y)&1)||alias(w,&out,sizeof out)||overlap(&out,sizeof out,&in,sizeof in))return false;
 TerrainGridPlan result{};result.supplied=true;const auto*t=w.tile(((size_t)y*m.width+(unsigned)x)/2);const bool land=(t->climate>>5)>=3,sea=(in.more_preferences&0x800000u)!=0;
 if(in.enabled&&(land||sea)&&y>0)for(unsigned k=0;k<2;++k){int nx=x-1+(int)k*2;if(!m.flat){if(nx<0)nx+=(int)m.width;else if(nx>=(int)m.width)nx-=(int)m.width;}if(nx<0||nx>=(int)m.width)continue;const auto*n=w.tile(((size_t)(y-1)*m.width+(unsigned)nx)/2);const bool neighbour_land=(n->climate>>5)>=3;if(neighbour_land||sea)result.colour[k]=land&&neighbour_land?37:190;}
 out=result;return true;
}
bool render_terrain_grid(const TerrainGridInput&in,const TerrainTextureSurface&s){
 if(!in.plan.supplied||!s.pixels||!s.width||!s.height||s.width>256||s.height>256||s.pitch<s.width||s.pitch>256)return false;
 const size_t used=(size_t)(s.height-1)*s.pitch+s.width;if(used>s.bytes||overlap(s.pixels,used,&in,sizeof in)||overlap(s.pixels,used,&s,sizeof s))return false;
 for(auto c:in.plan.colour)if(c!=0&&c!=37&&c!=190)return false;
 TerrainMesh mesh{};if(!build_terrain_mesh(in.mesh,mesh))return false;
 auto*mem=(uint8_t*)malloc(used);if(!mem)return false;memcpy(mem,s.pixels,used);auto stage=s;stage.pixels=mem;stage.bytes=used;bool ok=true;
 for(unsigned k=0;k<2&&ok;++k)if(in.plan.colour[k]){const auto&a=mesh.triangles[k][1];const auto&b=mesh.triangles[(k+1)%4][1];TerrainLineInput line{a.x,a.y,b.x,b.y,in.plan.colour[k]};ok=draw_terrain_line(line,stage);}
 if(ok)memcpy(s.pixels,mem,used);
 free(mem);return ok;
}
}
