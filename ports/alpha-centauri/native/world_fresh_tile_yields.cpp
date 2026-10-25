/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_fresh_tile_yields.h"
#include "technology_effects_ranges.h"
namespace alpha { namespace {
bool aliases(const OriginalWorld&w,const Rules&t,const BasicRules&b,const ResourceInfo&r,const WorldTileYieldStage&s,const void*out,size_t n){
 using detail::effects_range_overlap;using detail::effects_array_overlap;
 if(effects_range_overlap(out,n,&w,sizeof w)||effects_range_overlap(out,n,&t,sizeof t)||effects_range_overlap(out,n,&b,sizeof b)||effects_range_overlap(out,n,&r,sizeof r)||effects_range_overlap(out,n,&s,sizeof s))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();
 return effects_range_overlap(out,n,w.original_bytes(),w.original_size())||
  effects_array_overlap(out,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))||
  effects_array_overlap(out,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))||
  effects_array_overlap(out,n,w.base(0),g.base_count,sizeof(OriginalBase))||
  effects_array_overlap(out,n,w.prototype(0),512,sizeof(OriginalPrototype))||
  effects_array_overlap(out,n,w.faction(0),8,sizeof(OriginalFactionState))||
  effects_array_overlap(out,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));
}
}
TileYieldError world_fresh_tile_yield_context(const OriginalWorld&w,const Rules&t,const BasicRules&b,const ResourceInfo&r,size_t base,int32_t faction,int32_t x,int32_t y,const WorldTileYieldStage&s,TileYieldContext&output){
 if(aliases(w,t,b,r,s,&output,sizeof output))return TileYieldError::bad_buffer;
 TileYieldContext next{};const auto context=world_tile_yield_context(w,t,b,r,base,faction,x,y,s,next);
 if(context!=TileYieldError::ok)return context;
 TechnologyEffectsResult effects{};const auto error=world_technology_effects(w,t,faction,effects);
 if(error==LogicError::overflow)return TileYieldError::overflow;
 if(error!=LogicError::ok)return TileYieldError::bad_state;
 next.fungus[0]=effects.fungus_nutrient;next.fungus[1]=effects.fungus_mineral;next.fungus[2]=effects.fungus_energy;
 output=next;return TileYieldError::ok;
}
TileYieldError world_fresh_tile_yields(const OriginalWorld&w,const Rules&t,const BasicRules&b,const ResourceInfo&r,size_t base,int32_t faction,int32_t x,int32_t y,const WorldTileYieldStage&s,TileYieldResult&output){
 if(aliases(w,t,b,r,s,&output,sizeof output))return TileYieldError::bad_buffer;
 TileYieldContext context{};const auto error=world_fresh_tile_yield_context(w,t,b,r,base,faction,x,y,s,context);
 if(error!=TileYieldError::ok)return error;
 return calculate_tile_yields(context,output);
}
}
