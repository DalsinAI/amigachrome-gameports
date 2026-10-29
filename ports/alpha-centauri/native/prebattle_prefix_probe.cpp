/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "prebattle_prefix.h"
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
static int32_t signed_word(uint32_t n){return n&0x80000000u?-(int32_t)(~n)-1:(int32_t)n;}
static bool get(uint32_t&n){unsigned char b[4];if(fread(b,1,4,stdin)!=4)return false;n=b[0]|uint32_t(b[1])<<8|uint32_t(b[2])<<16|uint32_t(b[3])<<24;return true;}
static void store(unsigned char*p,uint32_t n){for(unsigned i=0;i<4;++i){p[i]=(unsigned char)n;n>>=8;}}
static uint32_t planhash(const PrebattlePrefixPlan&p){if(!p.ready())return 0;uint32_t h=fingerprint(*p.frame());const auto*b=(const unsigned char*)p.summary();for(size_t i=0;i<sizeof(*p.summary());++i)h=(h^b[i])*16777619u;return h;}
static bool snapshot(const OriginalWorld&w,const OriginalWorld&f,size_t a,size_t b){
 const auto&g=w.game();const auto&h=f.game();if(f.runtime_seed()!=w.runtime_seed()||f.original_size()!=w.original_size()||memcmp(f.original_bytes(),w.original_bytes(),w.original_size())||h.vehicle_count!=g.vehicle_count||h.base_count!=g.base_count)return false;
 if(memcmp(f.tile(0),w.tile(0),g.map.info.tile_count*sizeof(OriginalMapTile))||memcmp(f.prototype(0),w.prototype(0),512*sizeof(OriginalPrototype))||memcmp(f.faction_definition(0),w.faction_definition(0),8*sizeof(OriginalFactionDefinition))||(g.base_count&&memcmp(f.base(0),w.base(0),g.base_count*sizeof(OriginalBase)))||memcmp(f.current_monuments().data,w.current_monuments().data,0x27a0)||memcmp(h.bunglist.data,g.bunglist.data,4096))return false;
 for(size_t i=0;i<g.vehicle_count;++i){OriginalVehicle v;memcpy(&v,w.vehicle(i),sizeof v);if(i==a){v.state=f.vehicle(i)->state;v.moves_spent=f.vehicle(i)->moves_spent;}if(i==b)v.state=f.vehicle(i)->state;if(memcmp(&v,f.vehicle(i),sizeof v))return false;}
 for(size_t i=0;i<8;++i){OriginalFactionState v;memcpy(&v,w.faction(i),sizeof v);const auto*x=f.faction(i);for(unsigned j=0;j<8;++j){v.diplo_status[j]=x->diplo_status[j];v.diplo_combat_count[j]=x->diplo_combat_count[j];v.diplo_spoke[j]=x->diplo_spoke[j];v.diplo_friction[j]=x->diplo_friction[j];}if(memcmp(&v,x,sizeof v))return false;}
 auto*raw=(unsigned char*)malloc(w.original_size());if(!raw)return false;memcpy(raw,w.original_bytes(),w.original_size());const size_t vp=(size_t)(g.vehicles.data-w.original_bytes()),fp=(size_t)(g.factions.data-w.original_bytes()),stride=g.factions.size/8;
 for(size_t i=0;i<g.vehicle_count;++i){store(raw+vp+52*i+4,f.vehicle(i)->state);raw[vp+52*i+40]=f.vehicle(i)->moves_spent;}
 for(size_t i=0;i<8;++i)for(unsigned j=0;j<8;++j){const auto*x=f.faction(i);store(raw+fp+stride*i+0x18+4*j,x->diplo_status[j]);store(raw+fp+stride*i+0x58+4*j,(uint32_t)x->diplo_friction[j]);store(raw+fp+stride*i+0x78+4*j,(uint32_t)x->diplo_spoke[j]);store(raw+fp+stride*i+0x1ac+4*j,(uint32_t)x->diplo_combat_count[j]);}
 const bool ok=word((uint32_t)w.original_size())&&fwrite(raw,1,w.original_size(),stdout)==w.original_size();free(raw);return ok;
}
int main(int argc,char**argv){
 if(argc!=4)return 20;
 size_t nt=0,n=0,nl=0;auto*text=read(argv[1],nt);auto*data=read(argv[2],n);auto*label_bytes=read(argv[3],nl);if(!text||!data||!label_bytes)return 20;
 auto*world=new OriginalWorld;auto&w=*world;if(w.load(data,n,0x12345678)!=WorldLoadError::ok)return 20;auto ed=w.game().data_version<11?Variant::base:Variant::alien_crossfire;
 if(read_technologies(text,nt,t)!=Error::ok||read_unit_definitions(text,nt,t,ed,d)!=Error::ok||read_basic_rules(text,nt,t,ed,r,wr)!=Error::ok)return 20;
 OriginalLabelsView labels{};if(import_original_labels(label_bytes,nl,ed,labels)!=CombatLabelError::ok)return 20;
 CombatTextLedger prior_text;memset(&prior_text,0x5a,sizeof prior_text);prior_text.count=17;
 const auto sourcehash=fingerprint(w);PrebattlePrefixPlan plan;uint32_t words[17];unsigned accepted=0,guards=0;
 while(get(words[0])){
  for(unsigned i=1;i<17;++i)if(!get(words[i]))return 21;
  const size_t a=words[0],b=words[1];const PrebattlePrefixRuntime run{(BattleEntryObservation)words[2],2,0,1,0,words[3],words[4],words[5],words[6],words[7],words[8],words[9],words[10],words[11],words[12]};
  auto actual=run;actual.attack_flags=words[13];actual.mode=signed_word(words[14]);actual.execution_argument=signed_word(words[15]);actual.multiplayer_active=words[16];
  const auto oldhash=planhash(plan);const bool wasready=plan.ready();const auto before=actual;
  const auto error=stage_prebattle_prefix(w,d,t,r,a,b,actual,prior,labels,prior_text,plan);
  if(memcmp(&actual,&before,sizeof actual)||fingerprint(w)!=sourcehash)return 22;
  if(error!=PrefixError::ok&&(plan.ready()!=wasready||planhash(plan)!=oldhash))return 23;
  const auto*s=error==PrefixError::ok?plan.summary():nullptr;const auto*e=s?&s->reporting.numeric.entry:nullptr;
  uint32_t row[20]{};row[0]=(uint32_t)error;if(e){row[1]=(uint32_t)e->before_hasty.offense;row[2]=(uint32_t)e->before_hasty.defense;row[3]=(uint32_t)e->before_hasty.offense_morale;row[4]=(uint32_t)e->before_hasty.defense_morale;row[5]=e->before_hasty.terrain;row[6]=e->before_hasty.defense_multiplier;row[7]=e->before_hasty.sensor_coverage;row[8]=e->speed;row[9]=(uint32_t)e->charge.charge;row[10]=e->charge.speed_used;row[11]=e->hasty.attacker_state;row[12]=e->hasty.defender_state;row[13]=e->hasty.moves_spent;row[14]=e->hasty.scaled;row[15]=(uint32_t)e->hasty.offense;row[16]=(uint32_t)e->hasty.penalty;row[17]=(uint32_t)e->hasty.report_count;row[18]=e->hasty.string_lookup_required;row[19]=e->hasty.report_append_required;}
  for(auto x:row)word(x);
  report(s?&s->reporting.numeric:nullptr);word(s?s->reporting.text.ledger.count:0);for(unsigned i=0;i<4;++i)word(s?(uint32_t)s->reporting.text.ledger.percentages[i]:0);for(unsigned i=0;i<4;++i)for(unsigned j=0;j<80;++j)fputc(s?(unsigned char)s->reporting.text.ledger.names[i][j]:0,stdout);word(s?s->reporting.text.resolved_lookups:0);
  word(s?(uint32_t)s->notification.route:0);word(s?s->notification.warning:0);word(s?s->notification.spoke:0);word(s?s->aggression.forward_status:0);word(s?s->aggression.reverse_status:0);word(s?(uint32_t)s->aggression.forward_combat_count:0);word(s?(uint32_t)s->aggression.reverse_combat_count:0);word(s?s->aggression.combat_counts_reset:0);
  word(s?s->territory.tile_index:0);word(s?(uint32_t)s->territory.owner:0);word(s?s->territory.attacker:0);word(s?s->territory.defender:0);word(s?s->territory.effect.owner_defender_status:0);word(s?s->territory.effect.defender_owner_status:0);word(s?(uint32_t)s->territory.effect.defender_owner_friction:0);word(s?s->territory.effect.negotiation_accumulator:0);word(s?s->territory.effect.treaty_applied:0);word(s?s->territory.effect.friction_applied:0);
  if(!s){word(0);continue;}++accepted;
  if(!snapshot(w,*plan.frame(),a,b))return 24;
  const auto afterhash=planhash(plan);
  for(size_t limit: {size_t(0),size_t(1),size_t(4096)}){if(stage_prebattle_prefix(w,d,t,r,a,b,actual,prior,labels,prior_text,plan,limit)!=PrefixError::out_of_memory||planhash(plan)!=afterhash)return 25;++guards;}
  const auto stable=planhash(plan);const auto oldsummary=*plan.summary();
  const uintptr_t addresses[]={(uintptr_t)&w,(uintptr_t)w.original_bytes(),(uintptr_t)w.vehicle(0),(uintptr_t)w.faction(0),(uintptr_t)&d,(uintptr_t)&t,(uintptr_t)&r,(uintptr_t)&actual,(uintptr_t)&prior,(uintptr_t)&labels,(uintptr_t)&prior_text,(uintptr_t)label_bytes};
  for(auto p:addresses)if(stage_prebattle_prefix(w,d,t,r,a,b,actual,prior,labels,prior_text,at<PrebattlePrefixPlan>((p+3)&~uintptr_t(3)))!=PrefixError::bad_state)return 26;
  guards+=12;
  if(stage_prebattle_prefix(w,d,t,r,a,b,at<PrebattlePrefixRuntime>((uintptr_t)plan.frame()->faction(0)),prior,labels,prior_text,plan)!=PrefixError::bad_state||planhash(plan)!=stable||memcmp(plan.summary(),&oldsummary,sizeof oldsummary))return 27;
  ++guards;
 }
 if(fingerprint(w)!=sourcehash)return 28;
 const auto owned=planhash(plan);delete world;free(data);free(text);free(label_bytes);if(planhash(plan)!=owned)return 28;
 fprintf(stderr,"ACCEPTED %u GUARDS %u\n",accepted,guards);return ferror(stdin)||ferror(stdout)?29:0;
}
