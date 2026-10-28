/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "local_session_data.h"
#include "local_accounting_tables.h"
#include "technology_effects_ranges.h"
namespace alpha {
LocalOrderError LocalProductionSession::unit_readiness(size_t id,UnitReadiness&out)const{
 if(!data_)return LocalOrderError::bad_state;
 if(suspended_)return LocalOrderError::suspended;
 if(detail::effects_range_overlap(&out,sizeof out,this,sizeof(*this)) ||
    detail::effects_range_overlap(&out,sizeof out,data_,sizeof(Data)) ||
    (data_->accounting&&detail::effects_range_overlap(&out,sizeof out,data_->accounting,sizeof(LocalAccountingTables))) ||
    (data_->units&&detail::effects_range_overlap(&out,sizeof out,data_->units,sizeof(UnitDefinitions))))return LocalOrderError::bad_state;
 const UnitDefinitions*units=data_->units?data_->units:data_->accounting?&data_->accounting->units:nullptr;
 if(!units)return LocalOrderError::unsupported;
 const auto e=world_unit_readiness(data_->world,*units,data_->technology,id,data_->local_human,data_->rules.move_rate_roads,out);
 return e==LogicError::ok?LocalOrderError::ok:e==LogicError::overflow?LocalOrderError::overflow:LocalOrderError::bad_state;
}
}
