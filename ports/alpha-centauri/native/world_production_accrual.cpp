/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_production_accrual.h"
#include "technology_effects_ranges.h"
namespace alpha { namespace {
bool aliases(const OriginalWorld& w,const void* out,size_t n){
 using detail::effects_range_overlap;using detail::effects_array_overlap;
 if(effects_range_overlap(out,n,&w,sizeof w))return true;
 if(!w.loaded())return false;
 const auto& g=w.game();
 return w.overlaps_byte_storage(out,n) ||
 effects_array_overlap(out,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile)) ||
 effects_array_overlap(out,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle)) ||
 effects_array_overlap(out,n,w.base(0),g.base_count,sizeof(OriginalBase)) ||
 effects_array_overlap(out,n,w.prototype(0),512,sizeof(OriginalPrototype)) ||
 effects_array_overlap(out,n,w.faction(0),8,sizeof(OriginalFactionState)) ||
 effects_array_overlap(out,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));
}
}
LogicError world_production_accrual(const OriginalWorld& w,Variant profile,size_t base,
 ProductionAccrualResult& output){
 if(aliases(w,&output,sizeof output) || !w.loaded() ||
 (profile!=Variant::base && profile!=Variant::alien_crossfire) ||
 ((w.game().data_version>=11)!=(profile==Variant::alien_crossfire)))return LogicError::bad_state;
 const auto* b=w.base(base);if(!b || b->faction_id<0 || b->faction_id>7)return LogicError::bad_state;
 const auto* f=w.faction((size_t)b->faction_id);if(!f)return LogicError::bad_state;
 const ProductionAccrualInput in{profile,(uint32_t)b->state_flags,b->mineral_intake_2,b->mineral_surplus,
 b->nutrients_accumulated,b->minerals_accumulated,b->queue_items[0],f->best_mineral_output};
 return production_accrual(in,output);
}
}
