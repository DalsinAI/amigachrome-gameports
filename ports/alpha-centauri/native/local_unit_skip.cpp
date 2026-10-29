/* SPDX-License-Identifier: GPL-3.0-or-later. Original veh_skip; see header. */
#include "local_session_data.h"
#include "local_accounting_tables.h"
#include "world_unit_stats.h"
#include "technology_effects_ranges.h"
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
LocalOrderError LocalProductionSession::prepare_skip_unit(size_t id,LocalSkipCommand&out)const{
 if(!data_)return LocalOrderError::bad_state;
 if(suspended_)return LocalOrderError::suspended;
 if(detail::effects_range_overlap(&out,sizeof out,this,sizeof(*this)) ||
    detail::effects_range_overlap(&out,sizeof out,data_,sizeof(Data)) ||
    (data_->units&&detail::effects_range_overlap(&out,sizeof out,data_->units,sizeof(UnitDefinitions))) ||
    (data_->accounting&&detail::effects_range_overlap(&out,sizeof out,data_->accounting,sizeof(LocalAccountingTables))) ||
    !disjoint(data_->world,&out,sizeof out))return LocalOrderError::bad_state;
 const UnitDefinitions*units=data_->units?data_->units:data_->accounting?&data_->accounting->units:nullptr;
 if(!units)return LocalOrderError::unsupported;
 const auto*v=data_->world.vehicle(id);if(!v)return LocalOrderError::bad_state;
 if(v->faction_id!=data_->local_human)return LocalOrderError::not_owner;
 uint32_t speed=0;const auto e=world_vehicle_speed(data_->world,*units,data_->technology,id,data_->rules.move_rate_roads,false,speed);
 if(e!=LogicError::ok)return e==LogicError::overflow?LocalOrderError::overflow:LocalOrderError::bad_state;
 if(speed>255)return LocalOrderError::overflow;
 LocalSkipCommand next;next.owner_identity_=identity_;next.revision_=revision_;next.preview_={id,speed,v->moves_spent,(uint8_t)speed};out=next;
 return LocalOrderError::ok;
}
LocalOrderError LocalProductionSession::commit_skip_unit(const LocalSkipCommand&command,LocalSkipReceipt&out){
 if(!data_)return LocalOrderError::bad_state;
 if(suspended_)return LocalOrderError::suspended;
 if(detail::effects_range_overlap(&out,sizeof out,this,sizeof(*this)) ||
    detail::effects_range_overlap(&out,sizeof out,data_,sizeof(Data)) ||
    detail::effects_range_overlap(&out,sizeof out,&command,sizeof command) ||
    (data_->units&&detail::effects_range_overlap(&out,sizeof out,data_->units,sizeof(UnitDefinitions))) ||
    (data_->accounting&&detail::effects_range_overlap(&out,sizeof out,data_->accounting,sizeof(LocalAccountingTables))) ||
    !disjoint(data_->world,&out,sizeof out))return LocalOrderError::bad_state;
 if(!command.owner_identity_ || command.owner_identity_!=identity_ || command.revision_!=revision_)return LocalOrderError::stale_plan;
 LocalSkipCommand checked;const auto e=prepare_skip_unit(command.preview_.vehicle,checked);if(e!=LocalOrderError::ok)return e;
 const auto&a=command.preview_;const auto&b=checked.preview_;
 if(a.speed!=b.speed || a.spent_before!=b.spent_before || a.spent_after!=b.spent_after)return LocalOrderError::stale_plan;
 const bool changed=b.spent_before!=b.spent_after;
 if(changed&&revision_==UINT64_MAX)return LocalOrderError::overflow;
 if(changed){data_->world.vehicles_[b.vehicle].moves_spent=b.spent_after;++revision_;}
 out={changed,changed,changed,revision_,b};return LocalOrderError::ok;
}
}
