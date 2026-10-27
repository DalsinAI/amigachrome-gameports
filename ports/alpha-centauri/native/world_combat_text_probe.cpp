/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_combat_text.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <initializer_list>
using namespace alpha;
static const AttackModifierLedger prior{17,{{0x5a5a5a5a,0x5a5a5a5a},{0x5a5a5a5a,0x5a5a5a5a},{0x5a5a5a5a,0x5a5a5a5a},{0x5a5a5a5a,0x5a5a5a5a}}};
static UnitDefinitions d;static Rules t;static BasicRules r;static WorldRules wr;
static unsigned char*read(const char*p,size_t&n){FILE*f=fopen(p,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long z=ftell(f);rewind(f);if(z<1||z>64*1024*1024){fclose(f);return nullptr;}auto*b=(unsigned char*)malloc((size_t)z);if(!b){fclose(f);return nullptr;}bool ok=fread(b,1,(size_t)z,f)==(size_t)z;fclose(f);if(!ok){free(b);return nullptr;}n=(size_t)z;return b;}
static bool word(uint32_t n){for(unsigned i=0;i<4;++i){if(fputc((int)(n&255),stdout)==EOF)return false;n>>=8;}return true;}
template<class T>static T&at(uintptr_t x){return *reinterpret_cast<T*>(x);}
static uint32_t fingerprint(const OriginalWorld&w){
 uint32_t h=2166136261u;auto add=[&](const void*p,size_t n){const auto*b=(const unsigned char*)p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;};const auto&g=w.game();
 add(w.original_bytes(),w.original_size());add(w.tile(0),g.map.info.tile_count*sizeof(OriginalMapTile));add(w.vehicle(0),g.vehicle_count*sizeof(OriginalVehicle));add(w.base(0),g.base_count*sizeof(OriginalBase));add(w.prototype(0),512*sizeof(OriginalPrototype));add(w.faction(0),8*sizeof(OriginalFactionState));add(w.faction_definition(0),8*sizeof(OriginalFactionDefinition));add(w.current_monuments().data,w.current_monuments().size);add(g.bunglist.data,g.bunglist.size);return h;
}
static bool report(const WorldBattleReportingInspection*s){
 const auto*m=s?&s->modifiers:nullptr;
 if(!word(m?m->ledger.count:0))return false;
 for(unsigned i=0;i<4;++i)if(!word(m?m->ledger.slots[i].label:0)||!word(m?(uint32_t)m->ledger.slots[i].percent:0))return false;
 if(!word(m?m->lookup_count:0))return false;
 for(unsigned i=0;i<13;++i)if(!word(m?m->lookups[i].label:0)||!word(m?(uint32_t)m->lookups[i].percent:0)||!word(m?m->lookups[i].appended:0))return false;
 if(!word(s?s->final_ledger.count:0))return false;
 for(unsigned i=0;i<4;++i)if(!word(s?s->final_ledger.slots[i].label:0)||!word(s?(uint32_t)s->final_ledger.slots[i].percent:0))return false;
 return word(s?s->lookup_count:0);
}
int main(int argc,char**argv){
 if(argc!=4)return 20;
 size_t nt=0,n=0;auto*text=read(argv[1],nt);auto*data=read(argv[2],n);if(!text||!data)return 20;
 OriginalWorld w;if(w.load(data,n,0x12345678)!=WorldLoadError::ok)return 20;auto ed=w.game().data_version<11?Variant::base:Variant::alien_crossfire;
 if(read_technologies(text,nt,t)!=Error::ok||read_unit_definitions(text,nt,t,ed,d)!=Error::ok||read_basic_rules(text,nt,t,ed,r,wr)!=Error::ok)return 20;
 size_t nl=0;auto*label_bytes=read(argv[3],nl);if(!label_bytes)return 20;
 OriginalLabelsView labels{};if(import_original_labels(label_bytes,nl,ed,labels)!=CombatLabelError::ok)return 20;
 const auto original_labels=labels;auto*label_copy=(unsigned char*)malloc(nl);if(!label_copy)return 20;memcpy(label_copy,label_bytes,nl);
 CombatTextLedger prior_text;memset(&prior_text,0x5a,sizeof prior_text);prior_text.count=17;const auto original_prior=prior_text;
 const auto current_before=fingerprint(w);
 const size_t bytes=w.game().vehicle_count*sizeof(OriginalVehicle);auto*actors=(unsigned char*)malloc(bytes);if(bytes&&!actors)return 20;if(bytes)memcpy(actors,w.vehicle(0),bytes);
 WorldCombatTextInspection last{};WorldBattleReportingRuntime f{BattleEntryObservation::supplied,2,0,1,0};unsigned guards=0,accepted=0;
 for(size_t a=0;a<w.game().vehicle_count;++a)for(size_t b=0;b<w.game().vehicle_count;++b){
  WorldCombatTextInspection out;memset(&out,0xa5,sizeof out);unsigned char before[sizeof out];memcpy(before,&out,sizeof out);
  auto e=inspect_world_combat_text(w,d,t,r,a,b,f,prior,labels,prior_text,out);if(e!=WorldCombatTextError::ok&&memcmp(before,&out,sizeof out))return 21;
  uint32_t row[20]{};row[0]=(uint32_t)e;if(e==WorldCombatTextError::ok){++accepted;last=out;row[1]=(uint32_t)out.numeric.entry.before_hasty.offense;row[2]=(uint32_t)out.numeric.entry.before_hasty.defense;row[3]=(uint32_t)out.numeric.entry.before_hasty.offense_morale;row[4]=(uint32_t)out.numeric.entry.before_hasty.defense_morale;row[5]=out.numeric.entry.before_hasty.terrain;row[6]=out.numeric.entry.before_hasty.defense_multiplier;row[7]=out.numeric.entry.before_hasty.sensor_coverage;row[8]=out.numeric.entry.speed;row[9]=(uint32_t)out.numeric.entry.charge.charge;row[10]=out.numeric.entry.charge.speed_used;row[11]=out.numeric.entry.hasty.attacker_state;row[12]=out.numeric.entry.hasty.defender_state;row[13]=out.numeric.entry.hasty.moves_spent;row[14]=out.numeric.entry.hasty.scaled;row[15]=(uint32_t)out.numeric.entry.hasty.offense;row[16]=(uint32_t)out.numeric.entry.hasty.penalty;row[17]=(uint32_t)out.numeric.entry.hasty.report_count;row[18]=out.numeric.entry.hasty.string_lookup_required;row[19]=out.numeric.entry.hasty.report_append_required;}
  for(auto x:row)if(!word(x))return 22;
  if(!report(e==WorldCombatTextError::ok?&out.numeric:nullptr))return 22;
  const bool ok=e==WorldCombatTextError::ok;
  if(!word(ok?out.text.ledger.count:0))return 22;
  for(unsigned i=0;i<4;++i)if(!word(ok?(uint32_t)out.text.ledger.percentages[i]:0))return 22;
  for(unsigned i=0;i<4;++i)for(unsigned j=0;j<80;++j)if(fputc(ok?(unsigned char)out.text.ledger.names[i][j]:0,stdout)==EOF)return 22;
  if(!word(ok?out.text.resolved_lookups:0))return 22;
 }
 if(accepted){
  const uintptr_t addresses[]={(uintptr_t)&w,(uintptr_t)w.original_bytes(),(uintptr_t)w.tile(0),(uintptr_t)w.vehicle(0),(uintptr_t)w.base(0),(uintptr_t)w.prototype(0),(uintptr_t)w.faction(0),(uintptr_t)w.faction_definition(0),(uintptr_t)w.game().bunglist.data,(uintptr_t)&d,(uintptr_t)&t,(uintptr_t)&r,(uintptr_t)&f,(uintptr_t)&prior,(uintptr_t)&labels,(uintptr_t)&prior_text,(uintptr_t)label_bytes};
  for(auto x:addresses)if(x){const auto aligned=(x+alignof(WorldCombatTextInspection)-1)&~uintptr_t(alignof(WorldCombatTextInspection)-1);if(inspect_world_combat_text(w,d,t,r,0,1,f,prior,labels,prior_text,at<WorldCombatTextInspection>(aligned))!=WorldCombatTextError::bad_state)return 23;++guards;}
 }
 constexpr uintptr_t bad=4096;
 if(inspect_world_combat_text(w,at<UnitDefinitions>(bad),t,r,0,1,f,prior,labels,prior_text,at<WorldCombatTextInspection>(bad))!=WorldCombatTextError::bad_state||inspect_world_combat_text(w,d,at<Rules>(bad),r,0,1,f,prior,labels,prior_text,at<WorldCombatTextInspection>(bad))!=WorldCombatTextError::bad_state||inspect_world_combat_text(w,d,t,at<BasicRules>(bad),0,1,f,prior,labels,prior_text,at<WorldCombatTextInspection>(bad))!=WorldCombatTextError::bad_state||inspect_world_combat_text(w,d,t,r,0,1,at<WorldBattleReportingRuntime>(bad),prior,labels,prior_text,at<WorldCombatTextInspection>(bad))!=WorldCombatTextError::bad_state)return 24;
 if(inspect_world_combat_text(w,d,t,r,0,1,f,at<AttackModifierLedger>(bad),labels,prior_text,at<WorldCombatTextInspection>(bad))!=WorldCombatTextError::bad_state)return 24;
 if(inspect_world_combat_text(w,d,t,r,0,1,f,prior,at<OriginalLabelsView>(bad),prior_text,at<WorldCombatTextInspection>(bad))!=WorldCombatTextError::bad_state||inspect_world_combat_text(w,d,t,r,0,1,f,prior,labels,at<CombatTextLedger>(bad),at<WorldCombatTextInspection>(bad))!=WorldCombatTextError::bad_state)return 24;
 guards+=7;
 WorldCombatTextInspection output=last;
 for(unsigned k=0;k<10;++k){auto x=f;if(k==0)x.observation=BattleEntryObservation::missing;if(k==1)x.attack_flags=0;if(k==2)x.mode=2;if(k==3)x.execution_argument=0;if(k==4)x.multiplayer_active=1;if(k==5)x.multiplayer_active=2;if(k==6)x.attack_flags=4;if(k==7)x.mode=1;if(k==8)x.attack_flags=3;if(k==9)x.execution_argument=-1;if(inspect_world_combat_text(w,d,t,r,0,1,x,prior,labels,prior_text,output)==WorldCombatTextError::ok||memcmp(&last,&output,sizeof output))return 25;++guards;}
 for(unsigned i=0;i<5;++i){auto v=labels;if(i==0)v.edition=ed==Variant::base?Variant::alien_crossfire:Variant::base;if(i==1){v.bytes=w.original_bytes();v.size=w.original_size();}if(i==2)v.count=0;if(i==3)v.count=2049;if(i==4){v.bytes=(const uint8_t*)&r;v.size=sizeof r;}
  if(inspect_world_combat_text(w,d,t,r,0,1,f,prior,v,prior_text,output)==WorldCombatTextError::ok||memcmp(&last,&output,sizeof output))return 25;
  ++guards;}
 if(memcmp(&labels,&original_labels,sizeof labels)||memcmp(&prior_text,&original_prior,sizeof prior_text)||memcmp(label_bytes,label_copy,nl))return 26;
 if(w.runtime_seed()!=0x12345678||fingerprint(w)!=current_before||memcmp(w.original_bytes(),data,n)||(bytes&&memcmp(w.vehicle(0),actors,bytes)))return 26;
 if(!word(guards))return 22;
 free(label_copy);free(label_bytes);free(actors);free(data);free(text);return 0;
}
