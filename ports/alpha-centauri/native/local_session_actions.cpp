/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "local_session_data.h"
namespace alpha {
namespace {
bool overlaps(const void* a,size_t n,const void* b,size_t m){
 if(!n || !m)return false;
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return n>UINTPTR_MAX-x || m>UINTPTR_MAX-y || (x<y+m && y<x+n);
}
bool world_alias(const OriginalWorld& w,const void* p,size_t n){
 return overlaps(p,n,&w,sizeof w) || overlaps(p,n,w.original_bytes(),w.original_size()) ||
  overlaps(p,n,w.tile(0),w.game().map.info.tile_count*sizeof(OriginalMapTile)) ||
  overlaps(p,n,w.vehicle(0),w.game().vehicle_count*sizeof(OriginalVehicle)) ||
  overlaps(p,n,w.base(0),w.game().base_count*sizeof(OriginalBase)) ||
  overlaps(p,n,w.prototype(0),512*sizeof(OriginalPrototype)) ||
  overlaps(p,n,w.faction(0),8*sizeof(OriginalFactionState)) ||
  overlaps(p,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
}
LocalOrderError LocalProductionSession::prepare_clear_land_step(size_t vehicle,int32_t x,int32_t y,LocalStepCommand& output)const{
 if(!data_)return LocalOrderError::bad_state;
 if(suspended_)return LocalOrderError::suspended;
 if(!data_->units || data_->profile!=Variant::base || data_->expansion_enabled)return LocalOrderError::unsupported;
 const auto* unit=data_->world.vehicle(vehicle);if(!unit)return LocalOrderError::bad_state;
 if(unit->faction_id!=data_->local_human)return LocalOrderError::not_owner;
 MapGeometry geometry{(int32_t)data_->world.game().map.info.width,(int32_t)data_->world.game().map.info.height,data_->world.game().map.info.flat};
 size_t tile;if(movement_tile_index(geometry,x,y,tile)!=LogicError::ok)return LocalOrderError::bad_state;
 LocalStepCommand next;next.owner_identity_=identity_;next.revision_=revision_;next.intent_={vehicle,x,y};output=next;return LocalOrderError::ok;
}
LocalOrderError LocalProductionSession::commit_clear_land_step(const LocalStepCommand& command,LocalStepReceipt& output){
 if(!data_)return LocalOrderError::bad_state;
 if(suspended_)return LocalOrderError::suspended;
 if(!command.owner_identity_ || command.owner_identity_!=identity_ || command.revision_!=revision_)return LocalOrderError::stale_plan;
 if(command.surface_)return LocalOrderError::unsupported;
 if(!data_->units || data_->profile!=Variant::base || data_->expansion_enabled)return LocalOrderError::unsupported;
 if(revision_==UINT64_MAX)return LocalOrderError::overflow;
 const auto result=WorldActions::clear_land_step(data_->world,*data_->units,data_->technology,command.intent_.vehicle,data_->local_human,data_->rules.move_rate_roads,command.intent_.x,command.intent_.y);
 const bool moved=result.status==StepStatus::moved;
 // The inherited bounded action validates every rejection before any write.
 // No possible error remains after it moves. A rejected command is inspected
 // through StepResult; it does not invalidate an otherwise-current plan.
 if(moved)++revision_;
 output={moved,moved,moved,revision_,result};return LocalOrderError::ok;
}
LocalOrderError LocalProductionSession::prepare_surface_step(size_t vehicle,int32_t x,int32_t y,LocalStepCommand& output)const{
 if(!data_)return LocalOrderError::bad_state;
 if(suspended_)return LocalOrderError::suspended;
 if(!surface_actions_enabled())return LocalOrderError::unsupported;
 if(overlaps(&output,sizeof output,this,sizeof(*this)) || overlaps(&output,sizeof output,data_,sizeof(Data)) ||
    overlaps(&output,sizeof output,data_->units,sizeof(UnitDefinitions)) || world_alias(data_->world,&output,sizeof output))
  return LocalOrderError::bad_state;
 LocalStepCommand next;const auto error=prepare_clear_land_step(vehicle,x,y,next);
 if(error!=LocalOrderError::ok)return error;
 next.surface_=true;output=next;return LocalOrderError::ok;
}
LocalOrderError LocalProductionSession::commit_surface_step(const LocalStepCommand& command,LocalStepReceipt& output){
 if(!data_)return LocalOrderError::bad_state;
 if(suspended_)return LocalOrderError::suspended;
 if(overlaps(&command,sizeof command,this,sizeof(*this)) || overlaps(&command,sizeof command,data_,sizeof(Data)) ||
    overlaps(&command,sizeof command,data_->units,data_->units?sizeof(UnitDefinitions):0) ||
    world_alias(data_->world,&command,sizeof command) ||
    overlaps(&output,sizeof output,this,sizeof(*this)) || overlaps(&output,sizeof output,data_,sizeof(Data)) ||
    overlaps(&output,sizeof output,data_->units,data_->units?sizeof(UnitDefinitions):0) ||
    world_alias(data_->world,&output,sizeof output) || overlaps(&output,sizeof output,&command,sizeof command))
  return LocalOrderError::bad_state;
 if(!command.owner_identity_ || command.owner_identity_!=identity_ || command.revision_!=revision_)return LocalOrderError::stale_plan;
 if(!command.surface_ || !surface_actions_enabled())return LocalOrderError::unsupported;
 if(revision_==UINT64_MAX)return LocalOrderError::overflow;
 const auto* v=data_->world.vehicle(command.intent_.vehicle);
 const auto* p=v && v->unit_id>=0?data_->world.prototype((size_t)v->unit_id):nullptr;
 const bool sea=p && p->chassis_id<9 && data_->units->chassis[p->chassis_id].triad==1;
 const auto action=sea?WorldActions::clear_sea_step:WorldActions::clear_land_step;
 const auto result=action(data_->world,*data_->units,data_->technology,command.intent_.vehicle,data_->local_human,
  data_->rules.move_rate_roads,command.intent_.x,command.intent_.y);
 const bool moved=result.status==StepStatus::moved;
 if(moved)++revision_;
 output={moved,moved,moved,revision_,result};return LocalOrderError::ok;
}
}
