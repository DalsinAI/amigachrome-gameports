/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in upkeep_base_minerals.h. */
#include "upkeep_base_minerals.h"
namespace alpha {namespace {
bool put(int64_t n,int32_t& out){if(n<(-2147483647LL-1)||n>2147483647LL)return false;out=(int32_t)n;return true;}
int32_t clamp(int32_t n,int32_t lo,int32_t hi){return n<lo?lo:n>hi?hi:n;}
bool overlap(const void*a,unsigned an,const void*b,unsigned bn){const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<=y?y-x<an:x-y<bn;}
}
MineralError base_minerals(const MineralFacts&f,UpkeepMineralResult&output){
 if(overlap(&f,sizeof f,&output,sizeof output))return MineralError::bad_buffer;
 if(f.fresh_intake<0||f.convoy_to<0||f.forces_maintenance<0||f.convoy_from<0||
    f.population<0||f.population>127||f.major_atrocities<0||f.tectonic_detonations<0||
    f.faction_difficulty<0||f.faction_difficulty>5||f.game_difficulty<0||f.game_difficulty>5||
    f.native_life<0||f.native_life>2||f.tech_ranking<0||f.transcendent_techs<0||
    f.transcendent_techs>f.tech_ranking)return MineralError::bad_state;
 UpkeepMineralResult r{};int32_t value,clean,scaled;
 const int32_t bonus=(f.quantum_converter||f.singularity_inductor)+f.robotic_assembly+
  f.genejack+f.nanoreplicator+f.bulk_matter;
 if(!put((int64_t)f.fresh_intake+f.convoy_to,value)||!put((int64_t)value*(bonus+2),scaled))return MineralError::overflow;
 r.intake=scaled/2;
 if(!put((int64_t)f.forces_maintenance+f.convoy_from,r.consumption)||
    !put((int64_t)r.intake-r.consumption,r.surplus))return MineralError::overflow;
 r.surplus_final=r.surplus;r.eco_damage=f.raw_eco_damage/8;
 if(!put((int64_t)f.planet_ecology+16,clean))return MineralError::overflow;
 if(r.eco_damage>0){value=r.eco_damage<clean?r.eco_damage:clean;
  if(!put((int64_t)clean-value,clean)||!put((int64_t)r.eco_damage-value,r.eco_damage))return MineralError::overflow;
 }
 const int32_t reduction=((f.nanoreplicator||f.singularity_inductor)?2:1)+f.centauri_preserve+f.temple+f.pholus;
 if(!put((int64_t)r.intake-clean,value)||!put((int64_t)value-clamp(f.satellites,0,f.population),value)||
    !put((int64_t)r.eco_damage+value/reduction,r.eco_damage))return MineralError::overflow;
 if(f.human){
  if(!put((int64_t)f.major_atrocities+f.tectonic_detonations,value)||!put((int64_t)value*5,value)||
     !put((int64_t)r.eco_damage+value/(clamp(f.sea_level,0,100)/clamp(f.sea_level_rises,1,100)+reduction),r.eco_damage))return MineralError::overflow;
 }
 if(r.eco_damage<0)r.eco_damage=0;
 if(f.ascending_transcendence&&!put((int64_t)r.eco_damage*2,r.eco_damage))return MineralError::overflow;
 if(f.perihelion&&!put((int64_t)r.eco_damage*2,r.eco_damage))return MineralError::overflow;
 const int32_t difficulty=f.human?(f.faction_difficulty==0?2:f.faction_difficulty<=3?3:5):clamp(6-f.game_difficulty,1,3);
 value=f.tech_ranking-f.transcendent_techs;
 if(!put((int64_t)value*(3-clamp(f.planet_pending,-3,2)),value)||
    !put((int64_t)value*(f.native_life+1),value)||!put((int64_t)value*r.eco_damage,value)||
    !put((int64_t)value*difficulty,value))return MineralError::overflow;
 value/=6;if(!put((int64_t)value+50,value))return MineralError::overflow;r.eco_damage=value/100;
 if(f.space_elevator_orbital){if(!put((int64_t)r.intake*2,r.intake)||!put((int64_t)r.intake-r.consumption,r.surplus))return MineralError::overflow;}
 output=r;return MineralError::ok;
}
}
