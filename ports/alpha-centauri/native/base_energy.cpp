/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "base_energy.h"
#include <initializer_list>
namespace alpha {namespace {
bool word(int64_t v){return v>=INT32_MIN&&v<=INT32_MAX;}
bool add(int32_t&v,int32_t n){const int64_t x=(int64_t)v+n;if(!word(x))return false;v=(int32_t)x;return true;}
bool muladd(int32_t&v,int32_t m,int32_t a,int32_t divisor){const int64_t product=(int64_t)v*m;if(!word(product)||!word(product+a))return false;v=(int32_t)(product+a)/divisor;return true;}
int32_t clamp(int32_t v,int32_t lo,int32_t hi){return v<lo?lo:v>hi?hi:v;}
bool overlap(const void*a,size_t an,const void*b,size_t bn){const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
bool bit(uint32_t v,unsigned i){return (v&(uint32_t(1)<<i))!=0;}
unsigned popcount(uint32_t v){unsigned n=0;while(v){v&=v-1;++n;}return n;}
PsychError convert(InefficiencyError e){switch(e){case InefficiencyError::ok:return PsychError::ok;case InefficiencyError::bad_state:return PsychError::bad_state;case InefficiencyError::bad_buffer:return PsychError::bad_buffer;case InefficiencyError::overflow:return PsychError::overflow;}return PsychError::bad_state;}
}
PsychError base_energy(const BaseEnergyFacts&f,BaseEnergyResult&output){
 if(overlap(&f,sizeof f,&output,sizeof output))return PsychError::bad_buffer;
 const auto&p=f.population;const auto&a=f.allocation;
 if(p.police_count>2048)return PsychError::bad_state;
 const size_t bytes=p.police_count*sizeof(PoliceUnitFacts);
 if((bytes&&!p.police_units)||(bytes&&(overlap(p.police_units,bytes,&f,sizeof f)||overlap(p.police_units,bytes,&output,sizeof output))))return PsychError::bad_buffer;
 if(p.expansion>1||p.human>1||p.upkeep_stage>1||p.has_nodrone>1||p.worm_police>1||p.population<0||p.population>127||p.specialists<0||p.specialists>p.population||p.assimilation_turns<0||p.assimilation_turns>255||p.difficulty<0||p.difficulty>5||p.map_factor<0||p.base_id<0||p.base_id>511||p.faction_base_count<0||p.faction_base_count>512||p.rule_drone<0||p.rule_talent<0||p.pacifism_count<0||p.nerve_staple_turns<0||p.nerve_staple_turns>255)return PsychError::bad_state;
 for(size_t i=0;i<p.police_count;++i){const auto&u=p.police_units[i];if(u.prototype_id<0||u.prototype_id>511||u.plan<0||u.plan>255||u.weapon_offense<-128||u.weapon_offense>127||u.triad<0||u.triad>2||u.double_police>1)return PsychError::bad_state;}
 if(f.energy_grid_project_owned>1||f.alien>1||f.expansion_enabled>1||f.sanctioned>1||f.qualified_foreign_commerce>1||f.partner_aliens>127||f.partner_sanctions>127||f.facility_bits64_to69>63||f.projects_built_here>64||f.network_node_count<0||f.network_node_count>512||(uint32_t)f.alien_grid_policy>1)return PsychError::bad_state;
 if(!p.expansion&&(f.alien||f.energy_grid_project_owned||f.partner_aliens))return PsychError::bad_state;
 if(a.pending_efficiency!=p.pending_efficiency||a.upkeep_stage!=p.upkeep_stage||a.creche!=(uint32_t)bit(p.facilities[0],2)||a.allocation_psych<0||a.allocation_psych>10||a.allocation_labs<0||a.allocation_labs>10||a.allocation_psych+a.allocation_labs>10)return PsychError::bad_state;
 for(unsigned i=0;i<7;++i)if(a.commerce[i]&&(f.alien||f.sanctioned||bit(f.partner_aliens,i)||bit(f.partner_sanctions,i)))return PsychError::bad_state;
 for(const auto&c:f.citizens)if(c.prerequisite_available>1||c.obsolete_available>1)return PsychError::bad_state;
 if(f.alien&&(f.expansion_enabled||bit(f.projects_here,21))&&!f.qualified_foreign_commerce&&f.alien_grid_policy==AlienGridPolicy::reject_uninitialized)return PsychError::bad_state;
 BaseEnergyResult next{};next.specialist_types[0]=f.specialist_types[0];next.specialist_types[1]=f.specialist_types[1];auto&v=next.accounting;v.intake=a.intake;v.consumption=a.convoy_from;v.faction_surplus=a.faction_surplus;
 if(!add(v.intake,a.convoy_to)||!word((int64_t)v.intake-v.consumption))return PsychError::overflow;
 InefficiencyFacts loss{};loss.energy=v.intake-v.consumption;loss.pending_efficiency=a.pending_efficiency;loss.hq_distance=a.hq_distance;loss.creche=a.creche;loss.upkeep_stage=a.upkeep_stage;for(unsigned i=0;i<9;++i)loss.counters[i]=a.counters[i];
 InefficiencyResult first{};auto error=energy_inefficiency(loss,first);if(error!=InefficiencyError::ok)return convert(error);
 const int64_t initial=(int64_t)loss.energy-first.loss;if(!word(initial))return PsychError::overflow;
 for(int32_t imp:a.commerce)if(!add(v.intake,imp))return PsychError::overflow;
 int32_t grid=0;
 if(f.alien&&f.expansion_enabled){grid=(int32_t)(popcount(p.facilities[0])+popcount(p.facilities[1])+popcount(f.facility_bits64_to69)+5*f.projects_built_here)/2;if(f.sanctioned)grid=0;if(!add(v.intake,grid))return PsychError::overflow;}
 if(a.upkeep_stage==1&&!add(v.faction_surplus,clamp((int32_t)initial,0,99999)))return PsychError::overflow;
 if(!word((int64_t)v.intake-v.consumption))return PsychError::overflow;
 loss.energy=v.intake-v.consumption;for(unsigned i=0;i<9;++i)loss.counters[i]=first.counters[i];InefficiencyResult second{};error=energy_inefficiency(loss,second);if(error!=InefficiencyError::ok)return convert(error);
 v.inefficiency=second.loss;if(!word((int64_t)loss.energy-second.loss))return PsychError::overflow;v.surplus=loss.energy-second.loss;for(unsigned i=0;i<9;++i)v.counters[i]=second.counters[i];
 const int32_t total=clamp(v.surplus,0,9999);v.psych=clamp((total*a.allocation_psych+4)/10,0,total);v.economy=clamp((total*(10-a.allocation_labs-a.allocation_psych)+4)/10,0,total-v.psych);v.labs=total-v.psych-v.economy;v.unknown_total=clamp((total*(9-a.allocation_labs-a.allocation_psych)+4)/10,0,total-v.psych);
 BasePsychFacts pop=p;
 int32_t best=0,best_score=-999;
 if(p.specialists>16)for(unsigned i=0;i<7;++i){const auto&c=f.citizens[i];if(c.prerequisite_available){int32_t score=c.psych;if(!muladd(score,3,0,1))return PsychError::overflow;if(p.population>=f.minimum_specialist_size&&(!add(score,c.economy)||!add(score,c.labs)))return PsychError::overflow;if(score>best_score){best=(int32_t)i;best_score=score;}}}
 for(int32_t i=0;i<p.specialists;++i){unsigned selected=(unsigned)best;if(i<16){selected=(next.specialist_types[(unsigned)i/8]>>(((unsigned)i%8)*4))&15;if(selected>6)selected=6;}
  const auto&old=f.citizens[selected];if(old.obsolete_available)for(unsigned j=0;j<7;++j){const auto&c=f.citizens[j];if(c.prerequisite_available&&!c.obsolete_available&&old.economy<=c.economy&&old.psych<=c.psych&&old.labs<=c.labs){selected=j;if(i<16){const unsigned shift=((unsigned)i%8)*4;next.specialist_types[(unsigned)i/8]=(next.specialist_types[(unsigned)i/8]&~(uint32_t(15)<<shift))|(j<<shift);}break;}}
  const auto&c=f.citizens[selected];if(!add(v.economy,c.economy)||!add(v.psych,c.psych)||!add(v.labs,c.labs))return PsychError::overflow;
 }
 if(!add(v.labs,f.rule_research)||(bit(p.facilities[0],9)&&!add(v.labs,2)))return PsychError::overflow;
 int32_t ce=(bit(p.facilities[0],7)||(p.expansion&&f.energy_grid_project_owned))?6:4,cl=bit(p.facilities[0],8)?6:4,cp=(bit(p.facilities[0],11)||(bit(p.facilities[0],8)&&bit(p.owned_projects,6)))?6:4;
 for(unsigned i:{17u,18u})if(bit(p.facilities[0],i)){++cp;cl+=2;}
 for(unsigned i:{13u,14u})if(bit(p.facilities[0],i)){cp+=2;ce+=2;}
 for(unsigned i:{15u,16u})if(bit(p.facilities[0],i)){ce+=2;cl+=2;}
 if(!muladd(v.psych,cp,3,4)||!muladd(v.labs,cl,3,4))return PsychError::overflow;
 for(unsigned i:{12u,18u})if(bit(f.projects_here,i)&&!muladd(v.labs,2,0,1))return PsychError::overflow;
 if(!muladd(v.economy,ce,3,4)||(bit(f.projects_here,27)&&!muladd(v.economy,2,0,1)))return PsychError::overflow;
 if(bit(f.projects_here,14)&&p.pending_economics==1&&!add(v.economy,v.economy/2))return PsychError::overflow;
 if(!muladd(v.unknown_total,ce,3,4))return PsychError::overflow;
 if(bit(f.projects_here,21)){if(!add(v.labs,f.network_node_count))return PsychError::overflow;if(f.alien){if(!add(v.labs,grid))return PsychError::overflow;}else if(!f.sanctioned)for(unsigned i=0;i<7;++i)if(!bit(f.partner_sanctions,i)&&!bit(f.partner_aliens,i)&&!add(v.labs,a.commerce[i]))return PsychError::overflow;}
 int32_t balance=a.allocation_psych-10+2*a.allocation_labs;if(balance<0)balance=-balance;balance/=2;const int32_t penalty=balance*(4-clamp(a.pending_efficiency,-4,4))*2;
 if(penalty){const bool heavier=10-a.allocation_labs-a.allocation_psych<a.allocation_labs;const int32_t lp=clamp(penalty*(heavier?1:2),0,heavier?40:80),ep=clamp(penalty*(heavier?2:1),0,heavier?80:40);if(!muladd(v.labs,100-lp,50,100)||!muladd(v.economy,100-ep,50,100))return PsychError::overflow;}
 const int32_t repeat=p.expansion?-69:-38;
 if(f.current_item==repeat&&f.previous_item==repeat&&f.mineral_surplus>0){int32_t stock=f.mineral_surplus;if(!add(stock,1))return PsychError::overflow;stock/=2;if(p.expansion&&f.energy_grid_project_owned&&!muladd(stock,5,3,4))return PsychError::overflow;if(!add(v.economy,stock))return PsychError::overflow;}
 pop.psych=v.psych;const PsychError pe=base_psych(pop,next.population);if(pe!=PsychError::ok)return pe;output=next;return PsychError::ok;
}
}
