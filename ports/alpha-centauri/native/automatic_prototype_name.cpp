/* SPDX-License-Identifier: GPL-3.0-or-later. See automatic_prototype_name.h. */
#include "automatic_prototype_name.h"
#include "technology_effects_ranges.h"
#include "prototype_prefix_context.h"
#include <string.h>
#include <initializer_list>
namespace alpha {
template<class Result> static bool overlap(const OriginalWorld& w,const Result& out){using detail::effects_range_overlap;using detail::effects_array_overlap;return effects_range_overlap(&out,sizeof out,&w,sizeof w) || w.overlaps_byte_storage(&out,sizeof out) || effects_array_overlap(&out,sizeof out,w.tile(0),w.game().map.info.tile_count,sizeof(OriginalMapTile)) || effects_array_overlap(&out,sizeof out,w.vehicle(0),w.game().vehicle_count,sizeof(OriginalVehicle)) || effects_array_overlap(&out,sizeof out,w.base(0),w.game().base_count,sizeof(OriginalBase)) || effects_array_overlap(&out,sizeof out,w.prototype(0),512,sizeof(OriginalPrototype)) || effects_array_overlap(&out,sizeof out,w.faction(0),8,sizeof(OriginalFactionState)) || effects_array_overlap(&out,sizeof out,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
static bool string_ok(const char* s,size_t n){return memchr(s,0,n)!=nullptr;}
struct NameText {char b[256]{};bool ok=true;void add(const char* s){size_t n=strlen(b),k=strlen(s);if(n+k>=sizeof b){ok=false;return;}memcpy(b+n,s,k+1);}void space(){add(" ");}void trim(){size_t n=strlen(b);if(n)b[n-1]=0;}};
static bool known(const OriginalWorld& w,const Rules& r,int32_t id,int32_t f){if(id==-1)return true;if(id<0 || id>=88)return false;const auto& t=r.technologies[id];return t.prerequisites[0]>=-1 && (t.prerequisites[1]>=-1 || t.prerequisites[0]==-1) && (w.game().preferences.TechOwners[id]&(1u<<(unsigned)f));}
template<class Result> static PrototypeNameError name_core(const OriginalWorld& w,Variant v,const Rules& r,const UnitDefinitions& d,const PrototypeNameLabels& labels,int32_t id,const OriginalPrototype& candidate,const PrototypeNameSuffix* suffix,bool resolve,const RetirementHistoryResult* history,Result& out){using detail::effects_range_overlap;
 if(effects_range_overlap(&out,sizeof out,&w,sizeof w) || effects_range_overlap(&out,sizeof out,&r,sizeof r) || effects_range_overlap(&out,sizeof out,&d,sizeof d) || effects_range_overlap(&out,sizeof out,&labels,sizeof labels) || effects_range_overlap(&out,sizeof out,&candidate,sizeof candidate) || (suffix && effects_range_overlap(&out,sizeof out,suffix,sizeof *suffix)) || !w.loaded() || overlap(w,out) || id<64 || id>=512 || (v!=Variant::base && v!=Variant::alien_crossfire) || (w.game().data_version<11)!=(v==Variant::base))return PrototypeNameError::bad_state;
 if(history && (effects_range_overlap(&out,sizeof out,history,sizeof *history) || history->next_slot<0 || history->next_slot>63))return PrototypeNameError::bad_state;
 const bool base=v==Variant::base;const int f=id/64;
 if(resolve && !base)return PrototypeNameError::collision_context_required;
 if(resolve && (!suffix || !string_ok(suffix->token,sizeof suffix->token) || !suffix->token[0]))return PrototypeNameError::bad_state;
 if(!(candidate.unit_flags&1u) || r.count!=89 || d.weapon_count!=(base?21u:26u) || d.armor_count!=(base?10u:14u) || d.ability_count!=(base?24u:29u) || candidate.chassis_id>=9 || candidate.weapon_id>=d.weapon_count || candidate.armor_id>=d.armor_count || candidate.reactor_id<1 || candidate.reactor_id>4 || (candidate.ability_flags&~((1u<<d.ability_count)-1u)))return PrototypeNameError::bad_state;
 for(size_t i=0;i<9;++i){const auto& c=d.chassis[i];if(c.speed<0 || c.speed>255 || c.triad<0 || c.triad>2 || c.missile<0 || c.missile>255)return PrototypeNameError::bad_state;for(const auto& n:c.nouns)if(!string_ok(n.name,sizeof n.name))return PrototypeNameError::bad_state;}
 for(size_t i=0;i<d.weapon_count;++i)if(d.weapons[i].offense<-128 || d.weapons[i].offense>127 || d.weapons[i].mode<0 || d.weapons[i].mode>255 || !string_ok(d.weapons[i].short_name,128))return PrototypeNameError::bad_state;
 for(size_t i=0;i<d.armor_count;++i)if(d.armor[i].defense<-128 || d.armor[i].defense>127 || !string_ok(d.armor[i].short_name,128))return PrototypeNameError::bad_state;
 for(size_t i=0;i<4;++i)if(!string_ok(d.reactors[i].short_name,128))return PrototypeNameError::bad_state;
 for(size_t i=0;i<d.ability_count;++i)if(!string_ok(d.abilities[i].abbreviation,128) || d.abilities[i].prerequisite<-2 || d.abilities[i].prerequisite>=89)return PrototypeNameError::bad_state;
 for(const auto* s:{labels.scout,labels.recon,labels.sea,labels.amphibious1,labels.amphibious2,labels.artillery1,labels.artillery2})if(!string_ok(s,128))return PrototypeNameError::bad_state;
 for(size_t i=0;i<89;++i)for(unsigned j=0;j<2;++j)if(r.technologies[i].prerequisites[j]<-2 || r.technologies[i].prerequisites[j]>=89)return PrototypeNameError::bad_state;
 const auto& c=d.chassis[candidate.chassis_id];const auto& weapon=d.weapons[candidate.weapon_id];const auto& armor=d.armor[candidate.armor_id];const uint32_t flags=candidate.ability_flags;const int offense=weapon.offense,defense=armor.defense,mode=weapon.mode;const char* reactor=d.reactors[candidate.reactor_id-1].short_name;NameText primary,alternate;
 for(size_t i=0;i<d.ability_count;++i)if((flags&(1u<<(unsigned)i)) && d.abilities[i].abbreviation[0] && !(i==5 && c.triad==2) && i!=15 && !(i==3 && c.speed==1 && offense>=defense && !(flags&0x8000u))){primary.add(d.abilities[i].abbreviation);primary.space();break;}
 alternate=primary;bool large=false;if(mode<3){int limit=offense<defense?6:3;if(c.triad==1)++limit;if(c.triad==2)limit+=limit/2;large=(offense<defense?offense:defense)>limit && c.nouns[4].name[0] && c.nouns[5].name[0];}
 bool special=false;unsigned parity=((candidate.reactor_id-1)^f)&1u;
 if((candidate.group_id==10 && !(w.game().preferences.FactionStatus[0]&(1u<<(unsigned)f)) && c.speed>1) || (offense<=c.speed && defense<=c.speed && offense>=0 && c.triad==0)){
  if(mode<3){bool police=(flags&0x400000u)!=0;
   if(!police && !base){const auto* faction=w.faction_definition((size_t)f);if(!faction || faction->faction_bonus_count<0 || faction->faction_bonus_count>8)return PrototypeNameError::bad_state;for(int i=0;i<faction->faction_bonus_count;++i)if(faction->faction_bonus_id[i]==15){const int a=faction->faction_bonus_val1[i];if(a<0 || (size_t)a>=d.ability_count)return PrototypeNameError::bad_state;if(a==22 && known(w,r,d.abilities[a].prerequisite,f))police=true;}}
   if(police){primary.trim();if(alternate.b[0]){size_t n=strlen(primary.b);if(!n)return PrototypeNameError::bad_state;alternate.b[n-1]=0;}}else{primary.add(offense<2?labels.scout:labels.recon);alternate.add(offense<2?labels.scout:labels.recon);}special=true;parity=c.speed>1;
  }
 }
 bool upgraded=false;if(!special)for(int j=0;j<128;++j){const int other=j<64?j:f*64+j%64;const auto* ptr=other==id?&candidate:w.prototype((size_t)other);if(!ptr)return PrototypeNameError::bad_state;const auto& a=*ptr;if((a.unit_flags&1u) && a.group_id==candidate.group_id && a.chassis_id==candidate.chassis_id){if(a.reactor_id>=candidate.reactor_id){upgraded=false;break;}upgraded=true;}}
 auto both=[&](const char* s){primary.add(s);alternate.add(s);};auto spaces=[&](){primary.space();alternate.space();};
 const bool defensive=offense<defense && mode<3 && offense>=0 && !(flags&0x8000u);
 const bool interceptor=c.triad==2 && (flags&0x20u) && mode<3 && offense>=0;
 if(defensive || interceptor){if(!special){parity^=(unsigned)(defensive?defense:offense)&1u;both(upgraded?reactor:(defensive?armor.short_name:weapon.short_name));}spaces();primary.add(c.nouns[2].name);alternate.add((!special || !(id&1))?(large && c.triad==1?c.nouns[5].name:c.nouns[3].name):(large && c.triad!=1?c.nouns[5].name:c.nouns[2].name));}
 else if(c.missile && offense>98){both(reactor);spaces();both(weapon.short_name);}
 else if(c.missile && (candidate.weapon_id==24 || candidate.weapon_id==25)){both(weapon.short_name);spaces();both(reactor);spaces();both(c.nouns[0].name);}
 else{
  if(mode>2 && candidate.chassis_id!=0){if(candidate.reactor_id>1){both(reactor);spaces();}if(!(mode==7 && candidate.chassis_id==3) && !(mode==9 && c.triad==0 && strchr(primary.b,' '))){if((mode==8 || mode==9) && candidate.chassis_id==3)both(labels.sea);else{primary.add(c.nouns[0].name);alternate.add(c.nouns[1].name);}spaces();}}
  if(!special){parity^=(unsigned)offense&1u;both(weapon.short_name);}
  if(mode<3){spaces();if(flags&0x8000u){primary.add(labels.artillery1);alternate.add(labels.artillery2);}else if((flags&8u) && c.speed==1 && offense>defense){primary.add(labels.amphibious1);alternate.add(labels.amphibious2);}else{primary.add(upgraded?reactor:(large && c.triad!=1?c.nouns[4].name:c.nouns[0].name));alternate.add(upgraded?reactor:(large && c.triad==1?c.nouns[4].name:c.nouns[1].name));}}
 }
 if(!primary.ok || !alternate.ok)return PrototypeNameError::overflow;
 if(!resolve){
 for(int i=0;i<512;++i){if(i==id || (i/64!=0 && i/64!=f))continue;const auto* a=w.prototype((size_t)i);if(!a || !string_ok(a->name,sizeof a->name))return PrototypeNameError::bad_state;if(a->chassis_id==candidate.chassis_id && a->weapon_id==candidate.weapon_id && a->armor_id==candidate.armor_id && a->reactor_id==candidate.reactor_id && a->ability_flags==flags)continue;if(!strcmp(primary.b,a->name) || (strcmp(primary.b,alternate.b) && !strcmp(alternate.b,a->name)))return PrototypeNameError::collision_context_required;}
 }else{
  const bool distinct=strcmp(primary.b,alternate.b)!=0;bool primaryLimit=false;
  auto advance=[&](NameText& text){const char* token=suffix->token;const char* last=nullptr;for(const char* q=strstr(text.b,token);q;q=strstr(q+1,token))last=q;
   if(!last || strlen(last)>strlen(token)+1){text.space();text.add(token);text.add("2");return true;}
   const size_t n=strlen(text.b);if(n && text.b[n-1]>='2' && text.b[n-1]<='8'){++text.b[n-1];return true;}primaryLimit=true;return false;
  };
  for(int i=0;i<512;++i){if(i==id || (i/64!=0 && i/64!=f))continue;const auto* a=w.prototype((size_t)i);if(!a || !string_ok(a->name,sizeof a->name))return PrototypeNameError::bad_state;
   if(a->chassis_id==candidate.chassis_id && a->weapon_id==candidate.weapon_id && a->armor_id==candidate.armor_id && a->reactor_id==candidate.reactor_id && a->ability_flags==flags)continue;
   if(!primaryLimit && !strcmp(primary.b,a->name)){if(distinct)parity=1;advance(primary);if(!primary.ok)return PrototypeNameError::overflow;i=-1;}
   if(distinct){bool equal=false;if(i<0){if(history){uint8_t prefix[84];const auto* first=w.prototype(0);if(!first || !memchr(first->name,0,32))return PrototypeNameError::bad_state;memcpy(prefix,history->table+4044,52);memcpy(prefix+52,first->name,32);equal=!strcmp(alternate.b,(const char*)prefix);}else{PrototypePrefixFact fact{};PrototypePrefixComparison comparison{};if(world_prototype_prefix_compare(w,Variant::base,fact,(const uint8_t*)alternate.b,strlen(alternate.b)+1,comparison)!=PrototypePrefixError::ok)return PrototypeNameError::bad_state;equal=comparison.sign==0;}}
    else{const auto* other=w.prototype((size_t)i);if(!other || !string_ok(other->name,sizeof other->name))return PrototypeNameError::bad_state;equal=!strcmp(alternate.b,other->name);}
    if(equal){parity=0;const bool changed=advance(alternate);if(!alternate.ok)return PrototypeNameError::overflow;if(!changed && i>=0)return PrototypeNameError::original_would_not_return;i=-1;}
   }
  }
 }
 const char* selected=parity?alternate.b:primary.b;if(strlen(selected)>=sizeof out.bytes)return PrototypeNameError::overflow;Result next{};memcpy(next.bytes,selected,strlen(selected)+1);out=next;return PrototypeNameError::ok;
}
PrototypeNameError world_automatic_prototype_name(const OriginalWorld& w,Variant v,const Rules& r,const UnitDefinitions& d,const PrototypeNameLabels& labels,int32_t id,const OriginalPrototype& candidate,PrototypeName& out){return name_core(w,v,r,d,labels,id,candidate,nullptr,false,nullptr,out);}
PrototypeNameError world_resolve_automatic_prototype_name(const OriginalWorld& w,Variant v,const Rules& r,const UnitDefinitions& d,const PrototypeNameLabels& labels,const PrototypeNameSuffix& suffix,int32_t id,const OriginalPrototype& candidate,PrototypeName& out){return name_core(w,v,r,d,labels,id,candidate,&suffix,true,nullptr,out);}
PrototypeNameError world_resolve_automatic_prototype_logical_name(const OriginalWorld& w,Variant v,const Rules& r,const UnitDefinitions& d,const PrototypeNameLabels& labels,const PrototypeNameSuffix& suffix,int32_t id,const OriginalPrototype& candidate,PrototypeLogicalName& out){return name_core(w,v,r,d,labels,id,candidate,&suffix,true,nullptr,out);}
PrototypeNameError world_resolve_automatic_prototype_logical_name_history(const OriginalWorld& w,Variant v,const Rules& r,const UnitDefinitions& d,const PrototypeNameLabels& labels,const PrototypeNameSuffix& suffix,int32_t id,const OriginalPrototype& candidate,const RetirementHistoryResult& history,PrototypeLogicalName& out){return name_core(w,v,r,d,labels,id,candidate,&suffix,true,&history,out);}
}
