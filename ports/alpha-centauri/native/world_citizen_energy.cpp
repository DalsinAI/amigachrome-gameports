/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_citizen_energy.h"
#include "world_unit_stats.h"
#include "technology_effects_ranges.h"
namespace alpha { namespace {
using detail::effects_range_overlap;
bool array_overlap(const WorldCitizenEnergy& out,const void* p,size_t count,size_t size) {
 return detail::effects_array_overlap(&out,sizeof out,p,count,size);
}
bool world_overlap(const OriginalWorld& w,const WorldCitizenEnergy& out) {
 return effects_range_overlap(&out,sizeof out,w.original_bytes(),w.original_size()) ||
 array_overlap(out,w.tile(0),w.game().map.info.tile_count,sizeof(OriginalMapTile)) ||
 array_overlap(out,w.vehicle(0),w.game().vehicle_count,sizeof(OriginalVehicle)) ||
 array_overlap(out,w.base(0),w.game().base_count,sizeof(OriginalBase)) ||
 array_overlap(out,w.prototype(0),512,sizeof(OriginalPrototype)) ||
 array_overlap(out,w.faction(0),8,sizeof(OriginalFactionState)) ||
 array_overlap(out,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));
}
bool tech_id(int32_t id) { return id>=-2 && id<(int32_t)technology_count; }
}
LogicError world_citizen_energy(const OriginalWorld& w,const Rules& rules,
 const CitizenDefinitions& definitions,int32_t faction,WorldCitizenEnergy& out) {
 if (effects_range_overlap(&out,sizeof out,&w,sizeof w) ||
 effects_range_overlap(&out,sizeof out,&rules,sizeof rules) ||
 effects_range_overlap(&out,sizeof out,&definitions,sizeof definitions)) return LogicError::bad_state;
 if (!w.loaded() || faction<0 || faction>7 || rules.count!=technology_count ||
 definitions.count!=citizen_definition_count) return LogicError::bad_state;
 if (world_overlap(w,out)) return LogicError::bad_state;
 const auto edition=w.game().data_version<11 ? Variant::base : Variant::alien_crossfire;
 if (definitions.variant!=edition) return LogicError::bad_state;
 for (const auto& t:rules.technologies) if (!tech_id(t.prerequisites[0]) || !tech_id(t.prerequisites[1])) return LogicError::bad_state;
 for (size_t i=0;i<specialist_definition_count;++i)
  if (!tech_id(definitions.rows[i].prerequisite) || !tech_id(definitions.rows[i].obsolete)) return LogicError::bad_state;
 WorldCitizenEnergy next{};next.variant=edition;next.faction=faction;
 for (size_t i=0;i<specialist_definition_count;++i) {
  const auto& c=definitions.rows[i];auto& facts=next.citizens[i];bool known;
  auto error=world_has_technology(w,rules,c.prerequisite,faction,known);
  if (error!=LogicError::ok) return error;
  facts.prerequisite_available=known;
  error=world_has_technology(w,rules,c.obsolete,faction,known);
  if (error!=LogicError::ok) return error;
  facts.obsolete_available=known;
  facts.economy=c.economy;facts.psych=c.psych;facts.labs=c.labs;
 }
 out=next;return LogicError::ok;
}
}
