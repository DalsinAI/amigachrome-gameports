/* SPDX-License-Identifier: GPL-3.0-or-later. See world_police.h. */
#include "world_police.h"
#include "technology_effects_ranges.h"
#include <stdlib.h>
namespace alpha { namespace {
using detail::effects_range_overlap;
bool overlap_array(const void* out,size_t n,const void* p,size_t count,size_t size) {
 return detail::effects_array_overlap(out,n,p,count,size);
}
bool sealed(const OriginalWorld& w,const UnitDefinitions& d,const Rules& r,const void* out,size_t n) {
 if (effects_range_overlap(out,n,&w,sizeof w) || effects_range_overlap(out,n,&d,sizeof d) || effects_range_overlap(out,n,&r,sizeof r) || !w.loaded()) return false;
 return !(effects_range_overlap(out,n,w.original_bytes(),w.original_size()) ||
 overlap_array(out,n,w.tile(0),w.game().map.info.tile_count,sizeof(OriginalMapTile)) ||
 overlap_array(out,n,w.vehicle(0),w.game().vehicle_count,sizeof(OriginalVehicle)) ||
 overlap_array(out,n,w.base(0),w.game().base_count,sizeof(OriginalBase)) ||
 overlap_array(out,n,w.prototype(0),512,sizeof(OriginalPrototype)) ||
 overlap_array(out,n,w.faction(0),8,sizeof(OriginalFactionState)) ||
 overlap_array(out,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition)));
}
bool tables(const OriginalWorld& w,const UnitDefinitions& d,const Rules& r) {
 const bool expansion=w.game().data_version>=11;
 return r.count==89 && d.weapon_count==(expansion?26u:21u) && d.ability_count==(expansion?29u:24u);
}
PoliceError ability(const OriginalWorld& w,const UnitDefinitions& d,const Rules& r,size_t id,bool& out) {
 const auto* p=w.prototype(id);if (!p) return PoliceError::bad_state;
 if (p->ability_flags&0x400000u) { out=true;return PoliceError::ok; }
 if (w.game().data_version<11 || id<64) { out=false;return PoliceError::ok; }
 const int32_t owner=(int32_t)(id/64);const auto& f=*w.faction_definition((size_t)owner);
 if (f.faction_bonus_count<0 || f.faction_bonus_count>8) return PoliceError::bad_state;
 for (int32_t i=0;i<f.faction_bonus_count;++i) if (f.faction_bonus_id[i]==15) {
  const int32_t slot=f.faction_bonus_val1[i];if (slot<0 || slot>=(int32_t)d.ability_count) return PoliceError::bad_state;
  const auto tech=d.abilities[slot].prerequisite;if (tech<-2 || tech>=89) return PoliceError::bad_state;
  bool known;if (world_has_technology(w,r,tech,owner,known)!=LogicError::ok) return PoliceError::bad_state;
  if (known && slot==22) { out=true;return PoliceError::ok; }
 }
 out=false;return PoliceError::ok;
}
}
PoliceError world_double_police(const OriginalWorld& w,const UnitDefinitions& d,const Rules& r,size_t id,bool& out) {
 if (!sealed(w,d,r,&out,sizeof out)) return PoliceError::bad_buffer;
 if (!tables(w,d,r)) return PoliceError::bad_state;
 bool next;const auto error=ability(w,d,r,id,next);if (error==PoliceError::ok) out=next;return error;
}
PoliceError world_police_garrison(const OriginalWorld& w,const UnitDefinitions& d,const Rules& r,size_t base,
 PoliceSnapshotProfile profile,WorldPoliceGarrison& out) {
 if (!sealed(w,d,r,&out,sizeof out)) return PoliceError::bad_buffer;
 if (!tables(w,d,r) || !w.base(base) || profile!=PoliceSnapshotProfile::offline_validated || w.game().vehicle_count>2048) return PoliceError::bad_state;
 const auto& b=*w.base(base);const size_t tile=((size_t)b.y*w.game().map.info.width+(size_t)b.x)/2;
 const auto* square=w.tile(tile);if (!square) return PoliceError::bad_state;
 size_t count=0;int32_t head=-1;
 for (size_t i=0;i<w.game().vehicle_count;++i) {
  const auto& v=*w.vehicle(i);if (v.x!=b.x || v.y!=b.y) continue;
  ++count;if (v.prev_veh_id_stack<0) { if (head>=0) return PoliceError::bad_state;head=(int32_t)i; }
 }
 if (bool(square->items&2u)!=bool(count) || (count && head<0)) return PoliceError::bad_state;
 // Validate the complete selected stack before allocating or resolving grants.
 int32_t at=head,previous=-1;size_t walked=0;
 while (at>=0) {
  const auto* v=w.vehicle((size_t)at);
  if (!v || ++walked>count || v->x!=b.x || v->y!=b.y || v->prev_veh_id_stack!=previous) return PoliceError::bad_state;
  previous=at;at=v->next_veh_id_stack;
 }
 if (walked!=count) return PoliceError::bad_state;
 auto* next=(WorldPoliceGarrison*)calloc(1,sizeof(WorldPoliceGarrison));if (!next) return PoliceError::out_of_memory;
 next->variant=w.game().data_version<11?Variant::base:Variant::alien_crossfire;next->base=(int32_t)base;next->count=(uint32_t)count;
 at=head;PoliceError error=PoliceError::ok;
 for (size_t i=0;i<count;++i) {
  const auto& v=*w.vehicle((size_t)at);const auto* p=w.prototype((size_t)v.unit_id);
  if (!p || p->chassis_id>=9 || p->weapon_id>=d.weapon_count) { error=PoliceError::bad_state;break; }
  const int32_t triad=d.chassis[p->chassis_id].triad,offense=d.weapons[p->weapon_id].offense;
  if (triad<0 || triad>2 || offense<-128 || offense>127) { error=PoliceError::bad_state;break; }
  bool doubled=false;
  if ((uint8_t)p->plan<4) { error=ability(w,d,r,(size_t)v.unit_id,doubled);if (error!=PoliceError::ok) break; }
  next->vehicle_ids[i]=at;next->units[i]={v.unit_id,(uint8_t)p->plan,offense,triad,doubled};at=v.next_veh_id_stack;
 }
 if (error==PoliceError::ok) out=*next;
 free(next);return error;
}
}
