/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in upkeep_energy_allocation.h. */
#include "upkeep_energy_allocation.h"
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){if(!a||!b||!an||!bn)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
bool put(int64_t value,int32_t&out){if(value<(-2147483647LL-1)||value>2147483647LL)return false;out=(int32_t)value;return true;}
bool add(int32_t&out,int32_t value){return put((int64_t)out+value,out);}
int32_t clamp(int32_t x,int32_t lo,int32_t hi){return x<lo?lo:x>hi?hi:x;}
bool equal(const char*a,const char*b){for(;;++a,++b){const unsigned char x=*a>='A'&&*a<='Z'?(unsigned char)(*a+32):(unsigned char)*a,y=*b>='A'&&*b<='Z'?(unsigned char)(*b+32):(unsigned char)*b;if(x!=y)return false;if(!x)return true;}}
int technology(const Rules&r,const char*key){if(equal(key,"None"))return -1;if(equal(key,"Disable"))return -2;for(unsigned i=0;i<89;++i)if(equal(r.technologies[i].key,key))return (int)i;return -3;}
bool number(const char*p,int32_t&out){bool negative=false;if(*p=='-'||*p=='+')negative=*p++=='-';if(!*p)return false;uint32_t n=0,limit=negative?0x80000000u:0x7fffffffu;for(;*p;++p){if(*p<'0'||*p>'9')return false;unsigned d=(unsigned)(*p-'0');if(n>(limit-d)/10)return false;n=n*10+d;}out=negative?(n==0x80000000u?(-2147483647-1):-(int32_t)n):(int32_t)n;return true;}
bool line(Text&t){while(t.next())if(t.line()[0]&&t.line()[0]!=';')return true;return false;}
}
Error read_citizens(const uint8_t*bytes,size_t size,const Rules&r,UpkeepCitizenDefinitions&out){
 if(overlap(&out,sizeof out,bytes,size)||overlap(&out,sizeof out,&r,sizeof r))return Error::bad_row;
 if(r.count!=89)return Error::wrong_count;
 if(!bytes||!size)return Error::missing_section;
 Text t(bytes,size);if(!t.section("CITIZENS"))return t.error();UpkeepCitizenDefinitions next{};next.count=7;
 for(unsigned i=0;i<10;++i){if(!line(t)||t.line()[0]=='#')return t.error()==Error::ok?Error::wrong_count:t.error();char token[128];
  for(unsigned j=0;j<2;++j)if(!t.item(token,sizeof token)||!*token)return Error::bad_row;
  if(i<7){auto&c=next.specialists[i];int32_t*values[]={&c.prerequisite,&c.obsolete,&c.economy,&c.psych,&c.labs};
   for(unsigned j=0;j<5;++j){if(!t.item(token,sizeof token)||!*token)return Error::bad_row;if(j<2){*values[j]=technology(r,token);if(*values[j]==-3)return Error::unknown_prerequisite;}else if(!number(token,*values[j]))return Error::bad_number;}
  }
  // Source reads only the fields above; trailing sprite/annotation columns
  // (present in the genuine demo CITIZENS table) are intentionally ignored.
 }
 if(line(t)&&t.line()[0]!='#')return Error::wrong_count;
 if(t.error()!=Error::ok)return t.error();
 out=next;return Error::ok;
}
EnergyError allocate_energy(const AllocationFacts&f,AllocationResult&out){
 if(overlap(&f,sizeof f,&out,sizeof out))return EnergyError::bad_buffer;
 if(f.alloc_labs<0||f.alloc_labs>10||f.alloc_psych<0||f.alloc_psych>10-f.alloc_labs||f.population<0||f.population>127||f.specialist_count<0||f.specialist_count>f.population||f.min_specialist_population<0||f.min_specialist_population>100||f.citizens.count!=7||f.global_network_nodes<0||f.global_network_nodes>512||f.commerce<0)return EnergyError::bad_state;
 for(unsigned i=0;i<7;++i){const auto&c=f.citizens.specialists[i];if(c.prerequisite< -2||c.prerequisite>=89||c.obsolete< -2||c.obsolete>=89||c.economy<0||c.psych<0||c.labs<0||f.prerequisite_known[i]>1||f.obsolete_known[i]>1)return EnergyError::bad_state;}
 AllocationResult r{};r.distributed_energy=clamp(f.fresh_surplus,0,9999);r.specialist_types[0]=f.specialist_types[0];r.specialist_types[1]=f.specialist_types[1];
 r.psych=clamp((r.distributed_energy*f.alloc_psych+4)/10,0,r.distributed_energy);
 r.economy=clamp((r.distributed_energy*(10-f.alloc_labs-f.alloc_psych)+4)/10,0,r.distributed_energy-r.psych);
 r.unknown_total=clamp((r.distributed_energy*(9-f.alloc_labs-f.alloc_psych)+4)/10,0,r.distributed_energy-r.psych);
 r.labs=r.distributed_energy-r.psych-r.economy;
 int best=0;int64_t score=(-2147483647LL-1);
 for(unsigned i=0;f.specialist_count>16&&i<7;++i){const auto&c=f.citizens.specialists[i];if(f.prerequisite_known[i]&&!f.obsolete_known[i]&&(c.psych>=2||f.population>=f.min_specialist_population)){
  int32_t value;if(!put((int64_t)c.economy+c.labs+2LL*c.psych,value))return EnergyError::overflow;if(value>score){score=value;best=(int)i;}
 }}
 for(int i=0;i<f.specialist_count;++i){int citizen=i<16?(int)((f.specialist_types[i/8]>>(4*(i&7)))&15u):best;citizen=clamp(citizen,0,6);
  const auto&old=f.citizens.specialists[citizen];if(f.obsolete_known[citizen])for(unsigned j=0;j<7;++j){const auto&c=f.citizens.specialists[j];if(f.prerequisite_known[j]&&!f.obsolete_known[j]&&c.economy>=old.economy&&c.psych>=old.psych&&c.labs>=old.labs){citizen=(int)j;break;}}
  if(i<16){const unsigned shift=4u*(unsigned)(i&7);r.specialist_types[i/8]=(r.specialist_types[i/8]&~(15u<<shift))|((uint32_t)citizen<<shift);}
  const auto&c=f.citizens.specialists[citizen];
  if(!add(r.economy,c.economy)||!add(r.psych,c.psych)||!add(r.labs,c.labs)||!add(r.specialist_economy,c.economy)||!add(r.specialist_psych,c.psych)||!add(r.specialist_labs,c.labs))return EnergyError::overflow;
 }
 if(f.biology_lab&&!add(r.labs,2))return EnergyError::overflow;
 r.coefficient_economy=4+2*(f.energy_bank+f.tree_farm+f.hybrid_forest+f.fusion_lab+f.quantum_lab);
 r.coefficient_psych=4+2*f.hologram_theatre+f.research_hospital+f.nanohospital+2*(f.tree_farm+f.hybrid_forest);
 r.coefficient_labs=4+2*(f.network_node+f.research_hospital+f.nanohospital+f.fusion_lab+f.quantum_lab);
 int32_t*totals[]={&r.economy,&r.psych,&r.labs,&r.unknown_total};const int32_t coefficients[]={r.coefficient_economy,r.coefficient_psych,r.coefficient_labs,r.coefficient_economy};
 for(unsigned i=0;i<4;++i){int32_t product;if(!put((int64_t)coefficients[i]**totals[i],product)||!add(product,3))return EnergyError::overflow;*totals[i]=product/4;}
 if(f.supercollider&&!put(2LL*r.labs,r.labs))return EnergyError::overflow;
 if(f.theory_everything&&!put(2LL*r.labs,r.labs))return EnergyError::overflow;
 if(f.space_elevator&&!put(2LL*r.economy,r.economy))return EnergyError::overflow;
 if(f.longevity_free_market&&!add(r.economy,r.economy/2))return EnergyError::overflow;
 if(f.network_backbone&&(!add(r.labs,f.global_network_nodes)||!add(r.labs,f.commerce)))return EnergyError::overflow;
 const int effic=4-clamp(f.pending_efficiency,-4,4),imbalance=(2*f.alloc_labs+f.alloc_psych-10<0?(2*(5-f.alloc_labs)-f.alloc_psych):(2*f.alloc_labs+f.alloc_psych-10))/2;
 if(imbalance&&effic){const int penalty=imbalance*effic*2;int labs_penalty=clamp(penalty,0,40),econ_penalty=clamp(2*penalty,0,80);if(2*f.alloc_labs+f.alloc_psych<=10){labs_penalty=clamp(2*penalty,0,80);econ_penalty=clamp(penalty,0,40);}int32_t product;
  if(!put((int64_t)r.labs*(100-labs_penalty),product)||!add(product,50))return EnergyError::overflow;
  r.labs=product/100;
  if(!put((int64_t)r.economy*(100-econ_penalty),product)||!add(product,50))return EnergyError::overflow;
  r.economy=product/100;
 }
 out=r;return EnergyError::ok;
}
}
