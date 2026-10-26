/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_base_grid.h"
#include "terrain_line.h"
#include <stdlib.h>
#include <string.h>
namespace alpha {namespace {
bool overlap(const void*a,size_t n,const void*b,size_t m){if(!n||!m)return false;uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(x<y+m&&y<x+n);}
bool alias(const OriginalWorld&w,const void*p,size_t n){const auto&m=w.game().map.info;return overlap(p,n,&w,sizeof w)||overlap(p,n,w.original_bytes(),w.original_size())||overlap(p,n,w.tile(0),(size_t)m.tile_count*sizeof(OriginalMapTile))||overlap(p,n,w.vehicle(0),(size_t)w.game().vehicle_count*sizeof(OriginalVehicle))||overlap(p,n,w.base(0),(size_t)w.game().base_count*sizeof(OriginalBase))||overlap(p,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(p,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(p,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));}
unsigned abs_i(int x){return(unsigned)(x<0?-x:x);}
bool classified(const OriginalWorld&w,int x,int y,const OriginalMapTile&t,const TerrainBaseGridContext&in){
 unsigned user=t.val3&7;if(in.viewer>=0&&(int)user!=in.viewer&&!(w.faction(in.viewer)->diplo_status[user]&8u))user=0;
 if(user)return true;
 const auto&m=w.game().map.info;unsigned dx=abs_i(x-in.cursor_x),dy=abs_i(y-in.cursor_y);if(!m.flat&&dx>m.width/2)dx=m.width-dx;unsigned hi=dx>dy?dx:dy,lo=dx<dy?dx:dy;return hi-(((dx+dy)/2-lo+1)/2)<3;
}
}
bool world_terrain_base_grid(const OriginalWorld&w,int x,int y,const TerrainBaseGridContext&in,TerrainBaseGridPlan&out){
 const auto&m=w.game().map.info;if(!w.loaded()||!in.supplied||in.viewer< -1||in.viewer>7||x<0||y<0||x>=(int)m.width||y>=(int)m.height||((x+y)&1)||in.cursor_x<0||in.cursor_y<0||in.cursor_x>=(int)m.width||in.cursor_y>=(int)m.height||((in.cursor_x+in.cursor_y)&1)||alias(w,&out,sizeof out)||overlap(&out,sizeof out,&in,sizeof in))return false;
 const auto*t=w.tile(((size_t)y*m.width+(unsigned)x)/2);TerrainBaseGridPlan result{};result.supplied=true;
 if(in.viewer>=0&&!(w.faction(in.viewer)->player_flags&0x200u)&&!(t->visibility&(1u<<in.viewer))){out=result;return true;}
 result.late_visible=true;const bool centre=classified(w,x,y,*t,in);const int dx[4]={1,1,-1,-1},dy[4]={-1,1,1,-1};
 for(unsigned k=0;k<4;++k){int nx=x+dx[k],ny=y+dy[k];if(!m.flat){if(nx<0)nx+=(int)m.width;else if(nx>=(int)m.width)nx-=(int)m.width;}if(nx<0||ny<0||nx>=(int)m.width||ny>=(int)m.height)continue;const auto*n=w.tile(((size_t)ny*m.width+(unsigned)nx)/2);if(classified(w,nx,ny,*n,in)!=centre)result.edges|=(uint8_t)(1u<<k);}
 out=result;return true;
}
bool render_terrain_base_grid(const TerrainBaseGridInput&in,const TerrainTextureSurface&s){
 if(!in.plan.supplied||in.plan.edges>15||(!in.plan.late_visible&&in.plan.edges)||!s.pixels||!s.width||!s.height||s.width>256||s.height>256||s.pitch<s.width||s.pitch>256)return false;
 const size_t used=(size_t)(s.height-1)*s.pitch+s.width;if(used>s.bytes||overlap(s.pixels,used,&in,sizeof in)||overlap(s.pixels,used,&s,sizeof s))return false;
 TerrainMesh mesh{};if(!build_terrain_mesh(in.mesh,mesh))return false;auto*mem=(uint8_t*)malloc(used);if(!mem)return false;memcpy(mem,s.pixels,used);auto stage=s;stage.pixels=mem;stage.bytes=used;bool ok=true;
 for(unsigned k=0;k<4&&ok;++k)if(in.plan.edges&(1u<<k)){const auto&a=mesh.triangles[(k+1)&3][1];const auto&b=mesh.triangles[(k+2)&3][1];TerrainLineInput line{a.x,a.y,b.x,b.y,249};ok=draw_terrain_line(line,stage);}
 if(ok)memcpy(s.pixels,mem,used);
 free(mem);return ok;
}
}
