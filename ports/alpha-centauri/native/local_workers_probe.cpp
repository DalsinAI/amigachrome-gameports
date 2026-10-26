/* SPDX-License-Identifier: GPL-3.0-or-later. Test-only transaction harness. */
#include "local_production_session.h"
#include "local_workers_snapshot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>
static uint8_t* read(const char* path,size_t&n){FILE*f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long s=ftell(f);rewind(f);if(s<=0 || s>64*1024*1024){fclose(f);return nullptr;}auto*p=(uint8_t*)malloc((size_t)s);if(!p){fclose(f);return nullptr;}bool ok=fread(p,1,(size_t)s,f)==(size_t)s;fclose(f);if(!ok){free(p);return nullptr;}n=(size_t)s;return p;}
static alpha::WorkerAccountingReceipt& receipt_at(void* p){return *static_cast<alpha::WorkerAccountingReceipt*>(p);}
static bool emit(uint32_t n){uint8_t b[4];for(unsigned i=0;i<4;++i){b[i]=(uint8_t)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
#define REQUIRE(x) do{if(!(x)){fprintf(stderr,"failed line %d\n",__LINE__);return 21;}++guards;}while(0)
int main(int argc,char**argv){
 using namespace alpha;if(argc!=4)return 20;unsigned guards=0;size_t nt,ns;auto* text=read(argv[2],nt);auto* save=read(argv[3],ns);if(!text||!save)return 20;
 OriginalGameView view;if(read_original_game(save,ns,view)!=GameReadError::ok)return 20;
 Variant v=!strcmp(argv[1],"base")?Variant::base:Variant::alien_crossfire;bool expansion=v==Variant::alien_crossfire;int32_t human=(int32_t)view.player;
 WorldEnergyStage stage{{{v,expansion,false,{0,false},0,0},true,0},PoliceSnapshotProfile::offline_validated,CommerceEnergyStage::current_first_field_fresh_technology,0,AlienGridPolicy::reject_uninitialized};
 LocalProductionSession s;const auto load=s.load_local_accounting(save,ns,text,nt,v,expansion,human,0xabcdef01);
 if(!emit((uint32_t)load))return 21;
 if(load!=LocalOrderError::ok){free(save);free(text);return emit(guards)?0:21;}
 REQUIRE(s.accounting_enabled()&&!s.land_actions_enabled());const auto rev=s.revision();
 REQUIRE(s.load_local_accounting(save,ns,text,nt,v,expansion,human,1,0)==LocalOrderError::no_memory&&s.revision()==rev);
 REQUIRE(s.load_local_accounting(save,ns,(const uint8_t*)"#BAD",4,v,expansion,human,1)==LocalOrderError::bad_rules&&s.revision()==rev);
 WorkerAccountingPlan plan;const auto prep=s.prepare_workers(0,stage,plan);if(!emit((uint32_t)prep))return 21;
 WorkerAccountingPlan unissued;WorkerAccountingReceipt receipt,old_receipt;memset(&receipt,0x5a,sizeof receipt);memcpy(&old_receipt,&receipt,sizeof receipt);
 REQUIRE(s.commit_workers(unissued,receipt)==LocalOrderError::stale_plan&&!memcmp(&receipt,&old_receipt,sizeof receipt));
 auto badstage=stage;badstage.upkeep_stage=1;WorkerAccountingPlan untouched,before;memcpy(&before,&untouched,sizeof before);
 REQUIRE(s.prepare_workers(0,badstage,untouched)==LocalOrderError::unsupported&&!memcmp(&untouched,&before,sizeof before));
 REQUIRE(s.prepare_workers(view.base_count,stage,untouched)==LocalOrderError::bad_state&&!memcmp(&untouched,&before,sizeof before));
 if(prep!=LocalOrderError::ok){REQUIRE(s.revision()==rev&&!memcmp(s.world()->original_bytes(),save,ns));free(save);free(text);return emit(guards)?0:21;}
 REQUIRE(s.prepare_workers(0,stage,untouched,0)==LocalOrderError::no_memory&&!memcmp(&untouched,&before,sizeof before));
 const void* aliases[]={s.world(),s.world()->original_bytes(),s.world()->current_monuments().data,s.world()->tile(0),s.world()->base(0),s.world()->vehicle(0),s.world()->prototype(0),s.world()->faction(0),s.world()->faction_definition(0),&s,&stage};
 for(const void* at:aliases)if(at){REQUIRE(s.prepare_workers(0,stage,*reinterpret_cast<WorkerAccountingPlan*>(const_cast<void*>(at)))==LocalOrderError::bad_state);}
 for(const void* at:aliases)if(at && at!=&stage){REQUIRE(s.commit_workers(plan,*reinterpret_cast<WorkerAccountingReceipt*>(const_cast<void*>(at)))==LocalOrderError::bad_state);}
 REQUIRE(s.commit_workers(plan,receipt_at(&plan))==LocalOrderError::bad_state);
 LocalProductionSession other;REQUIRE(other.load_local_accounting(save,ns,text,nt,v,expansion,human,0xabcdef01)==LocalOrderError::ok);
 REQUIRE(other.commit_workers(plan,receipt)==LocalOrderError::stale_plan);
 REQUIRE(s.set_suspended(true)==LocalOrderError::ok);
 REQUIRE(s.commit_workers(plan,receipt)==LocalOrderError::suspended&&!memcmp(&receipt,&old_receipt,sizeof receipt));
 REQUIRE(s.set_suspended(false)==LocalOrderError::ok);
 REQUIRE(s.commit_workers(plan,receipt)==LocalOrderError::stale_plan);
 REQUIRE(s.prepare_workers(0,stage,plan)==LocalOrderError::ok);
 REQUIRE(s.commit_workers(plan,receipt)==LocalOrderError::ok);
 REQUIRE(receipt.base==0&&receipt.revision==s.revision()&&receipt.repaint_required==receipt.changed&&receipt.persistence_required==receipt.changed);
 const bool changed=receipt.changed;const auto committed_revision=s.revision();
 if(changed)REQUIRE(s.commit_workers(plan,receipt)==LocalOrderError::stale_plan);
 REQUIRE(s.world()->runtime_seed()==0xabcdef01&&!memcmp(s.world()->original_bytes(),save,ns));
 REQUIRE(s.prepare_workers(0,stage,plan)==LocalOrderError::ok);
 REQUIRE(s.commit_workers(plan,receipt)==LocalOrderError::ok&&!receipt.changed&&s.revision()==committed_revision);
 auto* output=(uint8_t*)malloc(ns+16);auto* baseline=(uint8_t*)malloc(ns+16);if(!output||!baseline)return 20;
 memset(output,0x5a,ns+16);memcpy(baseline,output,ns+16);LocalSnapshotReceipt sr,old;memset(&sr,0x5a,sizeof sr);memcpy(&old,&sr,sizeof sr);
 REQUIRE(encode_local_workers_snapshot(*s.world(),output,ns-1,sr)==LocalSnapshotError::bad_buffer&&!memcmp(output,baseline,ns+16)&&!memcmp(&sr,&old,sizeof sr));
 REQUIRE(encode_local_workers_snapshot(*s.world(),output,ns,*(LocalSnapshotReceipt*)output)==LocalSnapshotError::bad_buffer&&!memcmp(output,baseline,ns+16));
 REQUIRE(encode_local_workers_snapshot(*s.world(),(void*)s.world()->original_bytes(),ns,sr)==LocalSnapshotError::bad_buffer&&!memcmp(s.world()->original_bytes(),save,ns));
 REQUIRE(encode_local_workers_snapshot(*s.world(),output,ns,sr)==LocalSnapshotError::ok&&sr.bytes==ns&&sr.runtime_seed==0xabcdef01&&!memcmp(output+ns,baseline+ns,16));
 REQUIRE(other.load_local_accounting(output,ns,text,nt,v,expansion,human,sr.runtime_seed)==LocalOrderError::ok);
 REQUIRE(other.prepare_workers(0,stage,plan)==LocalOrderError::ok&&other.commit_workers(plan,receipt)==LocalOrderError::ok&&!receipt.changed);
 REQUIRE(encode_local_workers_snapshot(*other.world(),baseline,ns,sr)==LocalSnapshotError::ok&&!memcmp(output,baseline,ns));
 // Reload and placement-address reuse invalidate commands even with identical bytes.
 REQUIRE(s.prepare_workers(0,stage,plan)==LocalOrderError::ok);
 REQUIRE(s.load_local_accounting(save,ns,text,nt,v,expansion,human,0xabcdef01)==LocalOrderError::ok&&s.commit_workers(plan,receipt)==LocalOrderError::stale_plan);
 alignas(LocalProductionSession) uint8_t space[sizeof(LocalProductionSession)];auto* first=new(space)LocalProductionSession;
 REQUIRE(first->load_local_accounting(save,ns,text,nt,v,expansion,human,0xabcdef01)==LocalOrderError::ok&&first->prepare_workers(0,stage,plan)==LocalOrderError::ok);first->~LocalProductionSession();
 first=new(space)LocalProductionSession;REQUIRE(first->load_local_accounting(save,ns,text,nt,v,expansion,human,0xabcdef01)==LocalOrderError::ok&&first->commit_workers(plan,receipt)==LocalOrderError::stale_plan);first->~LocalProductionSession();
 if(!emit(changed)||!emit(guards)||fwrite(output,1,ns,stdout)!=ns)return 21;
 free(output);free(baseline);free(save);free(text);return 0;
}
