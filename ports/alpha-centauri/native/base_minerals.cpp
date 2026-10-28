/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in base_minerals.h. */
#include "base_minerals.h"
namespace alpha { namespace {
int32_t clamp(int32_t n,int32_t lo,int32_t hi){return n<lo?lo:n>hi?hi:n;}
bool overlap(const void*a,size_t an,const void*b,size_t bn){const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
bool checked(int64_t n,int32_t& out){if(n<(-2147483647LL-1) || n>2147483647LL)return false;out=(int32_t)n;return true;}
}
LogicError calculate_minerals(const MineralInput& in,MineralResult& output){
 if(overlap(&in,sizeof in,&output,sizeof output))return LogicError::bad_state;
 if(in.intake<0 || in.convoy_to<0 || in.convoy_from<0 || in.maintenance<0 ||
    in.population<0 || in.population>127 || in.clean_minerals_modifier<0 || in.atrocities<0 ||
    in.tectonic_detonations<0 || (!in.expansion && in.tectonic_detonations!=0) || in.tech_ranking<0 || in.transcendent_techs<0 ||
    in.transcendent_techs>in.tech_ranking || in.native_life<0 || in.native_life>2 ||
    (in.human && (in.faction_difficulty<0 || in.faction_difficulty>5)) ||
    (!in.human && (in.global_difficulty<0 || in.global_difficulty>5)))return LogicError::bad_state;
 MineralResult next{};
 next.bonus=(in.quantum_converter || in.singularity_inductor)+in.robotic_assembly+in.genejack+in.nanoreplicator+in.bulk_matter;
 int32_t raw;
 if(!checked((int64_t)in.intake+in.convoy_to,raw))return LogicError::overflow;
 // The original instructions use signed multiplication and division here;
 // the recovered source's unsigned temporary changes overflow behavior.
 if(!checked((int64_t)raw*(next.bonus+2),next.intake))return LogicError::overflow;
 next.intake/=2;
 if(!checked((int64_t)in.maintenance+in.convoy_from,next.consumption) ||
    !checked((int64_t)next.intake-next.consumption,next.surplus))return LogicError::overflow;
 next.final_surplus=next.surplus;
 int32_t clean;
 if(!checked((int64_t)in.clean_minerals_modifier+16,clean))return LogicError::overflow;
 // Original SMAC4ff345/SMAX4e9e19 use arithmetic shift-right by3.
 // Express floor division portably without negating INT32_MIN.
 int32_t damage=in.raw_eco_damage>=0 ? in.raw_eco_damage/8 : -1-((-1-in.raw_eco_damage)/8);
 if(damage>0){const int32_t removed=damage<clean?damage:clean;damage-=removed;clean-=removed;}
 const int32_t reduction=(in.nanoreplicator || in.singularity_inductor?2:1)+in.centauri_preserve+in.temple+in.pholus_mutagen;
 int32_t pollution;
 if(!checked((int64_t)next.intake-clean,pollution) ||
    !checked((int64_t)pollution-clamp(in.satellites,0,in.population),pollution) ||
    !checked((int64_t)damage+pollution/reduction,damage))return LogicError::overflow;
 if(in.human){int32_t violations;
  const int32_t denominator=clamp(in.sea_level,0,100)/clamp(in.sea_level_rises,1,100)+reduction;
  if(!checked((int64_t)in.atrocities*5,violations))return LogicError::overflow;
  if(!checked((int64_t)damage+violations/denominator,damage))return LogicError::overflow;
  // Original code rounds the two contributions separately, unlike the
  // recovered combined expression in OpenSMACX/Thinker.
  if(!checked((int64_t)in.tectonic_detonations*5,violations) ||
     !checked((int64_t)damage+violations/denominator,damage))return LogicError::overflow;
 }
 if(damage<0)damage=0;
 if(in.ascending && in.transcendence_enabled && !checked((int64_t)damage*2,damage))return LogicError::overflow;
 if(in.perihelion && !checked((int64_t)damage*2,damage))return LogicError::overflow;
 const int32_t difficulty=in.human?(in.faction_difficulty==0?2:in.faction_difficulty<=3?3:5):clamp(6-in.global_difficulty,1,3);
 int32_t effective=in.tech_ranking-in.transcendent_techs;
 if(!checked((int64_t)effective*(3-clamp(in.planet,-3,2)),effective))return LogicError::overflow;
 int32_t weighted;
 if(!checked((int64_t)effective*(in.native_life+1),weighted) ||
    !checked((int64_t)weighted*damage,weighted) ||
    !checked((int64_t)weighted*difficulty,weighted))return LogicError::overflow;
 next.eco_damage=(int32_t)((weighted/6+50)/100);
 const int32_t satellite_first=in.expansion?65:34;
 if(in.space_elevator && in.queue_id>=-(satellite_first+3) && in.queue_id<=-satellite_first){
  if(!checked((int64_t)next.intake*2,next.intake) ||
     !checked((int64_t)next.intake-next.consumption,next.surplus))return LogicError::overflow;
 }
 output=next;return LogicError::ok;
}
}
