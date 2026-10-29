/* SPDX-License-Identifier: GPL-3.0-or-later. Coherent bounded action/worker transaction test. */
#include "local_production_session.h"
#include "local_workers_snapshot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static uint8_t*read(const char*p,size_t&n){FILE*f=fopen(p,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long z=ftell(f);rewind(f);if(z<=0||z>64*1024*1024){fclose(f);return nullptr;}auto*b=(uint8_t*)malloc((size_t)z);if(!b){fclose(f);return nullptr;}bool ok=fread(b,1,(size_t)z,f)==(size_t)z;fclose(f);if(!ok){free(b);return nullptr;}n=(size_t)z;return b;}
static bool emit(uint32_t n){uint8_t b[4];for(unsigned i=0;i<4;++i){b[i]=(uint8_t)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"failed line %d\n",__LINE__);return 21;}++guards;}while(0)
int main(int argc,char**argv){if(argc!=3)return 20;size_t n,nt;auto*save=read(argv[2],n);auto*text=read(argv[1],nt);if(!save||!text)return 20;
 OriginalGameView view{};if(read_original_game(save,n,view)!=GameReadError::ok)return 20;const int human=(int)view.player;const size_t rover=view.vehicle_count-1;unsigned guards=0;
 LocalProductionSession s,other;CHECK(s.load_local_worker_actions(save,n,text,nt,Variant::base,false,human,0xabcdef01)==LocalOrderError::ok&&s.land_actions_enabled()&&s.accounting_enabled());
 const auto initial=s.revision();CHECK(s.load_local_worker_actions(save,n,text,nt,Variant::alien_crossfire,true,human,1)==LocalOrderError::unsupported&&s.revision()==initial);CHECK(s.load_local_worker_actions(save,n,text,nt,Variant::base,false,human,1,0)==LocalOrderError::no_memory&&s.revision()==initial);
 WorldEnergyStage stage{{{Variant::base,false,false,{0,false},0,0},true,0},PoliceSnapshotProfile::offline_validated,CommerceEnergyStage::current_first_field_fresh_technology,0,AlienGridPolicy::reject_uninitialized};
 WorkerAccountingPlan worker;FacilityOrderPlan facility;LocalStepCommand step;WorkerAccountingReceipt wr;FacilityOrderReceipt fr;LocalStepReceipt sr;
 CHECK(s.prepare_workers(0,stage,worker)==LocalOrderError::ok&&s.prepare_facility(0,1,facility)==LocalOrderError::ok&&s.prepare_clear_land_step(rover,3,3,step)==LocalOrderError::ok);
 CHECK(s.commit_clear_land_step(step,sr)==LocalOrderError::ok&&sr.changed&&sr.result.status==StepStatus::moved&&sr.result.price==3);
 CHECK(s.commit_workers(worker,wr)==LocalOrderError::stale_plan&&s.commit(facility,fr)==LocalOrderError::stale_plan);
 CHECK(s.prepare_workers(0,stage,worker)==LocalOrderError::ok&&s.prepare_facility(0,1,facility)==LocalOrderError::ok&&s.commit(facility,fr)==LocalOrderError::ok&&fr.changed);
 CHECK(s.commit_workers(worker,wr)==LocalOrderError::stale_plan);
 CHECK(s.prepare_clear_land_step(rover,2,2,step)==LocalOrderError::ok&&s.commit_clear_land_step(step,sr)==LocalOrderError::ok&&sr.changed&&sr.result.price==3);
 CHECK(s.prepare_facility(0,2,facility)==LocalOrderError::ok&&s.commit(facility,fr)==LocalOrderError::ok&&fr.changed);
 CHECK(s.prepare_workers(0,stage,worker)==LocalOrderError::ok&&s.prepare_clear_land_step(rover,3,3,step)==LocalOrderError::ok);const auto before=s.revision();
 CHECK(s.commit_clear_land_step(step,sr)==LocalOrderError::ok&&!sr.changed&&sr.result.status==StepStatus::exhausted&&s.revision()==before);
 CHECK(s.commit_workers(worker,wr)==LocalOrderError::ok&&wr.changed&&wr.revision==s.revision());
 auto*out=(uint8_t*)malloc(n);auto*again=(uint8_t*)malloc(n);if(!out||!again)return 20;LocalSnapshotReceipt snapshot{};
 CHECK(encode_local_workers_snapshot(*s.world(),out,n,snapshot)==LocalSnapshotError::ok&&snapshot.runtime_seed==0xabcdef01);
 CHECK(other.load_local_worker_actions(out,n,text,nt,Variant::base,false,human,snapshot.runtime_seed)==LocalOrderError::ok&&other.land_actions_enabled()&&other.accounting_enabled());
 CHECK(other.commit_workers(worker,wr)==LocalOrderError::stale_plan);
 CHECK(other.prepare_workers(0,stage,worker)==LocalOrderError::ok&&other.commit_workers(worker,wr)==LocalOrderError::ok&&!wr.changed);
 CHECK(encode_local_workers_snapshot(*other.world(),again,n,snapshot)==LocalSnapshotError::ok&&!memcmp(out,again,n));
 LocalProductionSession*sessions[]={&s,&other};
 for(auto*session:sessions){CHECK(session->prepare_facility(0,3,facility)==LocalOrderError::ok&&session->commit(facility,fr)==LocalOrderError::ok&&fr.changed);CHECK(session->prepare_workers(0,stage,worker)==LocalOrderError::ok&&session->commit_workers(worker,wr)==LocalOrderError::ok);CHECK(session->prepare_clear_land_step(rover,3,3,step)==LocalOrderError::ok&&session->commit_clear_land_step(step,sr)==LocalOrderError::ok&&!sr.changed&&sr.result.status==StepStatus::exhausted);}
 CHECK(encode_local_workers_snapshot(*s.world(),out,n,snapshot)==LocalSnapshotError::ok&&encode_local_workers_snapshot(*other.world(),again,n,snapshot)==LocalSnapshotError::ok&&!memcmp(out,again,n));
 CHECK(s.world()->runtime_seed()==0xabcdef01&&!memcmp(s.world()->original_bytes(),save,n));
 if(!emit(guards)||fwrite(out,1,n,stdout)!=n)return 20;
 free(out);free(again);free(save);free(text);return 0;
}
