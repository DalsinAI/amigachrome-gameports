/* SPDX-License-Identifier: GPL-3.0-or-later. See existing_prototype_reuse.h. */
#include "existing_prototype_reuse.h"
#include "technology_effects_ranges.h"
namespace alpha {
static bool known(const OriginalWorld& w,const Rules& r,int32_t id,int32_t f){if(id==-1)return true;if(id<0 || id>=88)return false;const auto& t=r.technologies[id];return t.prerequisites[0]>=-1 && (t.prerequisites[1]>=-1 || t.prerequisites[0]==-1) && (w.game().preferences.TechOwners[id]&(1u<<(unsigned)f));}
static bool overlap(const OriginalWorld& w,const ExistingProposalResult& out){using detail::effects_range_overlap;using detail::effects_array_overlap;return effects_range_overlap(&out,sizeof out,&w,sizeof w) || w.overlaps_byte_storage(&out,sizeof out) || effects_array_overlap(&out,sizeof out,w.tile(0),w.game().map.info.tile_count,sizeof(OriginalMapTile)) || effects_array_overlap(&out,sizeof out,w.vehicle(0),w.game().vehicle_count,sizeof(OriginalVehicle)) || effects_array_overlap(&out,sizeof out,w.base(0),w.game().base_count,sizeof(OriginalBase)) || effects_array_overlap(&out,sizeof out,w.prototype(0),512,sizeof(OriginalPrototype)) || effects_array_overlap(&out,sizeof out,w.faction(0),8,sizeof(OriginalFactionState)) || effects_array_overlap(&out,sizeof out,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
ExistingProposalError world_existing_prototype_proposal(const OriginalWorld& w,Variant v,const Rules& r,const UnitDefinitions& d,const ExistingProposalRequest& q,ExistingProposalResult& out){using detail::effects_range_overlap;
 if(effects_range_overlap(&out,sizeof out,&w,sizeof w) || effects_range_overlap(&out,sizeof out,&r,sizeof r) || effects_range_overlap(&out,sizeof out,&d,sizeof d) || effects_range_overlap(&out,sizeof out,&q,sizeof q) || !w.loaded() || overlap(w,out) || q.faction<1 || q.faction>7 || q.chassis<0 || q.chassis>=9 || q.weapon<0 || q.armor<0 || q.reactor<1 || q.reactor>4 || q.plan<-2 || q.plan>127 || (v!=Variant::base && v!=Variant::alien_crossfire) || (w.game().data_version<11)!=(v==Variant::base))return ExistingProposalError::bad_state;
 const bool base=v==Variant::base;const uint8_t bit=(uint8_t)(1u<<(unsigned)q.faction);
 if(q.mode!=ProposalMode::normal || !(w.game().preferences.FactionStatus[0]&bit) || (!base && q.expansion!=ExpansionObservation::disabled && q.expansion!=ExpansionObservation::enabled))return ExistingProposalError::unsupported_runtime;
 if(r.count!=89 || d.weapon_count!=(base?21u:26u) || d.armor_count!=(base?10u:14u) || d.ability_count!=(base?24u:29u) || (size_t)q.weapon>=d.weapon_count || (size_t)q.armor>=d.armor_count || (q.abilities&~((1u<<d.ability_count)-1u)))return ExistingProposalError::bad_state;
 for(size_t i=0;i<89;++i)for(unsigned j=0;j<2;++j)if(r.technologies[i].prerequisites[j]<-2 || r.technologies[i].prerequisites[j]>=89)return ExistingProposalError::bad_state;
 for(size_t i=0;i<9;++i)if(d.chassis[i].speed<0 || d.chassis[i].speed>255)return ExistingProposalError::bad_state;
 for(size_t i=0;i<d.weapon_count;++i)if(d.weapons[i].offense<-128 || d.weapons[i].offense>127)return ExistingProposalError::bad_state;
 for(size_t i=0;i<d.armor_count;++i)if(d.armor[i].defense<-128 || d.armor[i].defense>127)return ExistingProposalError::bad_state;
 const auto table=w.game().bunglist;if(!table.data || table.size!=4096)return ExistingProposalError::bad_state;
 ExistingProposalResult next{};next.id=-1;
 for(size_t i=0;i<64;++i){const auto* p=table.data+q.faction*512+i*8;const uint32_t flags=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
  if(flags==q.abilities && p[4]==q.chassis && p[5]==q.weapon && p[6]==q.armor && p[7]==q.reactor){next.kind=ExistingProposalKind::retired;out=next;return ExistingProposalError::ok;}}
 if(d.chassis[q.chassis].speed==1 && d.weapons[q.weapon].offense==1 && d.armor[q.armor].defense==1){next.kind=ExistingProposalKind::scout;next.id=2;out=next;return ExistingProposalError::ok;}
 const bool disabled=!base && q.expansion==ExpansionObservation::disabled;
 if((d.weapons[q.weapon].offense<0 && !(disabled && q.weapon>=12 && q.weapon<=14)) || (d.armor[q.armor].defense<0 && !(disabled && q.armor>9)))return ExistingProposalError::unsupported_strength;
 const auto* state=w.faction((size_t)q.faction);if(!state)return ExistingProposalError::bad_state;
 for(int32_t j=0;j<128;++j){const int32_t id=j<64?j:q.faction*64+j%64;const auto* p=w.prototype((size_t)id);if(!p)return ExistingProposalError::bad_state;
  if(!(p->unit_flags&1u) || (id>=64 && (p->obsolete_factions&bit) && !state->units_active[id] && !state->units_queue[id]))continue;
  if(id<64 && !known(w,r,p->preq_tech,q.faction))continue;
  if(p->chassis_id>=9 || p->weapon_id>=d.weapon_count || p->armor_id>=d.armor_count || p->reactor_id<1 || p->reactor_id>4)return ExistingProposalError::bad_state;
  const bool same=p->chassis_id==q.chassis && p->weapon_id==q.weapon && p->armor_id==q.armor && p->reactor_id==q.reactor;
  if(same && p->ability_flags==q.abilities){next.kind=ExistingProposalKind::duplicate;out=next;return ExistingProposalError::ok;}
  if(same && !((p->ability_flags^q.abilities)&0x44ae1bu) && (p->ability_flags&q.abilities)==p->ability_flags){next.kind=ExistingProposalKind::reused;next.id=id;next.prototype=*p;if(q.plan==-2)next.prototype.unit_flags|=4u;next.prototype.obsolete_factions=(uint8_t)(p->obsolete_factions&~bit);out=next;return ExistingProposalError::ok;}
  // Original conventional/psi comparison occurs here before its human guard.
  if(id>=64 && !((p->obsolete_factions&bit) && !state->units_active[id]) && p->chassis_id==q.chassis && p->reactor_id==q.reactor && !((p->ability_flags^q.abilities)&0x48841au) && ((d.weapons[p->weapon_id].offense<0 && !(disabled && p->weapon_id>=12 && p->weapon_id<=14)) || (d.armor[p->armor_id].defense<0 && !(disabled && p->armor_id>9))))return ExistingProposalError::unsupported_strength;
 }
 return ExistingProposalError::allocation_required;
}
}
