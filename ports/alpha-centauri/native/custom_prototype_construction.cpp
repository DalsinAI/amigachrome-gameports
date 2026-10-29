/* SPDX-License-Identifier: GPL-3.0-or-later. See custom_prototype_construction.h. */
#include "custom_prototype_construction.h"
#include "technology_effects_ranges.h"
#include <string.h>
namespace alpha {
static bool known(const OriginalWorld& w,const Rules& r,int32_t id,int32_t f){if(id==-1)return true;if(id<0 || id>=88)return false;const auto& t=r.technologies[id];return t.prerequisites[0]>=-1 && (t.prerequisites[1]>=-1 || t.prerequisites[0]==-1) && (w.game().preferences.TechOwners[id]&(1u<<(unsigned)f));}
static bool overlap(const OriginalWorld& w,const OriginalPrototype& out){using detail::effects_range_overlap;using detail::effects_array_overlap;return effects_range_overlap(&out,sizeof out,&w,sizeof w) || w.overlaps_byte_storage(&out,sizeof out) || effects_array_overlap(&out,sizeof out,w.tile(0),w.game().map.info.tile_count,sizeof(OriginalMapTile)) || effects_array_overlap(&out,sizeof out,w.vehicle(0),w.game().vehicle_count,sizeof(OriginalVehicle)) || effects_array_overlap(&out,sizeof out,w.base(0),w.game().base_count,sizeof(OriginalBase)) || effects_array_overlap(&out,sizeof out,w.prototype(0),512,sizeof(OriginalPrototype)) || effects_array_overlap(&out,sizeof out,w.faction(0),8,sizeof(OriginalFactionState)) || effects_array_overlap(&out,sizeof out,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
static LogicError candidate_ability(const OriginalWorld& w,Variant v,const Rules& r,const UnitDefinitions& d,const CustomPrototypeRequest& q,uint32_t mask,bool& out){
 if(q.abilities&mask){out=true;return LogicError::ok;}if(v==Variant::base){out=false;return LogicError::ok;}
 const auto* f=w.faction_definition((size_t)q.id/64);if(!f || f->faction_bonus_count<0 || f->faction_bonus_count>8)return LogicError::bad_state;
 bool has=false;for(int32_t i=0;i<f->faction_bonus_count;++i)if(f->faction_bonus_id[i]==15){int32_t a=f->faction_bonus_val1[i];if(a<0 || (size_t)a>=d.ability_count || d.abilities[a].prerequisite<-2 || d.abilities[a].prerequisite>=89)return LogicError::bad_state;
  if(known(w,r,d.abilities[a].prerequisite,q.id/64) && (mask&(1u<<(unsigned)a))){has=true;break;}}
 out=has;return LogicError::ok;
}
static LogicError core(const OriginalWorld& w,Variant v,const Rules& r,const UnitDefinitions& d,const CustomPrototypeRequest& q,const OriginalPrototype* retired,OriginalPrototype& out){using detail::effects_range_overlap;
 if(effects_range_overlap(&out,sizeof out,&w,sizeof w) || effects_range_overlap(&out,sizeof out,&r,sizeof r) || effects_range_overlap(&out,sizeof out,&d,sizeof d) || effects_range_overlap(&out,sizeof out,&q,sizeof q) || !w.loaded() || q.id<64 || q.id>=512 || q.chassis<0 || q.chassis>=9 || q.weapon<0 || q.armor<0 || q.reactor<1 || q.reactor>4 || (v!=Variant::base && v!=Variant::alien_crossfire) || (w.game().data_version<11)!=(v==Variant::base) || overlap(w,out))return LogicError::bad_state;
 if(retired){if(effects_range_overlap(&out,sizeof out,retired,sizeof *retired))return LogicError::bad_state;const auto* old=w.prototype((size_t)q.id);if(!old || !(old->unit_flags&1u))return LogicError::bad_state;auto expected=*old;expected.unit_flags&=(uint16_t)~1u;if(memcmp(&expected,retired,sizeof expected))return LogicError::bad_state;}
 auto get=[&](size_t id)->const OriginalPrototype*{return retired && id==(size_t)q.id?retired:w.prototype(id);};
 const bool base=v==Variant::base;if(r.count!=89 || d.weapon_count!=(base?21u:26u) || d.armor_count!=(base?10u:14u) || d.ability_count!=(base?24u:29u) || (size_t)q.weapon>=d.weapon_count || (size_t)q.armor>=d.armor_count || (q.abilities&~((1u<<d.ability_count)-1u)))return LogicError::bad_state;
 for(size_t i=0;i<89;++i)for(unsigned j=0;j<2;++j)if(r.technologies[i].prerequisites[j]<-2 || r.technologies[i].prerequisites[j]>=89)return LogicError::bad_state;
 for(size_t i=0;i<9;++i){const auto& c=d.chassis[i];if(c.speed<0 || c.speed>255 || c.triad<0 || c.triad>2 || c.cargo<0 || c.cargo>255 || c.cost<0 || c.cost>255 || c.missile<0 || c.missile>255)return LogicError::bad_state;}
 for(size_t i=0;i<d.weapon_count;++i)if(d.weapons[i].offense<-128 || d.weapons[i].offense>127 || d.weapons[i].cost<0 || d.weapons[i].cost>255 || d.weapons[i].mode<0 || d.weapons[i].mode>95)return LogicError::bad_state;
 for(size_t i=0;i<d.armor_count;++i)if(d.armor[i].defense<-128 || d.armor[i].defense>127 || d.armor[i].cost<1 || d.armor[i].cost>255)return LogicError::bad_state;
 const int32_t f=q.id/64;bool cw=false,ca=false,cc=false;
 const auto& c=d.chassis[q.chassis];const auto& wp=d.weapons[q.weapon];const auto& ar=d.armor[q.armor];
 for(int32_t j=0;j<128;++j){int32_t id=j<64?j:f*64+j%64;const auto* p=get((size_t)id);if(!p)return LogicError::bad_state;
  if(!(p->unit_flags&1u) || (id<64?!known(w,r,p->preq_tech,f):!(p->unit_flags&4u)))continue;
  if(p->chassis_id>=9 || p->weapon_id>=d.weapon_count || p->armor_id>=d.armor_count)return LogicError::bad_state;
  cw|=p->weapon_id==q.weapon || (wp.offense>0 && wp.offense<=d.weapons[p->weapon_id].offense);
  ca|=p->armor_id==q.armor || (ar.defense>0 && ar.defense<=d.armor[p->armor_id].defense);
  cc|=p->chassis_id==q.chassis || (c.triad==d.chassis[p->chassis_id].triad && c.speed<=d.chassis[p->chassis_id].speed);
 }
 uint32_t price=0;auto error=calculate_prototype_cost(d,q.chassis,q.weapon,q.armor,q.abilities,q.reactor,price);if(error!=LogicError::ok)return error;
 int64_t carry=0;if(wp.mode==7){carry=c.cargo;if(c.triad==1)carry*=q.reactor;if(q.abilities&0x800000u)carry/=2;if(q.abilities&0x80000u)carry=(carry*3+1)/2;}
 int32_t plan=0,group=0;
 if(c.missile){if(wp.offense<99){if(q.weapon==24){plan=13;group=22;}else if(q.weapon==25){plan=14;group=23;}else{plan=0;group=13;}}else{plan=5;group=14;}}
 else if(wp.mode>=7){plan=wp.mode;group=wp.mode+32;}
 else if(c.triad==1){plan=6;group=6;}
 else if(c.triad==2){bool has=false;error=candidate_ability(w,v,r,d,q,0x20u,has);if(error!=LogicError::ok)return error;plan=has?4:0;group=has?9:8;}
 else{plan=ar.defense>1?2:0;if(wp.offense>=ar.defense && plan==2)plan=1;if(c.speed>1 && wp.offense>ar.defense)plan=0;if(q.abilities&0x8018u)plan=0;
  group=3;if(ar.defense>wp.offense)group=2;if(wp.offense>ar.defense*2)group=1;if(c.speed>1)group=4;if(c.speed>2)group=5;
  if(wp.offense==1 && ar.defense==1){if(c.speed>1)plan=3;bool has=false;error=candidate_ability(w,v,r,d,q,0x400000u,has);if(error!=LogicError::ok)return error;if(has)plan=2;group=10;}}
 if(price>255 || carry<0 || carry>255 || plan<0 || plan>127 || group<0 || group>127)return LogicError::overflow;
 const auto* old=get((size_t)q.id);if(!old)return LogicError::bad_state;OriginalPrototype next=*old;
 next.chassis_id=(uint8_t)q.chassis;next.weapon_id=(uint8_t)q.weapon;next.armor_id=(uint8_t)q.armor;next.reactor_id=(uint8_t)q.reactor;next.ability_flags=q.abilities;next.cost=(uint8_t)price;next.carry_capacity=(uint8_t)carry;next.plan=(int8_t)plan;next.group_id=(int8_t)group;next.icon_offset=-1;next.obsolete_factions=0;next.combat_factions=(uint8_t)(1u<<(unsigned)f);next.unit_flags=1;
 if(cw && ca && cc){const auto* last=get((size_t)(f+1)*64-1);if(!last)return LogicError::bad_state;next.unit_flags=(last->unit_flags&0x10u)?0x115:0x105;}
 out=next;return LogicError::ok;
}
LogicError world_construct_custom_prototype(const OriginalWorld& w,Variant v,const Rules& r,const UnitDefinitions& d,const CustomPrototypeRequest& q,OriginalPrototype& out){return core(w,v,r,d,q,nullptr,out);}
LogicError world_construct_custom_prototype_after_retirement(const OriginalWorld& w,Variant v,const Rules& r,const UnitDefinitions& d,const CustomPrototypeRequest& q,const OriginalPrototype& retired,OriginalPrototype& out){return core(w,v,r,d,q,&retired,out);}
}
