/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independently reconstructed original SMAC/SMAX numeric leaf contracts.
 * Later Thinker yield changes are deliberately not used here.
 */
#include "tile_yields.h"
#include <limits.h>
namespace alpha { namespace {
bool overlap(const void* a,size_t an,const void* b,size_t bn){
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return x<=y ? y-x<an : x-y<bn;
}
bool add(int32_t& n,int64_t delta){const int64_t v=(int64_t)n+delta;if(v<INT32_MIN || v>INT32_MAX)return false;n=(int32_t)v;return true;}
bool fac(const TileYieldContext& c,unsigned id){return (c.facilities&(UINT64_C(1)<<id))!=0;}
int32_t planet(int32_t n){return n < -3 ? -3 : n>0 ? 0 : n;}
int32_t clamp99(int32_t n){return n<0 ? 0 : n>99 ? 99 : n;}
int32_t floor4(int32_t n){return n>=0 ? n/4 : -1-((-1-n)/4);}
bool cap(int32_t& value,bool enabled,bool bonus,int32_t& reduced){
 if(value>2 && !enabled && !bonus){if(!add(reduced,(int64_t)value-2))return false;value=2;}return true;
}
bool events(int32_t& value,uint32_t flags,uint32_t plus,uint32_t minus){
 if((flags&plus) && !add(value,1))return false;
 return !(flags&minus) || !value || add(value,-1);
}
bool nutrient(const TileYieldContext& c,bool bs,int32_t& value,int32_t& reduced){
 const uint32_t it=c.tile.items,lm=c.tile.landmarks,alt=c.tile.climate&0xe0u;
 const bool extra=c.bonus==1,sea=alt<0x60,exp=c.variant==Variant::alien_crossfire;
 const bool landmark=((lm&0x80000004u)==4 && !sea) || ((lm&0x80u) && sea);
 const auto& r=c.resources;
 bool limited=false;
 if(bs){value=r[1][0];if(extra && !add(value,r[1][0]))return false;
  if(landmark && !add(value,1))return false;
  if((fac(c,3)||fac(c,26)) && !add(value,r[4][0]))return false;
 }else if(it&0x1000000u){value=r[8][0];if(value && !add(value,r[2][0]))return false;
 }else if(it&0x2000u){value=r[7][0];if(extra && !add(value,r[2][0]))return false;
  if(exp && c.manifold_owned && c.planet_pending>0 && !add(value,1))return false;
 }else if((it&0x20u) && !c.terraform_preview && alt>0x3f){value=c.fungus[0];if(!add(value,planet(c.planet_pending)))return false;value=clamp99(value);
  if(exp && c.manifold_owned && c.planet_pending>0 && !add(value,1))return false;
 }else{
  limited=true;
  if(!sea){
   if(it&0x200000u){value=r[3][0];if(extra && !add(value,r[2][0]))return false;
    if(fac(c,14) && !add(value,1))return false;
 if(fac(c,13) && !add(value,1))return false;
    if(landmark && !add(value,1))return false;
   }else{const unsigned rough=c.tile.val3>>6;value=rough>=2 ? 0 : (c.tile.climate&0x18u)>>3;
    if(extra && !add(value,r[2][0]))return false;
 if(landmark && !add(value,1))return false;
    if(value<0)value=0;
    if((it&0x8000u) && rough<2 && !add(value,r[5][0]))return false;
    if((it&0x10u) && value>1 && !add(value,c.mine_nutrient_effect))return false;
   }
  }else{value=r[0][0];if(extra && !add(value,r[2][0]))return false;
 if(landmark && !add(value,1))return false;
   if(alt==0x40 || (exp&&c.expansion_enabled)){
    if(it&0x8000u){if(!add(value,r[6][0]))return false;
 if(exp && fac(c,36) && !add(value,1))return false;}
    if((it&0x10u) && value>1 && !add(value,c.mine_nutrient_effect))return false;
   }
  }
  if((it&0x80000u) && !add(value,value/2))return false;
  if((it&0x400000u) && !add(value,value/2))return false;
 }
 if(limited && !cap(value,c.allow_three[0],extra || (it&0x400000u),reduced))return false;
 return events(value,c.base_event_flags,0x200u,0x400u);
}
bool mineral(const TileYieldContext& c,bool bs,int32_t& value,int32_t& reduced){
 const uint32_t it=c.tile.items,lm=c.tile.landmarks,alt=c.tile.climate&0xe0u;
 const bool extra=c.bonus==2,exp=c.variant==Variant::alien_crossfire;
 const int32_t index=(int32_t)(lm>>24) - ((lm&0x80000000u) ? 256 : 0);
 const bool landmark=(((lm&0x80000002u)==2 || (lm&0x80000001u)==1) && index<9) || (exp && (lm&0x80008000u)==0x8000 && index<6) || (lm&0x80000200u)==0x200;
 const auto& r=c.resources;value=extra ? r[2][1] : 0;if(landmark && !add(value,1))return false;
 bool limited=false;
 if(bs){if(!add(value,r[1][1]))return false;
 if((fac(c,3)||fac(c,26)) && !add(value,r[4][1]))return false;
 }else if(it&0x2000u){value=r[7][1];if(extra && !add(value,r[2][1]))return false;
  if(exp && c.manifold_owned && c.planet_pending>1 && !add(value,1))return false;
 }else if(it&0x1000000u){value=r[8][1];if(extra && !add(value,r[2][1]))return false;limited=true;
 }else if((it&0x20u) && alt>0x3f){value=c.fungus[1];if(!add(value,planet(c.planet_pending)))return false;value=clamp99(value);
  if(exp && c.manifold_owned && c.planet_pending>1 && !add(value,1))return false;
 }else{limited=true;
  if(alt>=0x60){const unsigned rough=c.tile.val3>>6;
   if(it&0x200000u){if(!add(value,r[3][1]))return false;
   }else if(!(it&0x1000010u) && !c.terraform_preview){if(rough && !add(value,1))return false;
   }else{if(!add(value,rough))return false;int32_t increment=rough ? (int32_t)rough : 1;
    if((extra||landmark) && !add(increment,1))return false;
    if(!(it&4u) && !c.terraform_preview && increment>c.mine_without_road_limit){if(!add(reduced,(int64_t)increment-c.mine_without_road_limit))return false;increment=c.mine_without_road_limit;}
    if(!add(value,increment))return false;
   }
  }else{if(!add(value,r[0][1]))return false;
   if(exp && c.aquatic && alt==0x40 && !add(value,1))return false;
   if((alt==0x40 || (exp&&c.expansion_enabled)) && ((it&0x10u)||c.terraform_preview)){
    if(!add(value,r[6][1]))return false;
 if(c.sea_mineral_technology && !add(value,1))return false;
    if(exp && fac(c,37) && !add(value,1))return false;
   }
  }
 }
 if(limited && !cap(value,c.allow_three[1],extra,reduced))return false;
 return events(value,c.base_event_flags,0x800u,0x1000u);
}
bool energy(const TileYieldContext& c,bool bs,int32_t& value,int32_t& reduced,int32_t& base_energy,bool& written){
 const uint32_t it=c.tile.items,lm=c.tile.landmarks,alt=c.tile.climate&0xe0u;
 const bool exp=c.variant==Variant::alien_crossfire,land=alt>=0x60;
 const int32_t index=(int32_t)(lm>>24) - ((lm&0x80000000u) ? 256 : 0);
 const bool landmark=((lm&0x80000002u)==2 && index<9) || (lm&0x80000008u)==8 || (lm&0x80000400u)==0x400 || (lm&0x80000800u)==0x800;
 const auto& r=c.resources;bool fungus=false;
 int32_t economy=c.economy_pending;if(c.golden_age && !add(economy,1))return false;
 if(bs){value=r[1][2];if(fac(c,1) && !add(value,1))return false;
  if((fac(c,3)||fac(c,26)) && !add(value,r[4][2]))return false;
  int32_t delta=economy;
  if(delta<0){if((!fac(c,1)||delta<-1||c.base_count==1) && !add(delta,1))return false;
  }else if(delta>1){if(delta>4)delta=4;delta=delta*2-4;}
  if(!add(value,delta))return false;
 if(c.governor && !add(value,1))return false;
 }else if(it&0x2000u){value=r[7][2];if(exp && c.manifold_owned){if(c.planet_pending>=0 && !add(value,1))return false;
 if(c.planet_pending>2 && !add(value,1))return false;}
 }else if(it&0x1000000u){value=r[8][2];
 }else if((it&0x20u) && alt>0x3f){fungus=true;value=c.fungus[2];if(!add(value,planet(c.planet_pending)))return false;value=clamp99(value);
  if(exp && c.manifold_owned){if(c.planet_pending>=0 && !add(value,1))return false;
 if(c.planet_pending>2 && !add(value,1))return false;}
 }else if(land){
  if(it&0x200000u){value=r[3][2];if(fac(c,14) && !add(value,1))return false;
  }else{value=((it&0x1800040u)||c.terraform_preview) ? (int32_t)(c.tile.climate>>5)-2 : 0;if(value<0)value=0;
   if((it&0x40u) && !add(value,c.echelon_matches))return false;
  }
 }else{value=0;if(alt==0x40 || (exp&&c.expansion_enabled)){value=r[0][2];
   if((it&0x40u)||c.terraform_preview){if(!add(value,r[6][2]))return false;
 if(exp && fac(c,38) && !add(value,1))return false;}
  }
  if((it&0x200000u) && !add(value,-1))return false;
 }
 if(!fungus){if((it&0x80u) && land && !add(value,1))return false;
  if(c.bonus==3 && !add(value,r[2][2]))return false;
 if(landmark && !add(value,1))return false;
  if(c.merchant_exchange && !add(value,1))return false;
 if(economy>1 && !add(value,1))return false;
  if(bs){value=clamp99(value);base_energy=value;written=true;
  }else if(!cap(value,c.allow_three[2],c.bonus==3,reduced))return false;
 }
 if(!events(value,c.base_event_flags,0x2000u,0x4000u))return false;
 if(c.dust_cloud && value && !add(value,-1))return false;
 if(c.solar_flares&(c.control_upkeep ? 2u : 1u)){const int32_t old=value;if(!add(value,(int64_t)old*2))return false;}
 return true;
}
} // namespace
TileYieldError calculate_tile_yields(const TileYieldContext& c,TileYieldResult& output){
 if(overlap(&c,sizeof c,&output,sizeof output))return TileYieldError::bad_buffer;
 if((c.variant!=Variant::base && c.variant!=Variant::alien_crossfire) || (c.variant==Variant::base&&c.expansion_enabled) || c.bonus>3 || c.base_count<0 || c.echelon_matches>8 || c.mine_without_road_limit<0)return TileYieldError::bad_state;
 const bool bs=(c.tile.items&1u) && (c.tile.val2&15u)<8;
 TileYieldResult next{};next.terraform_reduction=c.terraform_reduction_before;next.base_energy=c.base_energy_before;
 if(!nutrient(c,bs,next.nutrient,next.reduction_by_resource[0]) || !mineral(c,bs,next.mineral,next.reduction_by_resource[1]) || !energy(c,bs,next.energy,next.reduction_by_resource[2],next.base_energy,next.base_energy_written))return TileYieldError::overflow;
 for(unsigned i=0;i<3;++i)if(!add(next.terraform_reduction,next.reduction_by_resource[i]))return TileYieldError::overflow;
 output=next;return TileYieldError::ok;
}
TileYieldError tile_resource_bonus(const OriginalMapInfo& map,const OriginalMapTile& tile,int32_t x,int32_t y,int32_t rules,uint32_t& output){
 if(overlap(&map,sizeof map,&output,sizeof output)||overlap(&tile,sizeof tile,&output,sizeof output))return TileYieldError::bad_buffer;
 if(map.width<2 || map.width>4096 || (map.width&1u) || !map.height || map.height>4096 || x<0 || y<0 || (uint32_t)x>=map.width || (uint32_t)y>=map.height || ((x+y)&1))return TileYieldError::bad_state;
 uint32_t next=0;const uint32_t it=tile.items,alt=tile.climate>>5;
 if((it&0x400u) || (map.random_seed && (alt<=2 || ((uint32_t)rules&0x2000u)))){
  const int32_t average=(x+y)/2,difference=x-average;
  uint32_t code=((uint32_t)average&3u)+(((uint32_t)difference&3u)*4u);
  const uint32_t generated=(map.random_seed-5u*(uint32_t)((x+y)/8)-3u*(uint32_t)floor4(difference))&15u;
  if(((it&0x400u)||code==generated) && alt>=2){code=alt<3 ? code%3+1 : (code%5)&3u;
   if(!code || (it&0x20000000u))code=(it&0x10000u) ? 3u : (it&0x20000u) ? 2u : 1u;
   next=code;
  }
 }
 output=next;return TileYieldError::ok;
}
const char* tile_yield_error_name(TileYieldError e){switch(e){case TileYieldError::ok:return "ok";case TileYieldError::bad_state:return "bad_state";case TileYieldError::overflow:return "overflow";case TileYieldError::bad_buffer:return "bad_buffer";}return "unknown";}
}
