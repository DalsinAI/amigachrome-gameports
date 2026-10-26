/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_world_masks.h"
namespace alpha {namespace {
bool overlap(const void*a,size_t n,const void*b,size_t m){if(!n||!m)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(x<y+m&&y<x+n);}
const OriginalMapTile* tile(const OriginalWorld&w,int x,int y){const auto&m=w.game().map.info;if(!m.flat){if(x<0)x+=(int)m.width;else if(x>=(int)m.width)x-=(int)m.width;}if(x<0||y<0||x>=(int)m.width||y>=(int)m.height||((x+y)&1))return nullptr;return w.tile(((size_t)y*m.width+(unsigned)x)/2);}
}
bool world_terrain_masks(const OriginalWorld&w,int x,int y,const TerrainMaskView&view,TerrainWorldMasks&out){
 const auto&m=w.game().map.info;if(!view.supplied||!w.loaded()||x<0||y<0||x>=(int)m.width||y>=(int)m.height||((x+y)&1))return false;
 if(overlap(&out,sizeof out,&view,sizeof view)||overlap(&out,sizeof out,&w,sizeof w)||overlap(&out,sizeof out,w.original_bytes(),w.original_size())||overlap(&out,sizeof out,w.tile(0),(size_t)m.tile_count*sizeof(OriginalMapTile))||overlap(&out,sizeof out,w.vehicle(0),(size_t)w.game().vehicle_count*sizeof(OriginalVehicle))||overlap(&out,sizeof out,w.base(0),(size_t)w.game().base_count*sizeof(OriginalBase))||overlap(&out,sizeof out,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(&out,sizeof out,w.faction(0),8*sizeof(OriginalFactionState))||overlap(&out,sizeof out,w.faction_definition(0),8*sizeof(OriginalFactionDefinition)))return false;
 const auto*centre=tile(w,x,y);if(!centre)return false;const bool land=(centre->climate>>5)>=3;const unsigned rough=(centre->climate>>3)&3;const bool jungle=(centre->landmarks&0x80000004u)==4;TerrainWorldMasks result{};
 const int dx[8]={1,2,1,0,-1,-2,-1,0},dy[8]={-1,0,1,2,1,0,-1,-2};
 if(land)for(unsigned k=0;k<8;++k){const auto*t=tile(w,x+dx[k],y+dy[k]);if(!t||(t->climate>>5)>=3)continue;const unsigned sector=k/2;if(k&1)result.coast[(sector+2)&3]|=2;else{result.coast[(sector+1)&3]|=4;result.coast[(sector+2)&3]|=1;}}
 const bool scan=land?(rough||(view.displayed_items&0x20u)||jungle):(view.displayed_items&0x20u)!=0;
 if(scan)for(unsigned k=0;k<8;++k){const unsigned d=(k+7)&7;const auto*t=tile(w,x+dx[d],y+dy[d]);if(!t)continue;const unsigned level=t->climate>>5;const uint8_t bit=(uint8_t)(1u<<k);if((t->items&0x20u)&&level>=2)result.fungus|=bit;if(!land)continue;if(jungle&&(t->landmarks&0x80000004u)==4&&level>=3)result.jungle|=bit;if(rough&&(level<3||((t->climate>>3)&3)>=rough))result.roughness|=bit;}
 if(land&&(view.displayed_items&0x200000u))for(unsigned k=0;k<4;++k){const auto*t=tile(w,x+dx[k*2],y+dy[k*2]);if(t&&(t->items&0x200000u))result.forest|=(uint8_t)(1u<<k);}
 if(land&&(centre->items&0x80u))for(unsigned k=0;k<4;++k){const auto*t=tile(w,x+dx[k*2],y+dy[k*2]);if(t&&(t->items&0x80u))result.river|=(uint8_t)(1u<<k);}
 out=result;return true;
}
}
