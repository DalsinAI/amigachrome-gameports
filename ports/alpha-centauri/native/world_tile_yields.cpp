/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_tile_yields.h"
#include "world_unit_stats.h"
#include "movement.h"
namespace alpha { namespace {
bool overlap(const void* a,size_t an,const void* b,size_t bn){
 if(!a || !b || !an || !bn)return false;
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<=y ? y-x<an : x-y<bn;
}
bool shape(const OriginalWorld& w){
 if(!w.loaded())return false;
 const auto& g=w.game();const auto& m=g.map.info;
 if(m.width<2 || m.width>4096 || (m.width&1u) || !m.height || m.height>4096 || m.tile_count!=(m.width/2)*m.height)return false;
 return (uint64_t)m.tile_count*sizeof(OriginalMapTile)<=w.memory_bytes() && (uint64_t)g.base_count*sizeof(OriginalBase)<=w.memory_bytes() && (uint64_t)g.vehicle_count*sizeof(OriginalVehicle)<=w.memory_bytes();
}
bool aliases(const OriginalWorld& w,const Rules& t,const BasicRules& b,const ResourceInfo& r,const WorldTileYieldStage& s,const void* out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&t,sizeof t)||overlap(out,n,&b,sizeof b)||overlap(out,n,&r,sizeof r)||overlap(out,n,&s,sizeof s))return true;
 if(!w.loaded())return false;
 const auto& g=w.game();
 return overlap(out,n,w.original_bytes(),w.original_size()) ||
  overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile)) ||
  overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle)) ||
  overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase)) ||
  overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype)) ||
  overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState)) ||
  overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
