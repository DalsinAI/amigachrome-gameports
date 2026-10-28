/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Fresh-stage tile yields over world_technology_effects, split out of
 * world_tile_yields.cpp so callers of the saved-query API do not need to link
 * the technology-effects engine. Behaviour is unchanged. */
#include "world_tile_yields.h"
#include "world_technology_effects.h"
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
}
TileYieldError world_upkeep_fresh_tile_yields(const OriginalWorld&w,const Rules&t,const BasicRules&b,
 const ResourceInfo&r,size_t base,int32_t faction,int32_t x,int32_t y,
 const WorldTileYieldStage&stage,TileYieldResult&output){
 if(!shape(w))return TileYieldError::bad_state;
 if(aliases(w,t,b,r,stage,&output,sizeof output))return TileYieldError::bad_buffer;
 TileYieldContext context{};
 const auto e=world_tile_yield_context(w,t,b,r,base,faction,x,y,stage,context);
 if(e!=TileYieldError::ok)return e;
 TechnologyEffects effects{};const auto te=world_technology_effects(w,t,faction,effects);
 if(te==TechnologyEffectsError::overflow)return TileYieldError::overflow;
 if(te!=TechnologyEffectsError::ok)return TileYieldError::bad_state;
 for(unsigned i=0;i<3;++i)context.fungus[i]=effects.fungus[i];
 return calculate_tile_yields(context,output);
}
}
