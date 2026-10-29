/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "local_session_data.h"
#include "local_accounting_tables.h"
#include "technology_effects_ranges.h"
#include <string.h>
namespace alpha {namespace {
bool disjoint(const OriginalWorld&w,const void*p,size_t n){
 using detail::effects_range_overlap;using detail::effects_array_overlap;const auto&g=w.game();
 return !effects_range_overlap(p,n,&w,sizeof w)&&!w.overlaps_byte_storage(p,n)&&
 !effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))&&
 !effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))&&
 !effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))&&
 !effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))&&
 !effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))&&
 !effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));
}
}

LocalOrderError LocalProductionSession::prepare_ready_search(const ReadySearchFacts&facts,ReadySearchPlan&out)const{
 if(!data_)return LocalOrderError::bad_state;
 if(suspended_)return LocalOrderError::suspended;
 if(data_->world.game().preferences.FactionStatus[0]!=(1u<<(unsigned)data_->local_human))return LocalOrderError::unsupported;
 if(detail::effects_range_overlap(&out,sizeof out,this,sizeof(*this)) || detail::effects_range_overlap(&out,sizeof out,data_,sizeof(Data)) ||
    detail::effects_range_overlap(&out,sizeof out,&facts,sizeof facts) ||
    (data_->units&&detail::effects_range_overlap(&out,sizeof out,data_->units,sizeof(UnitDefinitions))) ||
    (data_->accounting&&detail::effects_range_overlap(&out,sizeof out,data_->accounting,sizeof(LocalAccountingTables))) ||
    !disjoint(data_->world,&out,sizeof out))return LocalOrderError::bad_state;
 const UnitDefinitions*units=data_->units?data_->units:data_->accounting?&data_->accounting->units:nullptr;
 if(!units)return LocalOrderError::unsupported;
 ReadySearchPlan next;const auto e=world_ready_search(data_->world,*units,data_->technology,data_->local_human,data_->rules.move_rate_roads,facts,next.result_);
 if(e!=LogicError::ok)return e==LogicError::overflow?LocalOrderError::overflow:LocalOrderError::bad_state;
 next.owner_identity_=identity_;next.revision_=revision_;next.facts_=facts;out=next;return LocalOrderError::ok;
}
LocalOrderError LocalProductionSession::commit_ready_search(const ReadySearchPlan&plan,ReadySearchReceipt&out){
 if(!data_)return LocalOrderError::bad_state;
 if(suspended_)return LocalOrderError::suspended;
 if(detail::effects_range_overlap(&out,sizeof out,this,sizeof(*this)) || detail::effects_range_overlap(&out,sizeof out,data_,sizeof(Data)) ||
    detail::effects_range_overlap(&out,sizeof out,&plan,sizeof plan) ||
    (data_->units&&detail::effects_range_overlap(&out,sizeof out,data_->units,sizeof(UnitDefinitions))) ||
    (data_->accounting&&detail::effects_range_overlap(&out,sizeof out,data_->accounting,sizeof(LocalAccountingTables))) ||
    !disjoint(data_->world,&out,sizeof out))return LocalOrderError::bad_state;
 if(!plan.owner_identity_ || plan.owner_identity_!=identity_ || plan.revision_!=revision_)return LocalOrderError::stale_plan;
 ReadySearchPlan checked;const auto e=prepare_ready_search(plan.facts_,checked);if(e!=LocalOrderError::ok)return e;
 const auto&a=plan.result_;const auto&b=checked.result_;
 if(a.selected!=b.selected || a.distance!=b.distance || a.passes!=b.passes || memcmp(a.clear_deferred,b.clear_deferred,sizeof a.clear_deferred))return LocalOrderError::stale_plan;
 bool changed=false;for(auto byte:b.clear_deferred)changed|=byte!=0;
 if(changed&&revision_==UINT64_MAX)return LocalOrderError::overflow;
 if(changed){for(size_t i=0;i<data_->world.game().vehicle_count;++i)if(b.clear_deferred[i/8]&(1u<<(i%8)))data_->world.vehicles_[i].state&=~2u;++revision_;}
 out={changed,changed,changed,revision_,b};return LocalOrderError::ok;
}
}
