/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_unit_availability.h"
#include "technology_effects_ranges.h"
#include "movement.h"
namespace alpha { namespace {
bool aliases(const OriginalWorld& w,const Rules& t,const UnitDefinitions& d,const void* out,size_t n){
 using detail::effects_range_overlap;using detail::effects_array_overlap;
 if(effects_range_overlap(out,n,&w,sizeof w) || effects_range_overlap(out,n,&t,sizeof t) || effects_range_overlap(out,n,&d,sizeof d))return true;
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
LogicError coast(const OriginalWorld& w,const OriginalBase& b,bool& output){
 const auto& m=w.game().map.info;MapGeometry g{(int32_t)m.width,(int32_t)m.height,m.flat};
 const int32_t dx[]={1,2,1,0,-1,-2,-1,0},dy[]={-1,0,1,2,1,0,-1,-2};bool value=false;
 for(unsigned j=0;j<8;++j){
  int32_t x;if(movement_wrap_x(g,b.x+dx[j],x)!=LogicError::ok)return LogicError::bad_state;
  const int32_t y=b.y+dy[j];if(x<0 || x>=g.width || y<0 || y>=g.height)continue;
  size_t index;if(movement_tile_index(g,x,y,index)!=LogicError::ok)return LogicError::bad_state;
  const auto* tile=w.tile(index);if(!tile)return LogicError::bad_state;
  if((tile->climate&0xe0u)<0x60u)value=true;
 }
 output=value;return LogicError::ok;
}
}
LogicError world_unit_availability(const OriginalWorld& w,const Rules& t,const UnitDefinitions& d,
 Variant profile,bool expansion_enabled,int32_t faction,int32_t base,int32_t unit,bool& output){
 if(aliases(w,t,d,&output,sizeof output) || !w.loaded() || t.count!=89 ||
 (profile!=Variant::base && profile!=Variant::alien_crossfire) ||
 ((w.game().data_version>=11)!=(profile==Variant::alien_crossfire)) ||
 d.weapon_count!=(profile==Variant::base?21u:26u) ||
 d.armor_count!=(profile==Variant::base?10u:14u) ||
 d.ability_count!=(profile==Variant::base?24u:29u) ||
 faction<0 || faction>=8 || unit<0 || unit>=512 || base< -1)return LogicError::bad_state;
 const auto* p=w.prototype((size_t)unit);if(!p)return LogicError::bad_state;
 UnitAvailabilityInput in{};in.variant=profile;in.unit=unit;in.faction=faction;
 in.flags=p->unit_flags;in.obsolete=p->obsolete_factions;in.plan=p->plan;
 in.armor=p->armor_id;in.weapon=p->weapon_id;in.abilities=p->ability_flags;
 in.game_rules=(uint32_t)w.game().preferences.GameRules;in.expansion_enabled=expansion_enabled;
 in.technology=p->preq_tech;
 if(in.technology>=0 && in.technology<88){
  const auto& tech=t.technologies[(size_t)in.technology];in.prerequisite1=tech.prerequisites[0];in.prerequisite2=tech.prerequisites[1];
  in.technology_owners=w.game().preferences.TechOwners[(size_t)in.technology];
 }
 if(base>=0){
  const auto* b=w.base((size_t)base);if(!b || p->chassis_id>=9)return LogicError::bad_state;
  in.base_present=true;in.triad=(uint8_t)d.chassis[p->chassis_id].triad;
  if(in.triad==1 && coast(w,*b,in.coastal)!=LogicError::ok)return LogicError::bad_state;
 }
 return unit_availability(in,output);
}
}
