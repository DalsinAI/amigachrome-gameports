/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in research_options.h. */
#include "research_options.h"
namespace alpha {
LogicError world_research_options(const OriginalWorld& world,const Rules& tech,Variant variant,int32_t faction,ResearchOptions& output){
 if(!world.loaded() || tech.count!=technology_count || faction<0 || faction>=8 ||
    (variant!=Variant::base && variant!=Variant::alien_crossfire))return LogicError::bad_state;
 for(size_t i=0;i<technology_count;++i)for(unsigned k=0;k<2;++k){
  int32_t p=tech.technologies[i].prerequisites[k];if(p<-2 || p>=(int32_t)technology_count)return LogicError::bad_state;
 }
 const auto* saved=world.faction((size_t)faction);if(!saved)return LogicError::bad_state;
 ResearchOptions next{};next.current_id=saved->tech_research_id;next.accumulated=saved->tech_accumulated;
 next.cost=saved->tech_cost;next.transcendent_count=saved->tech_count_transcendent;
 next.current_id_supported=next.current_id==-1 || (next.current_id>=0 && next.current_id<(int32_t)technology_count);
 for(size_t i=0;i<technology_count;++i){
  auto& item=next.technologies[i];const auto& t=tech.technologies[i];item.repeatable=i==88;
  LogicError error=world_has_technology(world,tech,(int32_t)i,faction,item.known);if(error!=LogicError::ok)return error;
  next.known_count+=item.known;
  if(item.known)item.availability=ResearchAvailability::already_known;
  else if(variant==Variant::base && i>=78 && i<=86)item.availability=ResearchAvailability::base_mode_excluded;
  else if(t.prerequisites[0]==-2 || t.prerequisites[1]==-2)item.availability=ResearchAvailability::disabled;
  else if(!faction)item.availability=ResearchAvailability::planet;
  else{
   for(unsigned k=0;k<2;++k){bool present=false;error=world_has_technology(world,tech,t.prerequisites[k],faction,present);if(error!=LogicError::ok)return error;if(!present)item.missing_prerequisites|=(uint8_t)(1u<<k);}
   item.availability=item.missing_prerequisites?ResearchAvailability::missing_prerequisite:ResearchAvailability::available;
  }
  next.available_count+=item.availability==ResearchAvailability::available;
 }
 output=next;return LogicError::ok;
}
}
