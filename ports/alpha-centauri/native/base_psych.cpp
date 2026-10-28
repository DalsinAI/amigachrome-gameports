/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "base_psych.h"
namespace alpha { namespace {
bool fits(int64_t x){return x>=INT32_MIN&&x<=INT32_MAX;}
bool add(int32_t&x,int32_t y){const int64_t v=(int64_t)x+y;if(!fits(v))return false;x=(int32_t)v;return true;}
int32_t clamp(int32_t x,int32_t lo,int32_t hi){return x<lo?lo:x>hi?hi:x;}
bool overlap(const void*a,size_t an,const void*b,size_t bn){const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
bool flag(uint32_t x){return x<=1;}
struct Row {int32_t t,d,s;};
bool adjust(Row&r,int32_t pop,int32_t specialists,bool limit=true){
 if(limit){r.t=clamp(r.t,0,pop);r.d=clamp(r.d,0,pop);}
 const int32_t citizens=pop-specialists;
 if(!fits((int64_t)r.t+r.d)||!fits((int64_t)r.t+r.d+r.s))return false;
 // Equivalent bulk steps avoid a huge loop for an explicit large talent rule.
 for(unsigned phase=0;phase<2;++phase){int32_t&other=phase==0?r.s:r.d;const int32_t excess=r.t+r.d+r.s-citizens;if(excess>0&&r.t>0&&other>0){int32_t k=r.t<other?r.t:other;const int32_t needed=excess/2+excess%2;if(k>needed)k=needed;r.t-=k;other-=k;}}
 int32_t excess=r.t+r.d+r.s-citizens;
 if(excess>0&&r.t>0){const int32_t k=r.t<excess?r.t:excess;r.t-=k;}
 if(r.t+r.d+r.s>citizens&&r.t==0){if(r.d>citizens)r.d=citizens;r.s=clamp(r.s,0,r.d);}
 return true;
}
bool facility(const BasePsychFacts&f,unsigned id){return (f.facilities[id/32]&(uint32_t(1)<<(id%32)))!=0;}
bool project(const BasePsychFacts&f,unsigned id){return (f.owned_projects&(uint32_t(1)<<id))!=0;}
struct Context {int32_t drones,talents,bureaucracy,facility_drones;};
bool phases(const BasePsychFacts&f,const Context&c,int32_t talent,int32_t police,int32_t rows[5][3]){
 Row r{c.talents,c.drones,0};
 if(talent<0){if(!fits((int64_t)r.d-talent))return false;r.d=(int32_t)((int64_t)r.d-talent);}else if(!add(r.t,talent))return false;
 r.d=clamp(r.d,0,f.population);if(!add(r.d,c.bureaucracy))return false;
 r.s=clamp(r.d-f.population,0,f.population);r.d=clamp(r.d,0,f.population);
 for(unsigned phase=0;phase<5;++phase){
  if(phase==1){if(!add(r.t,f.psych/2))return false;}
  if(phase==2){if(!add(r.d,c.facility_drones)||!add(r.t,facility(f,12)?2:0))return false;}
  if(phase==3){
   if(!fits((int64_t)police+2))return false;
   int32_t quota=clamp(police+2,0,4);if(quota>=2)--quota;int32_t used=0;
   if(quota>0&&project(f,25)){r.d=clamp(r.d-1-(police>2),0,f.population);used=1;}
   for(unsigned pass=0;pass<2&&used<quota;++pass)for(size_t i=0;i<f.police_count&&used<quota;++i){const auto&u=f.police_units[i];const bool worm=f.expansion&&f.worm_police&&u.prototype_id<64&&(u.weapon_offense<0||u.prototype_id==15)&&u.triad==0;const bool priority=u.double_police||worm;if(u.plan<4&&priority==(pass==0)){r.d=clamp(r.d-1-(int32_t)u.double_police-(police>2)-(int32_t)worm,0,f.population);++used;}}
   int64_t extra=0;if(police==-4)extra=f.pacifism_count;else if(police==-3&&f.pacifism_count>1)extra=(int64_t)f.pacifism_count-1;else if(police<-4)extra=(int64_t)f.pacifism_count*2;
   if(!fits(extra)||!add(r.d,(int32_t)extra))return false;
  }
  if(phase==4){if(!add(r.t,(project(f,0)?1:0)+(project(f,26)?2:0)))return false;if(project(f,14)&&!add(r.d,-(int32_t)(f.pending_economics==2)-(int32_t)(f.pending_economics!=1)))return false;if(facility(f,23)||f.nerve_staple_turns){r.t=0;r.d=0;}}
  if(!adjust(r,f.population,f.specialists,phase!=0))return false;
  rows[phase][0]=r.t;rows[phase][1]=r.d;rows[phase][2]=r.s;
 }
 return true;
}
}
PsychError base_psych(const BasePsychFacts&f,BasePsychResult&output){
 if(overlap(&f,sizeof f,&output,sizeof output))return PsychError::bad_buffer;
 if(f.police_count>2048)return PsychError::bad_state;
 const size_t units_bytes=f.police_count*sizeof(PoliceUnitFacts);
 if((f.police_count&&!f.police_units)|| (units_bytes&&(overlap(f.police_units,units_bytes,&output,sizeof output)||overlap(f.police_units,units_bytes,&f,sizeof f))))return PsychError::bad_buffer;
 if(!flag(f.expansion)||!flag(f.human)||!flag(f.upkeep_stage)||!flag(f.has_nodrone)||!flag(f.worm_police)||f.population<0||f.population>127||f.specialists<0||f.specialists>f.population||f.assimilation_turns<0||f.assimilation_turns>255||f.difficulty<0||f.difficulty>5||f.map_factor<0||f.base_id<0||f.base_id>511||f.faction_base_count<0||f.faction_base_count>512||f.rule_drone<0||f.rule_talent<0||f.pacifism_count<0||f.nerve_staple_turns<0||f.nerve_staple_turns>255)return PsychError::bad_state;
 for(size_t i=0;i<f.police_count;++i){const auto&u=f.police_units[i];if(u.prototype_id<0||u.prototype_id>511||u.plan<0||u.plan>255||u.weapon_offense<-128||u.weapon_offense>127||u.triad<0||u.triad>2||!flag(u.double_police))return PsychError::bad_state;}
 BasePsychResult next{};for(unsigned i=0;i<82;++i)next.social_psych[i]=f.social_psych[i];Context c{};
 const int32_t difficulty=f.human?f.difficulty:3;int64_t limit=(int64_t)(f.pending_efficiency>0?f.pending_efficiency:0)+4;if(!fits(limit))return PsychError::overflow;limit*=8-difficulty;if(!fits(limit))return PsychError::overflow;limit*=f.map_factor;if(!fits(limit))return PsychError::overflow;limit/=112;
 if(limit)c.bureaucracy=clamp((f.base_id%(int32_t)limit+f.faction_base_count-(int32_t)limit)/(int32_t)limit,0,f.population);
 c.drones=clamp(f.population-6+difficulty,0,f.population);const int32_t capture=clamp((f.population-2+difficulty)/4,0,(f.assimilation_turns+9)/10);c.drones+=capture;
 if(f.rule_drone)c.drones+=f.population/f.rule_drone;
 if(f.rule_talent){const int64_t top=(int64_t)f.rule_talent-1+f.population;if(!fits(top))return PsychError::overflow;c.talents=(int32_t)top/f.rule_talent;}
 if(f.expansion&&f.has_nodrone){const int64_t x=(int64_t)c.drones-f.nodrone;if(!fits(x))return PsychError::overflow;c.drones=x<0?0:(int32_t)x;}
 c.facility_drones=facility(f,22)?f.genejack_drones:0;
 if(facility(f,6)&&!add(c.facility_drones,-2))return PsychError::overflow;
 if(((facility(f,8)&&project(f,6))||facility(f,11))&&!add(c.facility_drones,-2))return PsychError::overflow;
 if(project(f,7)&&f.population<4&&!add(c.facility_drones,-1))return PsychError::overflow;
 if(facility(f,17)&&!add(c.facility_drones,-1))return PsychError::overflow;
 if(facility(f,18)&&!add(c.facility_drones,-1))return PsychError::overflow;
 if(f.upkeep_stage)for(int ti=8;ti>=1;--ti)for(int pi=9;pi>=1;--pi){int32_t rows[5][3];if(!phases(f,c,ti-4,pi-6,rows))return PsychError::overflow;int32_t score=rows[4][0]-rows[4][1];if(score>0&&(rows[4][1]||rows[4][0]*2<f.population))score/=2;if(score<0){score-=f.population;if(f.population<6)score*=2;}if(!add(next.social_psych[ti*9+pi],score))return PsychError::overflow;}
 int32_t police=f.pending_police;if(f.expansion&&facility(f,35)&&!add(police,2))return PsychError::overflow;
 if(!phases(f,c,f.pending_talent,police,next.rows))return PsychError::overflow;
 next.talents=next.rows[4][0];next.drones=next.rows[4][1];next.superdrones=next.rows[4][2];output=next;return PsychError::ok;
}
}
