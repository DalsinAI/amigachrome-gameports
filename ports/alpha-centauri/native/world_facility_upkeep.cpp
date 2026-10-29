/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_facility_upkeep.h"
#include "world_unit_stats.h"
#include <string.h>
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){if(!a||!b||!an||!bn)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
bool aliases(const OriginalWorld&w,const Rules&t,const UnitDefinitions&u,const FacilityDefinitions&d,const FacilityUpkeepStage&s,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&t,sizeof t)||overlap(out,n,&u,sizeof u)||overlap(out,n,&d,sizeof d)||overlap(out,n,&s,sizeof s))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();return overlap(out,n,w.original_bytes(),w.original_size())||overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
}
EnergyError world_facility_upkeep(const OriginalWorld&w,const Rules&t,const UnitDefinitions&u,const FacilityDefinitions&d,size_t id,const FacilityUpkeepStage&s,FacilityUpkeepResult&output){
 if(aliases(w,t,u,d,s,&output,sizeof output))return EnergyError::bad_buffer;
 if(!w.loaded()||t.count!=89)return EnergyError::bad_state;
 const bool base=w.game().data_version<11;const size_t first=base?39:70,slots=base?72:134;
 if(u.weapon_count!=(base?21u:26u))return EnergyError::bad_state;
 if(d.slot_count!=slots||d.first_project!=first||d.first_repeatable!=(base?34u:65u)||d.project_count!=slots-first)return EnergyError::bad_state;
 const auto*b=w.base(id);if(!b||b->faction_id<1||b->faction_id>7)return EnergyError::bad_state;const auto*f=w.faction((size_t)b->faction_id);const auto*def=w.faction_definition((size_t)b->faction_id);if(!f||!def||def->faction_bonus_count<0||def->faction_bonus_count>8)return EnergyError::bad_state;
 FacilityUpkeepFacts facts{};facts.first_project=(int32_t)first;facts.difficulty=f->diff_level;facts.best_reactor=1;facts.credits_before=s.credits_before;facts.maintenance_before=s.maintenance_before;facts.mineral_cost_factor=s.mineral_cost_factor;facts.player_flags_before=s.player_flags_before;facts.queue_item=b->queue_items[0];memcpy(facts.built,b->facilities_built,sizeof facts.built);
 for(int i=3;i>=0;--i){bool known;if(world_has_technology(w,t,u.reactors[i].prerequisite,b->faction_id,known)!=LogicError::ok)return EnergyError::bad_state;if(known){facts.best_reactor=i+1;break;}}
 const int32_t project=w.game().preferences.SecretProjects[25];if(project< -2)return EnergyError::bad_state;if(project>=0){const auto*p=w.base((size_t)project);if(!p)return EnergyError::bad_state;facts.self_aware_colony=p->faction_id==b->faction_id;}
 for(size_t i=1;i<first;++i){facts.cost[i]=d.slots[i].cost;facts.maintenance[i]=d.slots[i].maintenance;
  for(int j=0;j<def->faction_bonus_count;++j)if(def->faction_bonus_val1[j]==(int32_t)i){const int bonus=def->faction_bonus_id[j];bool free=bonus==2;
   if(bonus==12){bool known;if(world_has_technology(w,t,d.slots[i].prerequisite,b->faction_id,known)!=LogicError::ok)return EnergyError::bad_state;free=known;}
   if(free)facts.free_maintenance[i/8]|=(uint8_t)(1u<<(i%8));
  }
 }
 return facility_upkeep(facts,output);
}
}
