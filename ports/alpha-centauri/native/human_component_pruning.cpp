/* SPDX-License-Identifier: GPL-3.0-or-later. See human_component_pruning.h. */
#include "human_component_pruning.h"
#include "technology_effects_ranges.h"
namespace alpha {
static bool overlaps(const OriginalWorld& w,const HumanPruningResult& out){using detail::effects_range_overlap;using detail::effects_array_overlap;return effects_range_overlap(&out,sizeof out,&w,sizeof w) || w.overlaps_byte_storage(&out,sizeof out) ||
 effects_array_overlap(&out,sizeof out,w.tile(0),w.game().map.info.tile_count,sizeof(OriginalMapTile)) || effects_array_overlap(&out,sizeof out,w.vehicle(0),w.game().vehicle_count,sizeof(OriginalVehicle)) || effects_array_overlap(&out,sizeof out,w.base(0),w.game().base_count,sizeof(OriginalBase)) || effects_array_overlap(&out,sizeof out,w.prototype(0),512,sizeof(OriginalPrototype)) || effects_array_overlap(&out,sizeof out,w.faction(0),8,sizeof(OriginalFactionState)) || effects_array_overlap(&out,sizeof out,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
static bool known(const OriginalWorld& w,const Rules& r,int32_t id,int32_t f){if(id==-1)return true;if(id<0 || id>=88)return false;const auto& t=r.technologies[id];return t.prerequisites[0]>=-1 && (t.prerequisites[1]>=-1 || t.prerequisites[0]==-1) && (w.game().preferences.TechOwners[id]&(1u<<(unsigned)f));}
HumanPruningError world_human_component_pruning(const OriginalWorld& w,Variant v,const Rules& r,const UnitDefinitions& d,PruningRuntime runtime,int32_t f,int32_t selected,HumanPruningResult& out){
 using detail::effects_range_overlap;
 if(effects_range_overlap(&out,sizeof out,&w,sizeof w) || effects_range_overlap(&out,sizeof out,&r,sizeof r) || effects_range_overlap(&out,sizeof out,&d,sizeof d) || !w.loaded() || f<1 || f>7 || selected<0 || selected>=512 || (v!=Variant::base && v!=Variant::alien_crossfire) || (w.game().data_version<11)!=(v==Variant::base) || overlaps(w,out))return HumanPruningError::bad_state;
 if(runtime!=PruningRuntime::multiplayer_off || !(w.game().preferences.FactionStatus[0]&(1u<<(unsigned)f)) || (w.game().preferences.GameMorePreferences&0x100000))return HumanPruningError::unsupported_runtime;
 if(r.count!=technology_count || d.weapon_count!=(v==Variant::base?21u:26u) || d.armor_count!=(v==Variant::base?10u:14u))return HumanPruningError::bad_state;
 for(size_t i=0;i<technology_count;++i)for(unsigned j=0;j<2;++j)if(r.technologies[i].prerequisites[j]<-2 || r.technologies[i].prerequisites[j]>=89)return HumanPruningError::bad_state;
 for(size_t i=0;i<9;++i)if(d.chassis[i].speed<0 || d.chassis[i].speed>255 || d.chassis[i].triad<0 || d.chassis[i].triad>255)return HumanPruningError::bad_state;
 for(size_t i=0;i<d.weapon_count;++i)if(d.weapons[i].offense<-128 || d.weapons[i].offense>127)return HumanPruningError::bad_state;
 for(size_t i=0;i<d.armor_count;++i)if(d.armor[i].defense<-128 || d.armor[i].defense>127)return HumanPruningError::bad_state;
 HumanPruningResult next{};
 for(size_t i=0;i<512;++i){const auto* p=w.prototype(i);if(!p)return HumanPruningError::bad_state;next.flags[i]=p->unit_flags;
  if((p->unit_flags&1u) && ((i<64) || (i>=(size_t)f*64 && i<(size_t)(f+1)*64)) && (p->weapon_id>=d.weapon_count || p->armor_id>=d.armor_count || p->chassis_id>=9 || p->preq_tech<-2 || p->preq_tech>=89))return HumanPruningError::bad_state;
 }
 if(selected<64 || (next.flags[selected]&4u))for(int32_t id=f*64;id<(f+1)*64;++id){
  if(!(next.flags[id]&1u) || (next.flags[id]&4u))continue;
  const auto& a=*w.prototype((size_t)id);bool weapon=false,armor=false,chassis=false;
  for(int32_t j=0;j<128;++j){int32_t other=j<64?j:f*64+(j%64);const auto& b=*w.prototype((size_t)other);
   if(!(next.flags[other]&1u) || (other<64?!known(w,r,b.preq_tech,f):!(next.flags[other]&4u)))continue;
   weapon|=a.weapon_id==b.weapon_id || (d.weapons[a.weapon_id].offense>0 && d.weapons[a.weapon_id].offense<=d.weapons[b.weapon_id].offense);
   armor|=a.armor_id==b.armor_id || (d.armor[a.armor_id].defense>0 && d.armor[a.armor_id].defense<=d.armor[b.armor_id].defense);
   chassis|=a.chassis_id==b.chassis_id || (d.chassis[a.chassis_id].triad==d.chassis[b.chassis_id].triad && d.chassis[a.chassis_id].speed<=d.chassis[b.chassis_id].speed);
  }
  if(weapon && armor && chassis)next.flags[id]|=0x104u;
 }
 for(size_t i=0;i<512;++i)if(next.flags[i]!=w.prototype(i)->unit_flags)++next.changed;
 out=next;return HumanPruningError::ok;
}
}
