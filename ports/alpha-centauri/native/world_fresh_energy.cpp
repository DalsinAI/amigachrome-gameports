/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_fresh_energy.h"
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){if(!a||!b||!an||!bn)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
bool aliases(const OriginalWorld&w,const Rules&t,const BasicRules&b,const ResourceInfo&r,const FreshEnergyStage&s,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&t,sizeof t)||overlap(out,n,&b,sizeof b)||overlap(out,n,&r,sizeof r)||overlap(out,n,&s,sizeof s))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();return overlap(out,n,w.original_bytes(),w.original_size())||overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
StableIntakeError convert(EnergyError e){return e==EnergyError::overflow?StableIntakeError::overflow:e==EnergyError::bad_buffer?StableIntakeError::bad_buffer:e==EnergyError::ok?StableIntakeError::ok:StableIntakeError::bad_state;}
}
StableIntakeError world_fresh_energy(const OriginalWorld&w,const Rules&t,const BasicRules&b,const ResourceInfo&r,size_t id,const FreshEnergyStage&s,FreshEnergyResult&output){
 if(aliases(w,t,b,r,s,&output,sizeof output))return StableIntakeError::bad_buffer;
 FreshEnergyResult next{};auto e=world_convoy_flows(w,t,b,r,id,s.intake,next.convoys);if(e!=StableIntakeError::ok)return e;
 auto stage=s.intake;stage.terraform_reduction_before=next.convoys.terraform_reduction_after;stage.base_energy_before=next.convoys.base_energy_after;
 e=world_stable_intake(w,t,b,r,id,stage,next.intake);if(e!=StableIntakeError::ok)return e;
 UpkeepWorldEnergyStage accounting{next.intake.intake[2],next.convoys.to[2],s.commerce,s.energy_grid,next.convoys.from[2],s.intake.worker.upkeep_state};
 auto error=world_energy_accounting(w,id,accounting,next.energy);if(error!=EnergyError::ok)return convert(error);
 error=world_energy_shortfall_orders(w,id,next.energy.accounting.surplus,next.shortfall);if(error!=EnergyError::ok)return convert(error);
 output=next;return StableIntakeError::ok;
}
}
