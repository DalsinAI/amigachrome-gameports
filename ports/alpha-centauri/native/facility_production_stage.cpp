/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution and scope in header. */
#include "facility_production_stage.h"
#include "world_facility_options.h"
#include <limits.h>
namespace alpha { namespace {
bool overlap(const void* a,size_t n,const void* b,size_t m){
 if(!n || !m)return false;
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return n>UINTPTR_MAX-x || m>UINTPTR_MAX-y || (x<y+m && y<x+n);
}
bool disjoint(const OriginalWorld& w,const void* p,size_t n){
 return !overlap(p,n,&w,sizeof(w)) && !overlap(p,n,w.original_bytes(),w.original_size()) &&
  !overlap(p,n,w.tile(0),w.game().map.info.tile_count*sizeof(OriginalMapTile)) &&
  !overlap(p,n,w.vehicle(0),w.game().vehicle_count*sizeof(OriginalVehicle)) &&
  !overlap(p,n,w.base(0),w.game().base_count*sizeof(OriginalBase)) &&
  !overlap(p,n,w.prototype(0),512*sizeof(OriginalPrototype)) &&
  !overlap(p,n,w.faction(0),8*sizeof(OriginalFactionState)) &&
  !overlap(p,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
int32_t signed_flags(uint32_t n){return n<=INT32_MAX?(int32_t)n:-1-(int32_t)(~n);}
}
FacilityProductionStage::FacilityProductionStage(OriginalWorld& w,const Rules& t,const BasicRules& r,const FacilityDefinitions& f):
 world_(w),technologies_(t),rules_(r),facilities_(f),source_(w.original_bytes()),source_size_(w.original_size()),base_count_(w.game().base_count){}
FacilityProductionError FacilityProductionStage::apply(size_t id,const FreshFacilityMinerals& fresh,FacilityStageReceipt& output){
 if(!world_.loaded() || source_!=world_.original_bytes() || source_size_!=world_.original_size() || base_count_!=world_.game().base_count)
  return FacilityProductionError::stale_world;
 if(!disjoint(world_,&fresh,sizeof(fresh)) || !disjoint(world_,&output,sizeof(output)) ||
    overlap(&fresh,sizeof(fresh),&output,sizeof(output)) || overlap(this,sizeof(*this),&fresh,sizeof(fresh)) ||
    overlap(this,sizeof(*this),&output,sizeof(output)) || overlap(&technologies_,sizeof(technologies_),&output,sizeof(output)) ||
    overlap(&rules_,sizeof(rules_),&output,sizeof(output)) || overlap(&facilities_,sizeof(facilities_),&output,sizeof(output)))return FacilityProductionError::bad_buffer;
 if(id>=base_count_ || id>=512)return FacilityProductionError::bad_state;
 if(fresh.intake<0 || fresh.surplus>fresh.intake)return FacilityProductionError::bad_state;
 if(processed_[id])return FacilityProductionError::already_processed;
 const auto& game=world_.game();const int32_t human=game.player;
 if(game.data_version>=11)return FacilityProductionError::unsupported;
 if(human<1 || human>7 || !(game.preferences.FactionStatus[0]&(1u<<(unsigned)human)) || game.preferences.CurrentFaction!=human)
  return FacilityProductionError::not_human;
 const auto& b=*world_.base(id);
 if(b.faction_id!=human)return FacilityProductionError::not_owner;
 if(b.queue_size!=0 || ((uint32_t)b.governor_flags&0x80000000u) || ((uint32_t)b.state_flags&0x10000000u))return FacilityProductionError::unsupported;
 const int32_t facility=b.queue_items[0]==-2?2:b.queue_items[0]==-3?3:0;
 if(!facility || (size_t)facility>=facilities_.slot_count)return FacilityProductionError::unsupported;
 WorldFacilityOptions options{};
 const auto option_error=world_facility_options(world_,technologies_,facilities_,Variant::base,false,human,(int32_t)id,0,options);
 if(option_error!=LogicError::ok)return option_error==LogicError::overflow?FacilityProductionError::overflow:FacilityProductionError::bad_state;
 if(!options.available[facility])return FacilityProductionError::unavailable;
 const auto& faction=*world_.faction((size_t)human);
 FacilityProductionFacts facts{b.minerals_accumulated,b.minerals_accumulated_2,fresh.intake,fresh.surplus,
  facilities_.slots[facility].cost,fresh.cost_factor,rules_.retool_exemption,faction.best_mineral_output,
  (uint32_t)b.state_flags,true,b.nutrients_accumulated<0,((uint32_t)b.state_flags&2u)!=0};
 FacilityStageReceipt next{};next.base=(int32_t)id;next.facility=facility;next.before=b.minerals_accumulated;
 const auto error=facility_production(facts,next.production);if(error!=FacilityProductionError::ok)return error;
 next.recompute_required=next.production.completed;next.production_choice_required=next.production.completed;
 // Every rejection above precedes the first state/ledger write.
 auto& base=world_.bases_[id];auto& owner=world_.factions_[(size_t)human];
 base.minerals_accumulated=next.production.accumulated;base.minerals_accumulated_2=next.production.baseline;
 base.state_flags=signed_flags(next.production.state_flags);owner.best_mineral_output=next.production.best_output;
 if(next.production.completed)base.facilities_built[(unsigned)facility/8]|=(uint8_t)(1u<<((unsigned)facility%8));
 processed_[id]=true;output=next;return FacilityProductionError::ok;
}
}
