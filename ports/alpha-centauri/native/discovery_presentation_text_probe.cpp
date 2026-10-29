/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "discovery_presentation_text.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct Facts {int32_t faction,tech;alpha::DiscoveryReactorFacts runtime;alpha::PrototypeNameLabels labels;alpha::PrototypeNameSuffix suffix;};
static alpha::Rules tech;static alpha::UnitDefinitions defs;
static uint32_t get(const uint8_t* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static bool emit(uint32_t n){uint8_t b[4];for(unsigned i=0;i<4;++i){b[i]=(uint8_t)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
static uint8_t* read(const char* path,size_t& n){FILE* f=fopen(path,"rb");if(!f)return nullptr;fseek(f,0,SEEK_END);long k=ftell(f);rewind(f);if(k<=0 || k>64*1024*1024){fclose(f);return nullptr;}auto* b=(uint8_t*)malloc((size_t)k);if(!b){fclose(f);return nullptr;}n=(size_t)k;if(fread(b,1,n,f)!=n){free(b);b=nullptr;}fclose(f);return b;}
static uint32_t whole(const alpha::OriginalWorld& w,const Facts& q){uint32_t h=2166136261u;auto add=[&](const void* data,size_t n){const auto* p=(const uint8_t*)data;for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;};add(&w,sizeof w);add(w.original_bytes(),w.original_size());add(w.game().bunglist.data,w.game().bunglist.size);add(w.current_monuments().data,w.current_monuments().size);add(w.tile(0),w.game().map.info.tile_count*sizeof(alpha::OriginalMapTile));add(w.vehicle(0),w.game().vehicle_count*sizeof(alpha::OriginalVehicle));add(w.base(0),w.game().base_count*sizeof(alpha::OriginalBase));add(w.prototype(0),512*sizeof(alpha::OriginalPrototype));add(w.faction(0),8*sizeof(alpha::OriginalFactionState));add(w.faction_definition(0),8*sizeof(alpha::OriginalFactionDefinition));add(&tech,sizeof tech);add(&defs,sizeof defs);add(&q,sizeof q);return h;}
static uint32_t stage_hash(const alpha::DiscoveryReactorSequencePlan& p){uint32_t h=2166136261u;auto add=[&](const void* a,size_t n){const auto* b=(const uint8_t*)a;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;};add(&p,sizeof p);if(p.ready()){add(p.prototype(0),512*sizeof(alpha::OriginalPrototype));add(p.base(0),p.base_count()*sizeof(alpha::OriginalBase));add(p.faction(0),8*sizeof(alpha::OriginalFactionState));add(p.history().data,p.history().size);add(p.wire_baseline().data,p.wire_baseline().size);add(p.monuments().data,p.monuments().size);add(p.tile(0),p.map_info().tile_count*sizeof(alpha::OriginalMapTile));add(p.vehicle(0),p.vehicle_count()*sizeof(alpha::OriginalVehicle));add(p.faction_definition(0),8*sizeof(alpha::OriginalFactionDefinition));add(&p.preferences(),sizeof p.preferences());}return h;}

static uint32_t plan_hash(const alpha::DiscoveryContinuationPlan& p){uint32_t h=stage_hash(p.state());const auto* b=(const uint8_t*)&p;for(size_t i=0;i<sizeof p;++i)h=(h^b[i])*16777619u;return h;}
int main(int argc,char** argv){if(argc!=6)return 20;size_t n;auto* b=read(argv[2],n);if(!b)return 20;
 if(alpha::read_technologies(b,n,tech)!=alpha::Error::ok || alpha::read_unit_definitions(b,n,tech,alpha::Variant::base,defs)!=alpha::Error::ok)return 20;
 free(b);
 b=read(argv[1],n);if(!b)return 20;alpha::OriginalWorld w;if(w.load(b,n,123)!=alpha::WorldLoadError::ok)return 20;
 free(b);
 b=read(argv[3],n);if(!b || n!=1108)return 20;Facts q{};q.faction=(int32_t)get(b);q.tech=(int32_t)get(b+4);q.runtime.allocation.multiplayer=(alpha::PruningRuntime)get(b+8);q.runtime.allocation.cursor=(alpha::AllocationCursorObservation)get(b+12);q.runtime.allocation.last_created=(int32_t)get(b+16);q.runtime.allocation.difficulty=(int32_t)get(b+20);q.runtime.proposal_mode=(alpha::ProposalMode)get(b+24);q.runtime.expansion=(alpha::ExpansionObservation)get(b+28);q.runtime.runtime=(alpha::DiscoveryRuntimeObservation)get(b+32);q.runtime.skip=(alpha::SkipTechnologyScreenObservation)get(b+36);q.runtime.map_owner=(int32_t)get(b+40);q.runtime.upgrade_scratch=(int32_t)get(b+44);memcpy(&q.labels,b+48,896);memcpy(&q.suffix,b+944,128);
 alpha::DiscoveryReactorBoundaryFacts cache{(alpha::DiscoveryRuntimeObservation)get(b+1072),(int32_t)get(b+1076),(int32_t)get(b+1080)};
 alpha::DiscoveryContinuationFacts runtime{(alpha::DiscoveryRuntimeObservation)get(b+1084),(int32_t)get(b+1088),(int32_t)get(b+1092),(int32_t)get(b+1096),(int32_t)get(b+1100),(int32_t)get(b+1104)};free(b);const auto originalCache=cache;const auto originalRuntime=runtime;
 b=read(argv[5],n);if(!b || n!=3036)return 20;
 alpha::DiscoveryTextRuntime textRuntime{};textRuntime.runtime=(alpha::DiscoveryRuntimeObservation)get(b);memcpy(textRuntime.parser.strings,b+4,2560);size_t at=2564;
 for(unsigned i=0;i<10;++i){textRuntime.parser.genders[i]=(int32_t)get(b+at);at+=4;}
 for(unsigned i=0;i<10;++i){textRuntime.parser.plurals[i]=(int32_t)get(b+at);at+=4;}
 textRuntime.parser.default_gender=(int32_t)get(b+at);textRuntime.parser.default_plural=(int32_t)get(b+at+4);at+=8;
 memcpy(textRuntime.script_file,b+at,256);at+=256;char adjective[128];memcpy(adjective,b+at,128);free(b);
 // Controlled fixture only: bind current decoded adjective distinct from wire.
 memcpy(const_cast<alpha::OriginalFactionDefinition*>(w.faction_definition((size_t)q.faction))->adj_name_faction,adjective,128);
 const auto originalTextRuntime=textRuntime;
 const auto before=whole(w,q);alpha::DiscoveryContinuationPlan plan;
 auto call=[&](const alpha::DiscoveryContinuationFacts& fact,alpha::DiscoveryContinuationPlan& result,size_t limit=64u*1024u*1024u){return alpha::world_discovery_notice_continuation(w,alpha::Variant::base,tech,defs,q.faction,q.tech,q.runtime,q.labels,q.suffix,cache,fact,result,limit);};
 if(call(runtime,plan)!=alpha::DiscoveryContinuationError::ok || !plan.ready())return 21;
 b=read(argv[4],n);if(!b)return 20;uint32_t expectedHash=2166136261u;for(size_t i=0;i<n;++i)expectedHash=(expectedHash^b[i])*16777619u;
 alpha::OriginalWorld expected;if(expected.load(b,n,123)!=alpha::WorldLoadError::ok)return 20;
 free(b);const auto& s=plan.state();
 memcpy(const_cast<alpha::OriginalFactionDefinition*>(expected.faction_definition((size_t)q.faction))->adj_name_faction,adjective,128);
 auto mismatch=[&](){return !s.ready() || s.runtime_seed()!=w.runtime_seed() || s.wire_baseline().size!=w.original_size() || memcmp(s.wire_baseline().data,w.original_bytes(),w.original_size()) || memcmp(&s.preferences(),&expected.game().preferences,sizeof s.preferences()) || memcmp(&s.map_info(),&expected.game().map.info,sizeof s.map_info()) || memcmp(s.monuments().data,expected.current_monuments().data,expected.current_monuments().size) || memcmp(s.history().data,expected.game().bunglist.data,4096) || memcmp(s.prototype(0),expected.prototype(0),512*sizeof(alpha::OriginalPrototype)) || memcmp(s.base(0),expected.base(0),s.base_count()*sizeof(alpha::OriginalBase)) || memcmp(s.faction(0),expected.faction(0),8*sizeof(alpha::OriginalFactionState)) || memcmp(s.faction_definition(0),expected.faction_definition(0),8*sizeof(alpha::OriginalFactionDefinition)) || memcmp(s.vehicle(0),expected.vehicle(0),s.vehicle_count()*sizeof(alpha::OriginalVehicle)) || memcmp(s.tile(0),expected.tile(0),s.map_info().tile_count*sizeof(alpha::OriginalMapTile));};if(mismatch())return 22;
 if(!emit((uint32_t)plan.boundary()))return 20;
 alpha::DiscoverySharedRequest request{};if(plan.shared_request())request=*plan.shared_request();if(!emit((uint32_t)request.faction) || !emit((uint32_t)request.technology) || !emit((uint32_t)request.source) || !emit((uint32_t)request.share) || !emit((uint32_t)s.count()) || !emit((uint32_t)s.last_created()) || !emit((uint32_t)s.upgrade_scratch()) || !emit((uint32_t)s.first_ui_cache()) || !emit((uint32_t)s.second_ui_cache()) || !emit((uint32_t)s.pending_reactor()))return 20;
 char name[128]{};if(s.pending_reactor_name())memcpy(name,s.pending_reactor_name(),128);char noticeName[128]{};if(plan.pending_secrets_technology_name())memcpy(noticeName,plan.pending_secrets_technology_name(),128);
 if(fwrite(name,1,128,stdout)!=128 || fwrite(noticeName,1,128,stdout)!=128 || !emit(expectedHash))return 20;
 if(plan.map_owner()!=q.runtime.map_owner)return 21;
 alpha::DiscoveryPresentationText presentation;const auto e=alpha::discovery_presentation_text(plan,textRuntime,presentation);
 const bool supported=plan.boundary()==alpha::DiscoveryContinuationBoundary::reactor_presentation || plan.boundary()==alpha::DiscoveryContinuationBoundary::first_secrets_presentation;
 if(e!=(supported?alpha::DiscoveryTextError::ok:alpha::DiscoveryTextError::pending_kind_required) || presentation.ready()!=supported || !emit((uint32_t)e))return 21;
 auto text_hash=[&](){uint32_t h=2166136261u;const auto* bytes=(const uint8_t*)&presentation;for(size_t i=0;i<sizeof presentation;++i)h=(h^bytes[i])*16777619u;return h;};
 const auto heldText=text_hash();const auto heldPlan=plan_hash(plan);
 if(supported){const auto& tokens=presentation.parser();if(fwrite(tokens.strings,1,2560,stdout)!=2560)return 20;
  for(unsigned i=0;i<10;++i)if(!emit((uint32_t)tokens.genders[i]))return 20;
  for(unsigned i=0;i<10;++i)if(!emit((uint32_t)tokens.plurals[i]))return 20;
  if(!emit((uint32_t)tokens.default_gender) || !emit((uint32_t)tokens.default_plural))return 20;
  char key[16]{};char file[256]{};memcpy(key,presentation.popup_key(),strlen(presentation.popup_key())+1);memcpy(file,presentation.script_file(),strlen(presentation.script_file())+1);
  if(fwrite(key,1,16,stdout)!=16 || fwrite(file,1,256,stdout)!=256 || !emit((uint32_t)presentation.faction()) || !emit((uint32_t)presentation.technology()) || !emit((uint32_t)presentation.source_boundary()))return 20;
  auto badFile=textRuntime;memset(badFile.script_file,'X',256);if(alpha::discovery_presentation_text(plan,badFile,presentation)!=alpha::DiscoveryTextError::bad_state || text_hash()!=heldText)return 21;
  if(plan.boundary()==alpha::DiscoveryContinuationBoundary::first_secrets_presentation){auto* definition=const_cast<alpha::OriginalFactionDefinition*>(plan.state().faction_definition((size_t)q.faction));char saved[128];memcpy(saved,definition->adj_name_faction,128);memset(definition->adj_name_faction,'X',128);const auto corrupt=plan_hash(plan);const auto error=alpha::discovery_presentation_text(plan,textRuntime,presentation);if(error!=alpha::DiscoveryTextError::text_held || plan_hash(plan)!=corrupt || text_hash()!=heldText)return 21;memcpy(definition->adj_name_faction,saved,128);}
 }
 auto missingText=textRuntime;missingText.runtime=alpha::DiscoveryRuntimeObservation::unknown;if(alpha::discovery_presentation_text(plan,missingText,presentation)!=alpha::DiscoveryTextError::runtime_required || text_hash()!=heldText)return 21;
 const void* textAliases[]={&plan,&plan.state(),&plan.captured(),&textRuntime,&textRuntime.parser,s.wire_baseline().data,s.monuments().data,s.history().data,s.tile(0),s.vehicle(0),s.base(0),s.prototype(0),s.faction(0),s.faction_definition(0),s.pending_reactor_name(),plan.pending_secrets_technology_name()};
 for(const auto* a:textAliases)if(a){auto& fake=*(alpha::DiscoveryPresentationText*)(((uintptr_t)a+alignof(alpha::DiscoveryPresentationText)-1)&~(uintptr_t)(alignof(alpha::DiscoveryPresentationText)-1));if(alpha::discovery_presentation_text(plan,textRuntime,fake)!=alpha::DiscoveryTextError::bad_state || text_hash()!=heldText || plan_hash(plan)!=heldPlan)return 21;}
 if(memcmp(&textRuntime,&originalTextRuntime,sizeof textRuntime) || plan_hash(plan)!=heldPlan)return 21;
 if(memcmp(&plan.captured(),&runtime,sizeof runtime) || plan.faction()!=q.faction || plan.technology()!=q.tech)return 21;
 const auto held=plan_hash(plan);auto bad=runtime;bad.runtime=alpha::DiscoveryRuntimeObservation::unknown;if(call(bad,plan)!=alpha::DiscoveryContinuationError::runtime_required || plan_hash(plan)!=held)return 21;
 bad=runtime;bad.first_ever=2;if(call(bad,plan)!=alpha::DiscoveryContinuationError::bad_state || plan_hash(plan)!=held)return 21;
 if(call(runtime,plan,0)!=alpha::DiscoveryContinuationError::reactor_held || plan_hash(plan)!=held)return 21;
 const void* aliases[]={&w,&tech,&defs,&q,&q.runtime,&q.labels,&q.suffix,&cache,&runtime,w.original_bytes(),w.current_monuments().data,w.game().bunglist.data,w.tile(0),w.vehicle(0),w.base(0),w.prototype(0),w.faction(0),w.faction_definition(0)};
 for(const auto* a:aliases)if(a){auto& fake=*(alpha::DiscoveryContinuationPlan*)(((uintptr_t)a+alignof(alpha::DiscoveryContinuationPlan)-1)&~(uintptr_t)(alignof(alpha::DiscoveryContinuationPlan)-1));if(call(runtime,fake)!=alpha::DiscoveryContinuationError::bad_state || plan_hash(plan)!=held)return 21;}
 const void* storage[]={s.wire_baseline().data,s.monuments().data,s.history().data,s.tile(0),s.vehicle(0),s.base(0),s.prototype(0),s.faction(0),s.faction_definition(0),&s.preferences(),&plan.captured(),&plan.state()};
 for(const auto* a:storage)if(a){const auto& fake=*(const alpha::DiscoveryContinuationFacts*)a;if(call(fake,plan)!=alpha::DiscoveryContinuationError::bad_state || plan_hash(plan)!=held)return 21;}
 // Late recomputation overflow must not publish partially changed monuments,
 // map visibility or caches. These mutations are TEST FIXTURES, not APIs.
 if(plan.boundary()!=alpha::DiscoveryContinuationBoundary::reactor_presentation && plan.boundary()!=alpha::DiscoveryContinuationBoundary::generic_presentation){
  auto* definition=const_cast<alpha::OriginalFactionDefinition*>(w.faction_definition((size_t)q.faction));const int32_t saved=definition->rule_commerce;definition->rule_commerce=INT32_MAX;
  const auto e=call(runtime,plan);definition->rule_commerce=saved;if(e!=alpha::DiscoveryContinuationError::overflow || plan_hash(plan)!=held)return 21;
 }
 if(plan.pending_secrets_technology_name()){
  char saved[128];memcpy(saved,tech.technologies[q.tech].name,128);memset(tech.technologies[q.tech].name,'X',128);
  const auto e=call(runtime,plan);memcpy(tech.technologies[q.tech].name,saved,128);if(e!=alpha::DiscoveryContinuationError::bad_state || plan_hash(plan)!=held)return 21;
 }
 if(whole(w,q)!=before || memcmp(&cache,&originalCache,sizeof cache) || memcmp(&runtime,&originalRuntime,sizeof runtime))return 21;
 auto omni=runtime;omni.game_state=(int32_t)((uint32_t)omni.game_state|0x80u);
 if(call(omni,plan)!=alpha::DiscoveryContinuationError::ok || plan.has_shared_request() || plan.pending_secrets_technology_name() || (plan.boundary()!=alpha::DiscoveryContinuationBoundary::generic_presentation && plan.boundary()!=alpha::DiscoveryContinuationBoundary::reactor_presentation))return 21;
 if(call(runtime,plan)!=alpha::DiscoveryContinuationError::ok || mismatch())return 21;
 const auto replaced=plan_hash(plan);w.clear();expected.clear();return plan_hash(plan)==replaced?0:21;
}
