/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "local_session_data.h"
namespace alpha {
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
}
