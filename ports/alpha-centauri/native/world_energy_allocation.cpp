/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in upkeep_energy_allocation.h. */
#include "world_energy_allocation.h"
#include "world_unit_stats.h"
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){if(!a||!b||!an||!bn)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
bool aliases(const OriginalWorld&w,const Rules&t,const BasicRules&b,const CitizenDefinitions&c,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&t,sizeof t)||overlap(out,n,&b,sizeof b)||overlap(out,n,&c,sizeof c))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();
 return overlap(out,n,w.original_bytes(),w.original_size())||overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
bool built(const OriginalBase&b,unsigned id){return (b.facilities_built[id/8]&(1u<<(id%8)))!=0;}
}
static EnergyError allocate_world(const OriginalWorld&w,const Rules&t,const BasicRules&b,const CitizenDefinitions&c,size_t id,const ProposedCitizenAssignment*proposal,int32_t surplus,int32_t commerce,WorldAllocationResult&out){
 if(aliases(w,t,b,c,&out,sizeof out))return EnergyError::bad_buffer;
 if(proposal&&overlap(&out,sizeof out,proposal,sizeof *proposal))return EnergyError::bad_buffer;
 if(!w.loaded()||t.count!=89||c.count!=7||w.game().base_count>512)return EnergyError::bad_state;
 const auto*base=w.base(id);if(!base||base->faction_id<1||base->faction_id>7)return EnergyError::bad_state;
 const int faction=base->faction_id;const auto*f=w.faction((size_t)faction);if(!f)return EnergyError::bad_state;
 if(proposal){
  if(proposal->population!=base->pop_size||proposal->population<0||proposal->population>127||proposal->specialist_count<0||proposal->specialist_count>proposal->population||!(proposal->worked_tiles&1u)||(proposal->worked_tiles&~0x1fffffu))return EnergyError::bad_state;
  unsigned workers=0;for(uint32_t mask=proposal->worked_tiles;mask;mask&=mask-1)++workers;
  if((int32_t)workers+proposal->specialist_count!=proposal->population+1)return EnergyError::bad_state;
  for(int i=0;i<proposal->specialist_count&&i<16;++i)if(((proposal->specialist_types[i/8]>>(4*(i&7)))&15u)>6)return EnergyError::bad_state;
 }
 // Full SMAX alien grid/specialist policy is not silently assumed here.
 if(w.game().data_version>=11||!(w.game().preferences.FactionStatus[0]&(1u<<(unsigned)faction)))return EnergyError::bad_state;
 WorldAllocationResult next{};auto&facts=next.facts;facts.fresh_surplus=surplus;facts.commerce=commerce;facts.alloc_labs=f->SE_alloc_labs;facts.alloc_psych=f->SE_alloc_psych;facts.pending_efficiency=f->SE_effic_pending;facts.population=base->pop_size;facts.specialist_count=base->specialist_total;facts.min_specialist_population=b.min_base_size_specialists;facts.citizens=c;facts.specialist_types[0]=(uint32_t)base->specialist_types[0];facts.specialist_types[1]=(uint32_t)base->specialist_types[1];
 if(proposal){facts.specialist_count=proposal->specialist_count;facts.specialist_types[0]=proposal->specialist_types[0];facts.specialist_types[1]=proposal->specialist_types[1];}
 for(unsigned i=0;i<7;++i){bool known=false;if(world_has_technology(w,t,c.specialists[i].prerequisite,faction,known)!=LogicError::ok)return EnergyError::bad_state;facts.prerequisite_known[i]=known?1:0;if(world_has_technology(w,t,c.specialists[i].obsolete,faction,known)!=LogicError::ok)return EnergyError::bad_state;facts.obsolete_known[i]=known?1:0;}
 facts.biology_lab=built(*base,9);facts.energy_bank=built(*base,7);facts.network_node=built(*base,8);facts.hologram_theatre=built(*base,11);facts.research_hospital=built(*base,17);facts.nanohospital=built(*base,18);facts.tree_farm=built(*base,13);facts.hybrid_forest=built(*base,14);facts.fusion_lab=built(*base,15);facts.quantum_lab=built(*base,16);
 const unsigned indices[]={6,12,14,18,21,27};bool ours[6]{},here[6]{};
 for(unsigned j=0;j<6;++j){const int32_t location=w.game().preferences.SecretProjects[indices[j]];if(location< -2)return EnergyError::bad_state;if(location>=0){const auto*p=w.base((size_t)location);if(!p||p->faction_id<1||p->faction_id>7)return EnergyError::bad_state;ours[j]=p->faction_id==faction;here[j]=(size_t)location==id;}}
 facts.hologram_theatre=facts.hologram_theatre||(ours[0]&&facts.network_node);facts.supercollider=here[1];facts.longevity_free_market=here[2]&&f->SE_Economics_pending==1;facts.theory_everything=here[3];facts.network_backbone=here[4];facts.space_elevator=here[5];
 for(size_t i=0;i<w.game().base_count;++i){const auto*p=w.base(i);if(!p)return EnergyError::bad_state;if(built(*p,8))++facts.global_network_nodes;}
 const auto error=allocate_energy(facts,next.allocation);if(error!=EnergyError::ok)return error;
 if(proposal){
  // A new assignment cannot grant unavailable specialist output. Original
  // saved-slot arithmetic above remains unchanged; verify the proposed effective
  // types after its original first-dominating obsolete upgrade has resolved.
  for(int i=0;i<proposal->specialist_count&&i<16;++i){const unsigned kind=(next.allocation.specialist_types[i/8]>>(4*(i&7)))&15u;const auto&citizen=facts.citizens.specialists[kind];if(!facts.prerequisite_known[kind]||facts.obsolete_known[kind]||(citizen.psych<2&&facts.population<facts.min_specialist_population))return EnergyError::bad_state;}
  if(proposal->specialist_count>16){bool available=false;for(unsigned kind=0;kind<7;++kind)if(facts.prerequisite_known[kind]&&!facts.obsolete_known[kind]&&(facts.citizens.specialists[kind].psych>=2||facts.population>=facts.min_specialist_population))available=true;if(!available)return EnergyError::bad_state;}
 }
 out=next;return EnergyError::ok;
}
EnergyError world_energy_allocation(const OriginalWorld&w,const Rules&t,const BasicRules&b,const CitizenDefinitions&c,size_t id,int32_t surplus,int32_t commerce,WorldAllocationResult&out){return allocate_world(w,t,b,c,id,nullptr,surplus,commerce,out);}
EnergyError world_energy_allocation_for_assignment(const OriginalWorld&w,const Rules&t,const BasicRules&b,const CitizenDefinitions&c,size_t id,const ProposedCitizenAssignment&proposal,int32_t surplus,int32_t commerce,WorldAllocationResult&out){return allocate_world(w,t,b,c,id,&proposal,surplus,commerce,out);}
}
