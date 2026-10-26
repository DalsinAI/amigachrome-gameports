/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_roads.h"
#include "terrain_forest_quad.h"
#include <stdlib.h>
#include <string.h>
namespace alpha {namespace {
bool overlap(const void*a,size_t n,const void*b,size_t m){if(!n||!m)return false;uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(x<y+m&&y<x+n);}
bool alias(const OriginalWorld&w,const void*p,size_t n){const auto&m=w.game().map.info;return overlap(p,n,&w,sizeof w)||overlap(p,n,w.original_bytes(),w.original_size())||overlap(p,n,w.tile(0),(size_t)m.tile_count*sizeof(OriginalMapTile))||overlap(p,n,w.vehicle(0),(size_t)w.game().vehicle_count*sizeof(OriginalVehicle))||overlap(p,n,w.base(0),(size_t)w.game().base_count*sizeof(OriginalBase))||overlap(p,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(p,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(p,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));}
const OriginalMapTile*tile(const OriginalWorld&w,int&x,int y){const auto&m=w.game().map.info;if(!m.flat){if(x<0)x+=(int)m.width;else if(x>=(int)m.width)x-=(int)m.width;}if(x<0||y<0||x>=(int)m.width||y>=(int)m.height||((x+y)&1))return nullptr;return w.tile(((size_t)y*m.width+(unsigned)x)/2);}
int visible_base(const OriginalWorld&w,int x,int y,const OriginalMapTile&t,int viewer,bool&repair){
 if(!(t.items&1u))return -1;
 for(unsigned k=0;k<w.game().base_count;++k){const auto*b=w.base(k);if(b->x==x&&b->y==y)return viewer<0||b->faction_id==viewer||(b->visibility&(1u<<viewer))?(int)k:-1;}
 repair=true;return -1;
}
}
TerrainRoadStatus world_terrain_roads(const OriginalWorld&w,int x,int y,const TerrainRoadView&v,TerrainRoadPlan&out){
 using S=TerrainRoadStatus;const auto&m=w.game().map.info;
 if(!w.loaded()||!v.supplied||v.viewer< -1||v.viewer>7||x<0||y<0||x>=(int)m.width||y>=(int)m.height||((x+y)&1)||alias(w,&out,sizeof out)||overlap(&out,sizeof out,&v,sizeof v))return S::invalid_input;
 int xx=x;const auto*t=tile(w,xx,y);if(!t)return S::invalid_input;TerrainRoadPlan result{};result.source_context_supplied=true;
 // Source465649..46569d: GameState reveal can show terrain, but does not
 // bypass this later raw-exploration/player-flag gate for roads/improvements.
 if(v.viewer>=0&&!(w.faction(v.viewer)->player_flags&0x200u)&&!(t->visibility&(1u<<v.viewer))){out=result;return S::resolved;}
 result.source_late_layers_visible=true;bool repair=false;const int centre=visible_base(w,x,y,*t,v.viewer,repair);if(repair)return S::held_base_repair;
 if((t->climate>>5)>=3&&((v.displayed_items&4u)||centre>=0)){
  const bool tube=(v.displayed_items&8u)||centre>=0;bool any=false,anytube=false;const int dx[8]={1,2,1,0,-1,-2,-1,0},dy[8]={-1,0,1,2,1,0,-1,-2};
  for(unsigned k=0;k<8;++k){int nx=x+dx[k];const int ny=y+dy[k];const auto*n=tile(w,nx,ny);if(!n||(n->climate>>5)<3)continue;const uint32_t flags=v.viewer<0?n->items:v.viewer==0?n->landmarks:n->visible_items[v.viewer-1];const int base=visible_base(w,nx,ny,*n,v.viewer,repair);if(repair)return S::held_base_repair;
   if((flags&4u)||base>=0){any=true;const bool nt=tube&&((flags&8u)||base>=0);result.direction[k]=nt?2:1;anytube|=nt;}
  }
  if(centre<0&&((tube&&!anytube)||!any))result.isolated=tube?2:1;
 }
 out=result;return S::resolved;
}
bool render_terrain_roads(const TerrainRoadLayerInput&in,const PreparedTerrainAtlas&a,const TerrainTextureSurface&s){
 if(!in.plan.source_context_supplied||in.plan.isolated>2||!a.pixels||!a.width||!a.height||a.width>4096||a.height>4096||a.pitch<a.width||a.pitch>4096||!s.pixels||!s.width||!s.height||s.width>256||s.height>256||s.pitch<s.width||s.pitch>256)return false;
 const size_t au=(size_t)(a.height-1)*a.pitch+a.width,used=(size_t)(s.height-1)*s.pitch+s.width;if(au>a.bytes||used>s.bytes||overlap(s.pixels,used,&in,sizeof in)||overlap(s.pixels,used,&a,sizeof a)||overlap(s.pixels,used,&s,sizeof s)||overlap(s.pixels,used,a.pixels,au))return false;
 bool any=in.plan.isolated!=0;for(auto v:in.plan.direction){if(v>2)return false;any|=v!=0;}
 if(any&&!in.plan.source_late_layers_visible)return false;
 for(auto v:in.lights.point)if(v< -4||v>4)return false;
 TerrainMesh mesh{};if(!build_terrain_mesh(in.mesh,mesh))return false;auto*mem=(uint8_t*)malloc(used+3136);if(!mem)return false;memcpy(mem,s.pixels,used);auto stage=s;stage.pixels=mem;stage.bytes=used;uint8_t*tex=mem+used;bool ok=true;
 for(unsigned k=0;k<9&&ok;++k){const unsigned kind=k<8?in.plan.direction[k]:in.plan.isolated;if(!kind)continue;const unsigned role=k<8?((k+2)&7)+1:0,x=775+57*(role%3),y=(kind==2?566:395)+57*(role/3);if(x+56>a.width||y+56>a.height){ok=false;break;}for(unsigned row=0;row<56;++row)memcpy(tex+row*56,a.pixels+(size_t)(y+row)*a.pitch+x,56);
  TerrainForestQuadInput q{};q.texels=tex;q.texture_bytes=3136;q.texture_width=q.texture_height=56;const int u[4]={0,55,55,0},v[4]={0,0,55,55};for(unsigned j=0;j<4;++j){const auto&p=mesh.triangles[j][1];q.vertex[j]={p.x,p.y,u[j],v[j],in.lights.point[j+1]};}ok=draw_terrain_forest_quad(q,stage);
 }
 if(ok)memcpy(s.pixels,mem,used);
 free(mem);return ok;
}
}
