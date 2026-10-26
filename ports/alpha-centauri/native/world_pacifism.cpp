/* SPDX-License-Identifier: GPL-3.0-or-later. See world_pacifism.h. */
#include "world_pacifism.h"
#include "technology_effects_ranges.h"
namespace alpha { namespace {
using detail::effects_range_overlap;
bool array_overlap(const void* out,size_t n,const void* p,size_t count,size_t size) {
 return detail::effects_array_overlap(out,n,p,count,size);
}
bool sealed(const OriginalWorld& w,const UnitDefinitions& d,const void* out,size_t n) {
 if (effects_range_overlap(out,n,&w,sizeof w) || effects_range_overlap(out,n,&d,sizeof d) || !w.loaded()) return false;
 return !(w.overlaps_byte_storage(out,n) ||
 array_overlap(out,n,w.tile(0),w.game().map.info.tile_count,sizeof(OriginalMapTile)) ||
 array_overlap(out,n,w.vehicle(0),w.game().vehicle_count,sizeof(OriginalVehicle)) ||
 array_overlap(out,n,w.base(0),w.game().base_count,sizeof(OriginalBase)) ||
 array_overlap(out,n,w.prototype(0),512,sizeof(OriginalPrototype)) ||
 array_overlap(out,n,w.faction(0),8,sizeof(OriginalFactionState)) ||
 array_overlap(out,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition)));
}
}
LogicError world_pacifism(const OriginalWorld& w,const UnitDefinitions& d,size_t base,WorldPacifism& out) {
 if (!sealed(w,d,&out,sizeof out)) return LogicError::bad_state;
 const auto* b=w.base(base);const bool expansion=w.game().data_version>=11;
 if (!b || w.game().vehicle_count>2048 || d.weapon_count!=(expansion?26u:21u)) return LogicError::bad_state;
 WorldPacifism next{expansion?Variant::alien_crossfire:Variant::base,(int32_t)base,0,0,0,0,0};
 for (size_t i=0;i<w.game().vehicle_count;++i) {
  const auto& v=*w.vehicle(i);if (v.faction_id!=(uint8_t)b->faction_id || v.home_base_id!=(int32_t)base) continue;
  const auto* p=w.prototype((size_t)v.unit_id);if (!p) return LogicError::bad_state;
  const uint8_t plan=(uint8_t)p->plan;if (plan>=10) continue;
  if (p->weapon_id>=d.weapon_count) return LogicError::bad_state;
  const int32_t offense=d.weapons[p->weapon_id].offense;if (offense<-128 || offense>127) return LogicError::bad_state;
  if (!offense) continue;
  if (p->chassis_id>=9 || d.chassis[p->chassis_id].triad<0 || d.chassis[p->chassis_id].triad>2) return LogicError::bad_state;
  const size_t index=((size_t)v.y*w.game().map.info.width+(size_t)v.x)/2;const auto* tile=w.tile(index);if (!tile) return LogicError::bad_state;
  const bool valid_base=(tile->items&1u) && (tile->val2&15u)<8;
  if (!valid_base && tile->owner>7) return LogicError::bad_state;
  const bool away=!valid_base && !(tile->owner>=1 && tile->owner==b->faction_id);
  const bool air=d.chassis[p->chassis_id].triad==2 && plan!=4;
  ++next.home_combat;next.away+=away;next.air+=air;next.both+=away&&air;
  if (away || air) ++next.count;
 }
 out=next;return LogicError::ok;
}
}
