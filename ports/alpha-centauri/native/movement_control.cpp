/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include <initializer_list>
#define ALPHA_CONTROL_NO_MAIN
#include "prototype_control.cpp"
#include "upstream_movement_reference.h"
namespace movement_control {
static void word(uint32_t& h,uint32_t value){for(unsigned s=0;s<32;s+=8)h=(h^((value>>s)&255u))*16777619u;}
static uint32_t items(uint32_t mask){const uint32_t values[]={4,8,0x20,0x80,0x200000,1};uint32_t bits=0;for(unsigned i=0;i<6;++i)if(mask&(1u<<i))bits|=values[i];return bits;}
int cost_case(const alpha::UnitDefinitions& d,size_t id,uint32_t pattern,uint32_t context,int32_t road,uint32_t& digest) {
  static alpha::MovementTile tiles[32];
  for(auto& t:tiles)t={0,0x60,0};
  const bool seam=context&1u;
  alpha::MovementMap map{{8,8,!seam},tiles,32};
  const int32_t sx=seam?0:2,sy=2,dx=seam?7:3,dy=3;
  const size_t si=(sx+8*sy)/2,di=(dx+8*dy)/2;
  const uint8_t altitude[]={0x60,0x40,0x20},rock[]={0,0x40,0x80};
  tiles[si]={items(pattern),(uint8_t)(pattern%7?0x60:0x20),0};
  tiles[di]={items((pattern*13+7)&63u),altitude[pattern%3],rock[(pattern/3)%3]};
  alpha::MovementContext ctx{context&8u?0:1,context&4u?1:0,road,(context&1u)!=0,(context&2u)!=0};
  prototype_control::setup(d,road);const auto& b=d.units[id];
  using namespace prototype_reference;
  make_proto((int)id,(uint32_t)b.chassis,(uint32_t)b.weapon,(uint32_t)b.armor,b.abilities,(uint32_t)b.reactor);
  if(b.plan!=-1)VehPrototypes[id].plan=(uint8_t)b.plan;
  movement_reference::map=map;movement_reference::xeno=ctx.xenoempathy_dome;
  movement_reference::PlayersData[ctx.faction].soc_effect_active.planet=ctx.planet;
  const int expected=movement_reference::hex_cost((int)id,ctx.faction,(uint32_t)sx,(uint32_t)sy,(uint32_t)dx,(uint32_t)dy,ctx.ignore_rough);
  uint32_t actual=12345;
  auto error=alpha::adjacent_movement_cost(map,d,id,ctx,sx,sy,dx,dy,actual);
  if(error!=alpha::LogicError::ok || actual!=(uint32_t)expected || unsupported)return 1;
  word(digest,actual);return 0;
}
int named_case(const alpha::UnitDefinitions& d,size_t id,uint32_t from_items,uint32_t to_items,
               uint8_t rock,int32_t faction,int32_t planet,bool xeno,bool ignore,
               int32_t sx,int32_t sy,int32_t dx,int32_t dy,bool flat,uint32_t expected,uint32_t& digest) {
  static alpha::MovementTile tiles[32];for(auto& tile:tiles)tile={0,0x60,0};
  alpha::MovementMap map{{8,8,flat},tiles,32};
  tiles[(sx+8*sy)/2].items=from_items;tiles[(dx+8*dy)/2]={to_items,0x60,rock};
  alpha::MovementContext ctx{faction,planet,3,xeno,ignore};uint32_t value=12345;
  prototype_control::setup(d,3);const auto& b=d.units[id];
  prototype_reference::make_proto((int)id,(uint32_t)b.chassis,(uint32_t)b.weapon,(uint32_t)b.armor,b.abilities,(uint32_t)b.reactor);
  if(b.plan!=-1)prototype_reference::VehPrototypes[id].plan=(uint8_t)b.plan;
  movement_reference::map=map;movement_reference::xeno=xeno;movement_reference::PlayersData[faction].soc_effect_active.planet=planet;
  if((uint32_t)movement_reference::hex_cost((int)id,faction,(uint32_t)sx,(uint32_t)sy,(uint32_t)dx,(uint32_t)dy,ignore)!=expected)return 20;
  if(alpha::adjacent_movement_cost(map,d,id,ctx,sx,sy,dx,dy,value)!=alpha::LogicError::ok || value!=expected)return 21;
  word(digest,value);return 0;
}
int geometry_cases(uint32_t& digest) {
  for(int32_t width:{2,4,8,80,32766})for(int32_t height:{1,8,40,32767})for(unsigned flat=0;flat<2;++flat){
    alpha::MapGeometry g{width,height,flat!=0};
    const int32_t xs[]={0,1,width-2,width-1};
    const int32_t ys[]={0,height-1};
    for(int32_t x:xs)for(int32_t y:ys){size_t index=12345;auto rc=alpha::movement_tile_index(g,x,y,index);
      if((x+y)&1){if(rc!=alpha::LogicError::bad_state || index!=12345)return 2;word(digest,0xffffffffu);}
      else {if(rc!=alpha::LogicError::ok || index!=(size_t)(x+width*y)/2)return 3;word(digest,(uint32_t)index);}
    }
    const int32_t wrap[]={-width,-1,0,width-1,width,width*2};
    for(int32_t x:wrap){int32_t out=12345,expected=x;if(!flat){if(x<0)expected+=width;else if(x>=width)expected-=width;}
      if(alpha::movement_wrap_x(g,x,out)!=alpha::LogicError::ok || out!=expected)return 4;
      word(digest,(uint32_t)out);}
    for(int32_t a:xs)for(int32_t b:xs){int32_t out=12345,expected=a>b?a-b:b-a;if(!flat && expected>width/2)expected=width-expected;
      if(alpha::movement_distance_x(g,a,b,out)!=alpha::LogicError::ok || out!=expected)return 5;
      word(digest,(uint32_t)out);}
  }
  return 0;
}
int synthetic(uint32_t& digest) {
  static alpha::UnitDefinitions d;prototype_control::fixture(d,true);
  for(auto& u:d.units){u.cost=1;u.carry=1;}
  d.weapons[8].offense=-1;
  for(size_t id:{8u,15u,19u}){d.units[id].chassis=0;d.units[id].weapon=8;d.units[id].abilities=0;}
  for(size_t id:{9u,14u}){d.units[id].chassis=3;d.units[id].weapon=8;d.units[id].abilities=0;}
  d.units[1].chassis=0;d.units[1].weapon=4;d.units[1].plan=9;d.units[1].abilities=0;
  d.units[7].chassis=0;d.units[7].weapon=7;d.units[7].plan=12;d.units[7].abilities=0;
  const size_t ids[]={0,1,2,3,7,8,9,10,11,14,15,19};
  for(size_t id:ids)for(uint32_t pattern=0;pattern<64;++pattern)for(uint32_t ctx=0;ctx<16;++ctx)for(int32_t road:{3,100}){
    int rc=cost_case(d,id,pattern,ctx,road,digest);if(rc)return rc;
  }
  // Hand-picked branches avoid correlations in the generated endpoint matrix.
  int rc=0;
  for(unsigned which=0;which<20;++which) {
    const uint32_t river=0x80u,mag=8u,road=4u,fungus=0x20u,forest=0x200000u;
    size_t id=0;uint32_t from=0,to=0,expected=3;uint8_t rough=0;
    int32_t faction=1,planet=0,sx=2,sy=2,dx=3,dy=3;bool xeno=false,ignore=false,flat=true;
    switch(which) {
      case 0:from=to=river;expected=1;break;
      case 1:from=to=river;sx=0;dx=7;flat=false;expected=1;break;
      case 2:from=to=river;dx=2;dy=4;expected=3;break;
      case 3:from=to=river;dx=4;dy=2;expected=3;break;
      case 4:from=to=mag;expected=0;break;
      case 5:from=to=mag;faction=0;expected=3;break;
      case 6:from=to=road;expected=1;break;
      case 7:from=to=road;faction=0;expected=3;break;
      case 8:to=forest;rough=0x80;expected=9;break;
      case 9:to=forest;rough=0x80;ignore=true;expected=3;break;
      case 10:id=11;to=forest;rough=0x80;expected=3;break;
      case 11:id=10;to=forest;rough=0x80;expected=3;break;
      case 12:to=fungus;expected=9;break;
      case 13:to=fungus;xeno=true;expected=1;break;
      case 14:to=fungus;faction=0;expected=1;break;
      case 15:id=8;to=fungus;expected=1;break;
      case 16:id=15;to=fungus;expected=1;break;
      case 17:id=1;to=fungus;expected=3;break;
      case 18:from=to=mag;faction=7;expected=0;break;
      case 19:to=fungus;planet=-1;expected=9;break;
    }
    rc=named_case(d,id,from,to,rough,faction,planet,xeno,ignore,sx,sy,dx,dy,flat,expected,digest);if(rc)return rc;
  }
  rc=geometry_cases(digest);if(rc)return rc;
  static alpha::MovementTile tiles[32];for(auto& tile:tiles)tile={0,0x60,0};
  alpha::MovementMap map{{8,8,true},tiles,32};alpha::MovementContext ctx{1,0,3,false,false};uint32_t value=12345;
  auto rejected=[&](const alpha::MovementMap& m,const alpha::MovementContext& c,int32_t sx,int32_t sy,int32_t dx,int32_t dy){return alpha::adjacent_movement_cost(m,d,0,c,sx,sy,dx,dy,value)==alpha::LogicError::bad_state && value==12345;};
  if(!rejected(map,ctx,-1,2,1,2))return 6;
  if(!rejected(map,ctx,2,2,3,2))return 7;
  if(!rejected(map,ctx,2,2,2,2))return 8;
  if(!rejected(map,ctx,2,2,6,2))return 9;
  if(!rejected(map,ctx,2,2,2,8))return 10;
  map.count=31;if(!rejected(map,ctx,2,2,3,3))return 11;map.count=32;
  map.tiles=nullptr;if(!rejected(map,ctx,2,2,3,3))return 12;map.tiles=tiles;
  ctx.faction=8;if(!rejected(map,ctx,2,2,3,3))return 13;ctx.faction=-1;if(!rejected(map,ctx,2,2,3,3))return 14;ctx.faction=1;
  ctx.road_rate=0;if(!rejected(map,ctx,2,2,3,3))return 15;ctx.road_rate=3;
  tiles[13].rockiness=0xc0;if(!rejected(map,ctx,2,2,3,3))return 16;tiles[13].rockiness=0;
  tiles[13].altitude=0x61;if(!rejected(map,ctx,2,2,3,3))return 17;tiles[13].altitude=0x60;
  size_t index=12345;alpha::MapGeometry invalid{7,8,false};
  if(alpha::movement_tile_index(invalid,2,2,index)!=alpha::LogicError::bad_state || index!=12345)return 18;
  invalid={8,0,false};int32_t wrapped=12345;
  if(alpha::movement_wrap_x(invalid,0,wrapped)!=alpha::LogicError::bad_state || wrapped!=12345)return 19;
  return 0;
}
}
#ifndef ALPHA_CONTROL_NO_MAIN_MOVEMENT
int main(){uint32_t digest=2166136261u;int rc=movement_control::synthetic(digest);
#ifndef ALPHA_CONTROL_RAW
 printf("movement=24576 named=20 geometry=1200 invalid=14 digest=%08lx rc=%d\n",(unsigned long)digest,rc);
#endif
 return rc;}
#endif
