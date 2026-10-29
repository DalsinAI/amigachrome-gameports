/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "combat_labels.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
using E=CombatLabelError;
bool overlap(const void*a,size_t n,const void*b,size_t m){return detail::effects_range_overlap(a,n,b,m);}
bool line(const uint8_t*b,size_t n,size_t&pos,OriginalLabelSpan&s){
 if(pos>=n)return false;
 const size_t start=pos;while(pos<n&&b[pos]!='\n'){if(!b[pos])return false;++pos;}
 size_t end=pos;const bool lf=pos<n;if(lf)++pos;if(lf&&end>start&&b[end-1]=='\r')--end;
 if(end-start>(lf?509u:510u))return false;
 for(size_t i=start;i<end;++i)if(b[i]=='\r')return false;
 size_t first=start;while(first<end&&b[first]==' ')++first;
 while(end>first&&b[end-1]==' ')--end;
 s={(uint32_t)first,(uint32_t)(end-first)};return true;
}
bool equal(const uint8_t*b,const OriginalLabelSpan&s,const char*t){size_t n=0;while(t[n])++n;if(s.length!=n)return false;for(size_t i=0;i<n;++i)if(b[s.offset+i]!=(uint8_t)t[i])return false;return true;}
E lookup(const OriginalLabelsView&v,uint32_t id,OriginalLabelSpan&s){
 if(!v.bytes||v.size>0xffffffffu||v.count>2048||id>=v.count)return E::missing_label;
 s=v.labels[id];if(s.offset>v.size||s.length>v.size-s.offset||s.length>510)return E::bad_input;
 for(uint32_t i=0;i<s.length;++i)if(!v.bytes[s.offset+i]||v.bytes[s.offset+i]=='\r'||v.bytes[s.offset+i]=='\n')return E::bad_input;
 if(s.length&&(v.bytes[s.offset]==' '||v.bytes[s.offset+s.length-1]==' '))return E::bad_input;
 return E::ok;
}
}
CombatLabelError import_original_labels(const uint8_t*b,size_t n,Variant edition,OriginalLabelsView&out){
 if(!b||!n||n>0xffffffffu||(uint32_t)edition>1||overlap(b,n,&out,sizeof out))return E::bad_input;
 OriginalLabelsView next{};next.bytes=b;next.size=n;next.edition=edition;size_t pos=0;OriginalLabelSpan s{};bool found=false;
 while(pos<n){if(!line(b,n,pos,s))return E::unsupported;if(equal(b,s,"#LABELS")){found=true;break;}if(s.length&&b[s.offset]!=';')return E::unsupported;}
 if(!found||!line(b,n,pos,s)||!s.length)return E::unsupported;
 uint32_t count=0;for(uint32_t i=0;i<s.length;++i){const uint8_t c=b[s.offset+i];if(c<'0'||c>'9')return E::unsupported;count=count*10+(c-'0');if(count>2048)return E::unsupported;}
 if(!count)return E::unsupported;
 next.count=count;
 for(uint32_t i=0;i<count;++i)if(!line(b,n,pos,next.labels[i]))return E::unsupported;
 /* The original loader stops after the declared rows. The remainder is not a
  * second section to interpret; no comments are stripped from actual labels. */
 out=next;return E::ok;
}
CombatLabelError resolve_combat_reporting(const OriginalLabelsView&v,Variant edition,const WorldBattleReportingInspection&r,const CombatTextLedger&prior,CombatTextReporting&out){
 if((uint32_t)edition>1||(uint32_t)v.edition>1||v.edition!=edition)return E::unsupported;
 const void*inputs[]={&v,&r,&prior};const size_t sizes[]={sizeof v,sizeof r,sizeof prior};
 for(unsigned i=0;i<3;++i){if(overlap(inputs[i],sizes[i],&out,sizeof out)||overlap(inputs[i],sizes[i],v.bytes,v.size))return E::bad_input;for(unsigned j=0;j<i;++j)if(overlap(inputs[i],sizes[i],inputs[j],sizes[j]))return E::bad_input;}
 if(overlap(&out,sizeof out,v.bytes,v.size)||!v.bytes||!v.size||v.size>0xffffffffu||!v.count||v.count>2048)return E::bad_input;
 if(r.modifiers.lookup_count>13||r.modifiers.ledger.count>4||r.final_ledger.count>4||r.entry.hasty.string_lookup_required>1||r.entry.hasty.report_append_required>1)return E::bad_input;
 CombatTextReporting next{};next.ledger=prior;next.ledger.count=0;
 auto visit=[&](uint32_t id,int32_t percent,uint32_t append)->E{
  OriginalLabelSpan s{};E e=lookup(v,id,s);if(e!=E::ok)return e;
  ++next.resolved_lookups;const uint32_t expected=percent&&next.ledger.count<4;
  if(append!=expected)return E::bad_input;
  if(append){if(s.length>=80)return E::text_too_long;const uint32_t slot=next.ledger.count++;next.ledger.percentages[slot]=percent;for(uint32_t i=0;i<s.length;++i)next.ledger.names[slot][i]=(char)v.bytes[s.offset+i];next.ledger.names[slot][s.length]=0;}
  return E::ok;
 };
 for(uint32_t i=0;i<r.modifiers.lookup_count;++i){const auto&e=r.modifiers.lookups[i];const uint32_t slot=next.ledger.count;E result=visit(e.label,e.percent,e.appended);if(result!=E::ok)return result;if(e.appended&&(r.modifiers.ledger.slots[slot].label!=e.label||r.modifiers.ledger.slots[slot].percent!=e.percent))return E::bad_input;}
 if(next.ledger.count!=r.modifiers.ledger.count)return E::bad_input;
 const auto&h=r.entry.hasty;
 if(h.string_lookup_required){E e=visit(339,h.penalty,h.report_append_required);if(e!=E::ok)return e;}else if(h.report_append_required)return E::bad_input;
 if(h.report_count!=(int32_t)next.ledger.count||r.final_ledger.count!=next.ledger.count||r.lookup_count!=next.resolved_lookups)return E::bad_input;
 for(uint32_t i=0;i<next.ledger.count;++i){const auto&s=r.final_ledger.slots[i];if(s.percent!=next.ledger.percentages[i])return E::bad_input;
  uint32_t id=339;uint32_t seen=0;for(uint32_t j=0;j<r.modifiers.lookup_count;++j)if(r.modifiers.lookups[j].appended){if(seen++==i)id=r.modifiers.lookups[j].label;}if(s.label!=id)return E::bad_input;}
 out=next;return E::ok;
}
}
