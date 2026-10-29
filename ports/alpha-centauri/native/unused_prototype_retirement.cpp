/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "unused_prototype_retirement.h"
#include "technology_effects_ranges.h"
namespace alpha {
static bool overlap(const OriginalWorld& w,const UnusedRetirementProposal& out){
 using detail::effects_range_overlap;using detail::effects_array_overlap;
 return effects_range_overlap(&out,sizeof out,&w,sizeof w) || w.overlaps_byte_storage(&out,sizeof out) ||
 effects_array_overlap(&out,sizeof out,w.tile(0),w.game().map.info.tile_count,sizeof(OriginalMapTile)) ||
 effects_array_overlap(&out,sizeof out,w.vehicle(0),w.game().vehicle_count,sizeof(OriginalVehicle)) ||
 effects_array_overlap(&out,sizeof out,w.base(0),w.game().base_count,sizeof(OriginalBase)) ||
 effects_array_overlap(&out,sizeof out,w.prototype(0),512,sizeof(OriginalPrototype)) ||
 effects_array_overlap(&out,sizeof out,w.faction(0),8,sizeof(OriginalFactionState)) ||
 effects_array_overlap(&out,sizeof out,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));
}
UnusedRetirementError world_unused_prototype_retirement(const OriginalWorld& w,Variant variant,int32_t faction,int32_t id,UnusedRetirementProposal& out){
 if(detail::effects_range_overlap(&out,sizeof out,&w,sizeof w) || !w.loaded() || faction<1 || faction>7 || id<64 || id>=512 || id/64!=faction ||
 (variant!=Variant::base && variant!=Variant::alien_crossfire) || (w.game().data_version<11)!=(variant==Variant::base) || w.game().base_count>512 || overlap(w,out))return UnusedRetirementError::bad_state;
 const auto* state=w.faction((size_t)faction);const auto* design=w.prototype((size_t)id);
 const auto table=w.game().bunglist;
 if(!state || !design || !table.data || table.size!=retirement_history_bytes || state->unk_103<0 || state->unk_103>63)return UnusedRetirementError::bad_state;
 for(size_t i=0;i<w.game().vehicle_count;++i){const auto* v=w.vehicle(i);if(!v)return UnusedRetirementError::bad_state;if(v->unit_id==id)return UnusedRetirementError::live_unit_effects_required;}
 RetirementHistoryResult history{};
 if(retirement_history_stage(table.data,table.size,faction,state->unk_103,*design,history)!=LogicError::ok)return UnusedRetirementError::bad_state;
 // Queue proposal is the only fallible final operation. Its transactional
 // contract leaves out untouched on failure; following scalar/byte copies fail
 // neither allocation nor callbacks. No partial result escapes.
 const auto e=retirement_queue_stage(variant,id,design->unit_flags,w.base(0),w.game().base_count,out.queues);
 if(e!=RetirementQueueError::ok)return e==RetirementQueueError::out_of_memory?UnusedRetirementError::out_of_memory:UnusedRetirementError::bad_state;
 out.history=history;out.faction=faction;out.prototype_id=id;out.ready=true;return UnusedRetirementError::ok;
}
}
