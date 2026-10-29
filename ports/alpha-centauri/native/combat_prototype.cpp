/* SPDX-License-Identifier: GPL-3.0-or-later. See combat_prototype.h. */
#include "combat_prototype.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
bool i32(int64_t n){return n>=(-2147483647LL-1)&&n<=2147483647LL;}
bool byte_signed(int32_t n){return n>=-128&&n<=127;}
bool byte_unsigned(int32_t n){return n>=0&&n<=255;}
bool profile(Variant v,const UnitDefinitions&d){return (v==Variant::base&&d.weapon_count==21&&d.armor_count==10&&d.ability_count==24)||(v==Variant::alien_crossfire&&d.weapon_count==26&&d.armor_count==14&&d.ability_count==29);}
}
LogicError prototype_combat_ratings(Variant v,const UnitDefinitions&d,const BasicRules&r,
 const OriginalPrototype&p,size_t id,const OriginalPrototype&o,size_t oid,bool bombard,PrototypeCombatRatings&out){
 using detail::effects_range_overlap;
 if(!profile(v,d)||id>=512||oid>=512||p.chassis_id>=9||o.chassis_id>=9||p.weapon_id>=d.weapon_count||o.weapon_id>=d.weapon_count||p.armor_id>=d.armor_count||o.armor_id>=d.armor_count||
 effects_range_overlap(&out,sizeof out,&d,sizeof d)||effects_range_overlap(&out,sizeof out,&r,sizeof r)||effects_range_overlap(&out,sizeof out,&p,sizeof p)||effects_range_overlap(&out,sizeof out,&o,sizeof o))return LogicError::bad_state;
 const auto&c=d.chassis[p.chassis_id];const auto&oc=d.chassis[o.chassis_id];const auto&w=d.weapons[p.weapon_id];const auto&ow=d.weapons[o.weapon_id];const auto&a=d.armor[p.armor_id];const auto&oa=d.armor[o.armor_id];
 if(c.triad<0||c.triad>2||oc.triad<0||oc.triad>2||!byte_unsigned(c.missile)||!byte_unsigned(w.mode)||!byte_signed(w.offense)||!byte_signed(ow.offense)||!byte_signed(a.defense)||!byte_signed(oa.defense))return LogicError::bad_state;
 const int32_t atk[3]={r.psi_combat_ratio_atk_land,r.psi_combat_ratio_atk_sea,r.psi_combat_ratio_atk_air};
 const int32_t def[3]={r.psi_combat_ratio_def_land,r.psi_combat_ratio_def_sea,r.psi_combat_ratio_def_air};
 for(unsigned i=0;i<3;++i)if(atk[i]<1||atk[i]>1000||def[i]<1||def[i]>1000)return LogicError::bad_state;
 PrototypeCombatRatings n{};
 if(w.mode==11&&o.plan==11){n.offense=n.defense=16;n.probe_duel=true;out=n;return LogicError::ok;}
 const bool spore=v==Variant::alien_crossfire&&(id==15||oid==15);
 const bool conventional_offense=(bombard||(w.offense>=0&&oa.defense>=0))&&!spore;
 const bool psi_defense=(!bombard||spore)&&(a.defense<0||ow.offense<0);
 n.offense_psi=!conventional_offense;n.defense_psi=psi_defense;
 if(n.offense_psi)n.offense=atk[oc.triad]*8;
 else{
  int32_t value=w.offense<0?-w.offense:w.offense;
  // Original compares the signed raw weapon byte with99, before abs().
  if(c.missile&&w.offense<99)value=value*3/2;
  n.offense=value*8;
 }
 n.defense=psi_defense?def[c.triad]*8:(a.defense<1?1:a.defense)*8;
 out=n;return LogicError::ok;
}
LogicError combat_psi_factor(Variant v,int32_t strength,int32_t rule,bool attacking,bool tower,bool neural,bool dream,int32_t&out){
 if(v!=Variant::base&&v!=Variant::alien_crossfire)return LogicError::bad_state;
 int64_t n=strength;
 if(rule){const int64_t percent=(int64_t)rule+100;if(!i32(percent)||!i32(percent*n))return LogicError::overflow;n=percent*n/100;}
 if((attacking&&dream)||(!attacking&&neural)){n+=n/2;if(!i32(n))return LogicError::overflow;}
 if(v==Variant::alien_crossfire&&!attacking&&tower){n+=n/2;if(!i32(n))return LogicError::overflow;}
 out=(int32_t)n;return LogicError::ok;
}
}
