/* SPDX-License-Identifier: GPL-3.0-or-later. See world_commerce.h. */
#include "world_commerce.h"
#include "technology_effects_ranges.h"
namespace alpha { namespace {
using detail::effects_range_overlap;
bool array_overlap(const void* out,size_t n,const void* p,size_t count,size_t size) {
 return detail::effects_array_overlap(out,n,p,count,size);
}
bool sealed(const OriginalWorld& w,const Rules& r,const void* out,size_t n) {
 if (effects_range_overlap(out,n,&w,sizeof w) || effects_range_overlap(out,n,&r,sizeof r)) return false;
 if (!w.loaded()) return false;
 return !(effects_range_overlap(out,n,w.original_bytes(),w.original_size()) ||
 array_overlap(out,n,w.tile(0),w.game().map.info.tile_count,sizeof(OriginalMapTile)) ||
 array_overlap(out,n,w.vehicle(0),w.game().vehicle_count,sizeof(OriginalVehicle)) ||
 array_overlap(out,n,w.base(0),w.game().base_count,sizeof(OriginalBase)) ||
 array_overlap(out,n,w.prototype(0),512,sizeof(OriginalPrototype)) ||
 array_overlap(out,n,w.faction(0),8,sizeof(OriginalFactionState)) ||
 array_overlap(out,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition)));
}
bool word(int64_t value,int32_t& out) { if (value<INT32_MIN || value>INT32_MAX) return false;out=(int32_t)value;return true; }
LogicError score(const OriginalWorld& w,size_t id,int32_t& value) {
 int32_t product;if (!word((int64_t)w.base(id)->energy_intake*512,product) || !word((int64_t)product+(int64_t)id,value)) return LogicError::overflow;
 return LogicError::ok;
}
LogicError ranked(const OriginalWorld& w,int32_t faction,int32_t position,int32_t& out) {
 int32_t scores[512],ids[512];size_t count=0;
 for (size_t i=0;i<w.game().base_count;++i) if (w.base(i)->faction_id==faction) {
  auto error=score(w,i,scores[count]);if (error!=LogicError::ok) return error;ids[count++]=(int32_t)i;
 }
 out=-1;if (position<0 || (size_t)position>=count) return LogicError::ok;
 for (size_t i=1;i<count;++i) for (size_t j=i;j && scores[j]>scores[j-1];--j) {
  const auto s=scores[j];scores[j]=scores[j-1];scores[j-1]=s;const auto id=ids[j];ids[j]=ids[j-1];ids[j-1]=id;
 }
 out=ids[position];return LogicError::ok;
}
LogicError income(int32_t value,int32_t bonus,int32_t denominator,int32_t& out) {
 int32_t factor,product,numerator;
 if (!word((int64_t)bonus+1,factor) || !word((int64_t)factor*value,product) || !word((int64_t)product+denominator/2,numerator)) return LogicError::overflow;
 out=numerator/denominator;return LogicError::ok;
}
}
LogicError world_commerce(const OriginalWorld& w,const Rules& rules,const CommerceBonuses& bonuses,
 size_t base,WorldCommerceResult& out) {
 if (!sealed(w,rules,&out,sizeof out) || effects_range_overlap(&out,sizeof out,&bonuses,sizeof bonuses)) return LogicError::bad_state;
 if (rules.count!=technology_count || !w.base(base) || w.game().base_count>512) return LogicError::bad_state;
 const auto variant=w.game().data_version<11?Variant::base:Variant::alien_crossfire;
 if (bonuses.variant!=variant) return LogicError::bad_state;
 WorldCommerceResult next{};next.variant=variant;next.base=(int32_t)base;
 for (const auto& t:rules.technologies) {
  if (t.prerequisites[0]<-2 || t.prerequisites[0]>=89 || t.prerequisites[1]<-2 || t.prerequisites[1]>=89) return LogicError::bad_state;
  if (t.prerequisites[0]>=-1 && t.prerequisites[1]>=-1 && (t.flags&4)) ++next.global_technology_count;
 }
 const auto& selected=*w.base(base);const int32_t own=selected.faction_id;int32_t selected_score;
 auto error=score(w,base,selected_score);if (error!=LogicError::ok) return error;
 for (size_t i=0;i<w.game().base_count;++i) if (w.base(i)->faction_id==own) {
  int32_t value;error=score(w,i,value);if (error!=LogicError::ok) return error;if (value>selected_score) ++next.rank;
 }
 const auto& pref=w.game().preferences;const auto& own_state=*w.faction(own);
 for (int32_t foreign=1;foreign<8;++foreign) {
  const unsigned slot=(unsigned)foreign-1;next.partner[slot]=-1;
  const auto& other=*w.faction(foreign);
  if (foreign==own || !((own_state.diplo_status[foreign]|other.diplo_status[own])&2) || !other.base_count) continue;
  error=ranked(w,foreign,next.rank,next.partner[slot]);if (error!=LogicError::ok) return error;
  if (next.partner[slot]<0) continue;
  next.qualified_foreign_commerce=1;const auto& partner=*w.base((size_t)next.partner[slot]);int32_t numerator,value;
  // Original x86 LEA forms the complete three-operand signed32 sum once.
  if (!word((int64_t)selected.energy_intake+partner.energy_intake+7,numerator)) return LogicError::overflow;
  value=numerator/8;if (pref.ProposalPassCount[3]>pref.ProposalPassCount[4] && !word((int64_t)value*2,value)) return LogicError::overflow;
  int32_t imp,exp;const int32_t denominator=next.global_technology_count+1;
  // Original evaluates the partner (export) numerator before the import.
  error=income(value,bonuses.faction[foreign],denominator,exp);if (error!=LogicError::ok) return error;
  error=income(value,bonuses.faction[own],denominator,imp);if (error!=LogicError::ok) return error;
  if (!(own_state.diplo_status[foreign]&1)) { imp/=2;exp/=2; }
  if (pref.GovernorFaction==own && !word((int64_t)imp+1,imp)) return LogicError::overflow;
  if (pref.GovernorFaction==foreign && !word((int64_t)exp+1,exp)) return LogicError::overflow;
  if (own_state.sanction_turns || other.sanction_turns ||
   (variant==Variant::alien_crossfire && ((w.faction_definition(own)->rule_flags|w.faction_definition(foreign)->rule_flags)&0x80))) imp=exp=0;
  next.imports[slot]=imp;next.exports[slot]=exp;
  if (!word((int64_t)next.total+imp,next.total)) return LogicError::overflow;
 }
 out=next;return LogicError::ok;
}
LogicError fresh_world_commerce(const OriginalWorld& w,const Rules& rules,size_t base,WorldCommerceResult& out) {
 if (!sealed(w,rules,&out,sizeof out) || !w.base(base)) return LogicError::bad_state;
 CommerceBonuses bonuses{};bonuses.variant=w.game().data_version<11?Variant::base:Variant::alien_crossfire;
 for (int32_t faction=0;faction<8;++faction) {
  TechnologyEffectsResult result;auto error=world_technology_effects(w,rules,faction,result);if (error!=LogicError::ok) return error;bonuses.faction[faction]=result.commerce;
 }
 return world_commerce(w,rules,bonuses,base,out);
}
}
