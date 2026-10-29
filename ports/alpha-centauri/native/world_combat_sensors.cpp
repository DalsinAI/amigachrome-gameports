/* SPDX-License-Identifier: GPL-3.0-or-later; attribution world_combat_sensors.h. */
#include "world_combat_sensors.h"
#include "movement.h"
namespace alpha {namespace {
const int32_t dx[25]={0,1,2,1,0,-1,-2,-1,0,2,2,-2,-2,1,3,3,1,-1,-3,-3,-1,4,-4,0,0};
const int32_t dy[25]={0,-1,0,1,2,1,0,-1,-2,-2,2,2,-2,-3,-1,1,3,3,1,-1,-3,0,0,4,-4};
bool overlap(const void*a,size_t an,const void*b,size_t bn){if(!a||!b||!an||!bn)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
bool alias(const OriginalWorld&w,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();return overlap(out,n,w.original_bytes(),w.original_size())||overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
int32_t absval(int32_t n){return n<0?-n:n;}
int32_t xdistance(const MapGeometry&g,int32_t x,int32_t bx){int32_t d=absval(x-bx);return !g.flat&&d>g.width/2?g.width-d:d;}
int32_t distance(const MapGeometry&g,int32_t x,int32_t y,int32_t bx,int32_t by){const int32_t a=xdistance(g,x,bx),b=absval(y-by),lo=a<b?a:b,hi=a>b?a:b;return hi-(((a+b)/2-lo+1)/2);}
int32_t nearest(const OriginalWorld&w,const MapGeometry&g,int32_t x,int32_t y){
 int32_t best=-1,dist=9999;for(size_t i=0;i<w.game().base_count;++i){const auto*b=w.base(i);const int32_t d=distance(g,x,y,b->x,b->y);if(d<=dist){dist=d;best=(int32_t)i;}}
 return best;
}
}
CombatError world_combat_sensor_mask(const OriginalWorld&w,int32_t defender,int32_t x,int32_t y,uint32_t&out){
 if(alias(w,&out,sizeof out))return CombatError::bad_buffer;
 if(!w.loaded()||defender<1||defender>7)return CombatError::bad_state;
 const auto&m=w.game().map.info;const MapGeometry g{(int32_t)m.width,(int32_t)m.height,m.flat};size_t center;
 if(movement_tile_index(g,x,y,center)!=LogicError::ok)return CombatError::bad_state;
 uint32_t mask=0;
 for(unsigned i=0;i<25;++i){
  int32_t tx;if(movement_wrap_x(g,x+dx[i],tx)!=LogicError::ok)continue;
  const int32_t ty=y+dy[i];size_t tile;if(movement_tile_index(g,tx,ty,tile)!=LogicError::ok)continue;
  const auto*square=w.tile(tile);if(!square||square->owner>7)return CombatError::bad_state;
  const int32_t base=nearest(w,g,tx,ty);const auto*b=base>=0?w.base((size_t)base):nullptr;
  uint32_t status=(square->items&0x80000000u)?1u:0u;
  if(!status&&b&&(b->facilities_built[5]&2u)){
   const int32_t a=xdistance(g,tx,b->x),c=absval(ty-b->y);
   if((a==0||a==2)&&(c==0||c==2))status=2;
  }
  if(!status)continue;
  const bool ocean=(square->climate&0xe0u)<0x60u;
  const bool found=ocean?(b&&b->faction_id==defender):(square->owner<0||square->owner==defender);
  if(found){mask|=1;if(status>1){mask|=2;break;}}
 }
 out=mask;return CombatError::ok;
}
}