bool valid_technology(const Rules& t){
 if(t.count!=89)return false;
 for(size_t i=0;i<89;++i)for(unsigned j=0;j<2;++j)if(t.technologies[i].prerequisites[j]<-2 || t.technologies[i].prerequisites[j]>=89)return false;
 return true;
}
bool technology(const OriginalWorld& w,const Rules& t,int32_t id,int32_t faction,bool& value){
 return id>=-2 && id<89 && world_has_technology(w,t,id,faction,value)==LogicError::ok;
}
}
TileYieldError world_tile_yield_context(const OriginalWorld& w,const Rules& t,const BasicRules& basic,const ResourceInfo& resources,size_t base_id,int32_t faction,int32_t x,int32_t y,const WorldTileYieldStage& stage,TileYieldContext& output){
 if(!shape(w))return TileYieldError::bad_state;
 if(aliases(w,t,basic,resources,stage,&output,sizeof output))return TileYieldError::bad_buffer;
 if((stage.variant!=Variant::base && stage.variant!=Variant::alien_crossfire) || (stage.variant==Variant::base&&stage.expansion_enabled) || faction<0 || faction>=8 || !valid_technology(t))return TileYieldError::bad_state;
 const auto* base=w.base(base_id);const auto* querying=w.faction((size_t)faction);
 if(!base || !querying || base->faction_id<0 || base->faction_id>=8)return TileYieldError::bad_state;
 const auto* owner=w.faction_definition(base->faction_id);if(!owner)return TileYieldError::bad_state;
 const auto& info=w.game().map.info;MapGeometry geometry{(int32_t)info.width,(int32_t)info.height,info.flat};size_t index;
 if(movement_tile_index(geometry,x,y,index)!=LogicError::ok)return TileYieldError::bad_state;
 const auto* tile=w.tile(index);if(!tile)return TileYieldError::bad_state;
 TileYieldContext next{};next.variant=stage.variant;next.tile=*tile;next.expansion_enabled=stage.expansion_enabled;next.terraform_preview=stage.terraform_preview;
 if(tile_resource_bonus(info,*tile,x,y,w.game().preferences.GameRules,next.bonus)!=TileYieldError::ok)return TileYieldError::bad_state;
 next.planet_pending=querying->SE_planet_pending;next.economy_pending=querying->SE_economy_pending;
 next.fungus[0]=querying->tech_fungus_nutrient;next.fungus[1]=querying->tech_fungus_mineral;next.fungus[2]=querying->tech_fungus_energy;next.base_count=querying->base_count;
 if(next.base_count<0 || basic.limit_mineral_mine_sans_road<0)return TileYieldError::bad_state;
 next.golden_age=((uint32_t)base->state_flags&4u)!=0;next.base_event_flags=(uint32_t)base->event_flags;
 for(unsigned i=0;i<8;++i)next.facilities|=(uint64_t)base->facilities_built[i]<<(8*i);
 next.aquatic=((uint32_t)owner->rule_flags&0x100u)!=0;
 const int32_t thresholds[]={basic.tech_three_nutrients_sqr,basic.tech_three_minerals_sqr,basic.tech_three_energy_sqr};
 for(unsigned i=0;i<3;++i)if(!technology(w,t,thresholds[i],faction,next.allow_three[i]))return TileYieldError::bad_state;
 if(!technology(w,t,basic.tech_mining_platform_bonus,faction,next.sea_mineral_technology))return TileYieldError::bad_state;
 if(stage.variant==Variant::alien_crossfire && world_has_project(w,33,faction,next.manifold_owned)!=LogicError::ok)return TileYieldError::bad_state;
 const auto& prefs=w.game().preferences;const int32_t merchant=prefs.SecretProjects[3];
 if(merchant>=0 && !w.base((size_t)merchant))return TileYieldError::bad_state;
 next.merchant_exchange=merchant>=0 && (size_t)merchant==base_id;next.governor=prefs.GovernorFaction==faction;
 next.dust_cloud=prefs.DustCloudDuration!=0;next.solar_flares=(uint32_t)prefs.SolarFlaresEvent;next.control_upkeep=stage.control_upkeep;
 next.mine_nutrient_effect=basic.tgl_nutrient_effect_with_mine;next.mine_without_road_limit=basic.limit_mineral_mine_sans_road;
 next.terraform_reduction_before=stage.terraform_reduction_before;next.base_energy_before=stage.base_energy_before;
 for(unsigned i=0;i<9;++i){next.resources[i][0]=resources.rows[i].nutrients;next.resources[i][1]=resources.rows[i].minerals;next.resources[i][2]=resources.rows[i].energy;}
 const uint32_t items=tile->items;const bool base_square=(items&1u) && (tile->val2&15u)<8;
 const bool mirrors=!base_square && !(items&(0x2000u|0x1000000u)) && !((items&0x20u)&&(tile->climate&0xe0u)>0x3f) && (tile->climate&0xe0u)>=0x60 && !(items&0x200000u) && (items&0x40u);
 if(mirrors){
  if(stage.current_base<0)return TileYieldError::bad_state;
  const auto* current=w.base((size_t)stage.current_base);if(!current || current->faction_id<0 || current->faction_id>=8)return TileYieldError::bad_state;
  const int32_t dx[]={0,1,2,1,0,-1,-2,-1},dy[]={-2,-1,0,1,2,1,0,-1};
  for(unsigned i=0;i<8;++i){int32_t nx;
   if(movement_wrap_x(geometry,x+dx[i],nx)!=LogicError::ok)return TileYieldError::bad_state;
   size_t neighbor;if(movement_tile_index(geometry,nx,y+dy[i],neighbor)!=LogicError::ok)continue;
   const auto* nearby=w.tile(neighbor);if(!nearby)return TileYieldError::bad_state;
   if((nearby->items&0x800000u) && nearby->owner==current->faction_id)++next.echelon_matches;
  }
 }
 output=next;return TileYieldError::ok;
}
TileYieldError world_tile_yields(const OriginalWorld& w,const Rules& t,const BasicRules& b,const ResourceInfo& r,size_t base,int32_t faction,int32_t x,int32_t y,const WorldTileYieldStage& stage,TileYieldResult& output){
 if(!shape(w))return TileYieldError::bad_state;
 if(aliases(w,t,b,r,stage,&output,sizeof output))return TileYieldError::bad_buffer;
 TileYieldContext context{};const auto error=world_tile_yield_context(w,t,b,r,base,faction,x,y,stage,context);
 if(error!=TileYieldError::ok)return error;
 return calculate_tile_yields(context,output);
}
}
