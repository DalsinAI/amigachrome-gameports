/* SPDX-License-Identifier: GPL-3.0-or-later. Policy attribution in base_psych.h. */
#include "base_psych.h"
#include <stddef.h>
namespace alpha {namespace {
int32_t clamp(int32_t n,int32_t lo,int32_t hi){return n<lo?lo:n>hi?hi:n;}
bool put(int64_t n,int32_t&out){if(n<(-2147483647LL-1)||n>2147483647LL)return false;out=(int32_t)n;return true;}
bool overlap(const void*a,size_t an,const void*b,size_t bn){const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<=y?y-x<an:x-y<bn;}
void adjust(const PsychFacts&f,PsychResult&r,int32_t value,bool force){
 const int32_t ordinary=f.population-f.specialists;
 while(force&&value>0){--value;if(r.talents<ordinary)++r.talents;if(r.talents+r.drones+r.superdrones>ordinary){if(r.superdrones>0)--r.superdrones;else if(r.drones>0)--r.drones;}}
 if(value>0)r.talents+=value;
 r.drones=clamp(r.drones,0,f.population);
 while(r.talents+r.drones+r.superdrones>ordinary){
  if(r.talents>0){--r.talents;if(r.superdrones>0)--r.superdrones;else if(r.drones>0)--r.drones;}
  else {if(r.drones<=ordinary){r.superdrones=clamp(r.superdrones,0,r.drones);break;}--r.drones;}
 }
}
void row(PsychResult&r,unsigned at){r.rows[at][0]=r.talents;r.rows[at][1]=r.drones;r.rows[at][2]=r.superdrones;}
}
PsychError base_psych_policy(const PsychFacts&f,PsychResult&output){
 if(overlap(&f,sizeof f,&output,sizeof output))return PsychError::bad_buffer;
 if(f.population<0||f.population>127||f.specialists<0||f.specialists>f.population||f.content_population<0||f.base_limit<0||f.base_index<0||f.base_index>=512||f.faction_base_count<1||f.faction_base_count>512||f.difficulty<0||f.difficulty>5||f.assimilation_turns<0||f.assimilation_turns>255||f.rule_drone<0||f.rule_talent<0||f.pending_talent< -100||f.pending_talent>100||f.pending_police< -100||f.pending_police>100||f.fresh_psych<0||f.genejack_drones<0||f.genejack_drones>100||f.pacifism_units<0)return PsychError::bad_state;
 for(unsigned i=0;i<3;++i)if(f.police_effects[i]<0||f.police_effects[i]>4||(i&&f.police_effects[i]>f.police_effects[i-1]))return PsychError::bad_state;
 PsychResult r{};r.can_riot=!f.nerve_stapled&&!f.punishment_sphere&&!f.telepathic_matrix;
 if(f.specialists==f.population){output=r;return PsychError::ok;}
 int32_t drones=f.population>f.content_population?f.population-f.content_population:0,talents=0;
 if(f.base_limit){int32_t numerator;if(!put((int64_t)(f.base_index%f.base_limit)+f.faction_base_count-f.base_limit,numerator))return PsychError::overflow;r.efficiency_drones=clamp(numerator/f.base_limit,0,f.population);}
 if(f.assimilation_turns>0){const int32_t a=(f.population+f.difficulty-2)/4,b=(f.assimilation_turns+9)/10;r.capture_drones=clamp(a<b?a:b,0,f.population);drones+=r.capture_drones;}
 if(f.rule_drone)drones+=f.population/f.rule_drone;
 if(f.rule_talent){int32_t n;if(!put((int64_t)f.rule_talent+f.population-1,n))return PsychError::overflow;talents=n/f.rule_talent;}
 if(!put((int64_t)drones-f.no_drone_bonus,drones))return PsychError::overflow;
 drones=drones<0?0:drones;
 if(f.pending_talent>=0)talents+=f.pending_talent;else if(!put((int64_t)drones-f.pending_talent,drones))return PsychError::overflow;
 r.talents=talents;r.drones=clamp(drones,0,f.population)+r.efficiency_drones;r.superdrones=clamp(r.drones-f.population,0,f.population);r.drones=clamp(r.drones,0,f.population);adjust(f,r,0,false);row(r,0);
 const int32_t psych=clamp(f.fresh_psych/2,0,f.population-r.talents);int32_t addon=f.fresh_psych/2-psych,cost=2;adjust(f,r,psych,false);
 const int32_t ordinary=f.population-f.specialists;
 while(addon>=cost&&r.talents<ordinary){addon-=cost;cost+=2;++r.talents;adjust(f,r,0,false);}
 row(r,1);
 int32_t facilities=(f.genejack?f.genejack_drones:0)-(f.recreation_commons?2:0)-(f.hologram_or_virtual?2:0)-(f.planetary_transit&&f.population<=3?1:0)-(f.research_hospital?1:0)-(f.nanohospital?1:0);
 r.drones+=facilities;if(f.paradise_garden)adjust(f,r,2,true);adjust(f,r,0,false);row(r,2);
 if(f.pending_police>=-1){const int32_t number=clamp((f.pending_police==-1)+f.pending_police+1,0,3);for(int32_t i=0;i<number;++i)r.police_total+=f.police_effects[i];r.drones=r.drones>r.police_total?r.drones-r.police_total:0;}
 int32_t extra=0;
 if(f.pending_police==-3&&f.pacifism_units>1)extra=f.pacifism_units-1;
 else if(f.pending_police==-4&&f.pacifism_units>0)extra=f.pacifism_units;
 else if(f.pending_police<=-5&&f.pacifism_units>0){if(!put((int64_t)f.pacifism_units*2,extra))return PsychError::overflow;}
 if(!put((int64_t)r.drones+extra,r.drones))return PsychError::overflow;
 if(extra)r.drones=r.drones>f.population?f.population:r.drones;
 adjust(f,r,0,false);row(r,3);
 if(f.nerve_stapled||f.punishment_sphere){r.talents=0;r.drones=0;adjust(f,r,0,false);}
 else {if(f.longevity_vaccine){if(r.drones>0&&f.pending_economics==2)--r.drones;if(r.drones>0&&f.pending_economics!=1)--r.drones;}adjust(f,r,(f.human_genome?1:0)+(f.clinical_immortality?1:0),true);}
 row(r,4);r.will_riot=r.can_riot&&r.drones>r.talents;output=r;return PsychError::ok;
}
}
