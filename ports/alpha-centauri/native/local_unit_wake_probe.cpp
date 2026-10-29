/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "local_production_session.h"
#include "local_workers_snapshot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>
using namespace alpha;
static uint8_t*read(const char*p,size_t&n){FILE*f=fopen(p,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long z=ftell(f);rewind(f);if(z<=0||z>64*1024*1024){fclose(f);return nullptr;}auto*b=(uint8_t*)malloc((size_t)z);if(!b){fclose(f);return nullptr;}bool ok=fread(b,1,(size_t)z,f)==(size_t)z;fclose(f);if(!ok){free(b);return nullptr;}n=(size_t)z;return b;}
static bool emit(uint32_t n){uint8_t b[4];for(unsigned i=0;i<4;++i){b[i]=(uint8_t)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"failed line %d\n",__LINE__);return 21;}++guards;}while(0)

template<class T>static T& alias(uintptr_t p){return *reinterpret_cast<T*>(p);}
int main(int argc,char**argv){
 if(argc!=3)return 20;
 size_t n=0,nt=0;auto*save=read(argv[2],n);auto*text=read(argv[1],nt);if(!save||!text)return 20;
 OriginalGameView v{};if(read_original_game(save,n,v)!=GameReadError::ok || !v.vehicle_count)return 20;
 const auto profile=v.data_version<11?Variant::base:Variant::alien_crossfire;const size_t id=v.vehicle_count-1;unsigned guards=0;
 LocalProductionSession s,other,bare;CHECK(s.load_resumed_commands(save,n,text,nt,profile,profile==Variant::alien_crossfire,false,0xabcdef01)==LocalOrderError::ok);
 const auto baseline=s.world()->original_bytes();auto*before=(uint8_t*)malloc(n);auto*out=(uint8_t*)malloc(n);auto*again=(uint8_t*)malloc(n);if(!before||!out||!again)return 20;memcpy(before,baseline,n);
 LocalWakeCommand plan,unissued;LocalWakeReceipt receipt{};const auto rev=s.revision();
 CHECK(s.commit_wake_unit(unissued,receipt)==LocalOrderError::stale_plan);
 CHECK(s.prepare_wake_unit(v.vehicle_count,plan)==LocalOrderError::bad_state);
 CHECK(s.prepare_wake_unit(id,alias<LocalWakeCommand>((uintptr_t)baseline))==LocalOrderError::bad_state);
 CHECK(s.prepare_wake_unit(id,alias<LocalWakeCommand>((uintptr_t)&s))==LocalOrderError::bad_state);
 CHECK(bare.load_local(baseline,n,text,nt,profile,profile==Variant::alien_crossfire,s.local_human(),1)==LocalOrderError::ok&&bare.prepare_wake_unit(id,plan)==LocalOrderError::unsupported);
 const auto prepared=s.prepare_wake_unit(id,plan);
 if(prepared==LocalOrderError::overflow || prepared==LocalOrderError::not_owner){
  CHECK(s.revision()==rev&&!memcmp(before,baseline,n));LocalSnapshotReceipt snap{};CHECK(encode_local_workers_snapshot(*s.world(),out,n,snap)==LocalSnapshotError::ok&&!memcmp(out,before,n));
  CHECK(emit(guards)&&emit(prepared==LocalOrderError::overflow?1:2)&&fwrite(out,1,n,stdout)==n);free(before);free(out);free(again);free(save);free(text);return 0;
 }
 CHECK(prepared==LocalOrderError::ok);
 const auto preview=plan.preview();const bool changed=preview.spent_before!=preview.spent_after || preview.state_before!=preview.state_after || preview.turns_before!=preview.turns_after || preview.order_before!=0;
 CHECK(other.load_resumed_commands(save,n,text,nt,profile,profile==Variant::alien_crossfire,false,1)==LocalOrderError::ok&&other.commit_wake_unit(plan,receipt)==LocalOrderError::stale_plan);
 CHECK(s.commit_wake_unit(plan,alias<LocalWakeReceipt>((uintptr_t)&plan))==LocalOrderError::bad_state);
 CHECK(s.commit_wake_unit(plan,alias<LocalWakeReceipt>(((uintptr_t)s.world()->vehicle(id)+alignof(LocalWakeReceipt)-1)&~(uintptr_t)(alignof(LocalWakeReceipt)-1)))==LocalOrderError::bad_state);
 CHECK(s.set_suspended(true)==LocalOrderError::ok&&s.commit_wake_unit(plan,receipt)==LocalOrderError::suspended);
 CHECK(s.set_suspended(false)==LocalOrderError::ok&&s.commit_wake_unit(plan,receipt)==LocalOrderError::stale_plan);
 CHECK(s.prepare_wake_unit(id,plan)==LocalOrderError::ok);
 const auto active=s.revision();CHECK(s.commit_wake_unit(plan,receipt)==LocalOrderError::ok&&receipt.changed==changed&&receipt.repaint_required==changed&&receipt.persistence_required==changed&&s.revision()==active+(changed?1:0));
 CHECK(s.prepare_wake_unit(id,plan)==LocalOrderError::ok&&s.commit_wake_unit(plan,receipt)==LocalOrderError::ok&&!receipt.changed);
 CHECK(s.world()->runtime_seed()==0xabcdef01&&!memcmp(s.world()->original_bytes(),before,n));
 LocalSnapshotReceipt snap{};CHECK(encode_local_workers_snapshot(*s.world(),out,n,snap)==LocalSnapshotError::ok);
 CHECK(s.load_resumed_commands(out,n,text,nt,profile,profile==Variant::alien_crossfire,false,snap.runtime_seed)==LocalOrderError::ok&&s.commit_wake_unit(plan,receipt)==LocalOrderError::stale_plan);
 CHECK(s.prepare_wake_unit(id,plan)==LocalOrderError::ok&&s.commit_wake_unit(plan,receipt)==LocalOrderError::ok&&!receipt.changed);
 CHECK(encode_local_workers_snapshot(*s.world(),again,n,snap)==LocalSnapshotError::ok&&!memcmp(out,again,n));
 alignas(LocalProductionSession) unsigned char storage[sizeof(LocalProductionSession)];auto*reuse=new(storage)LocalProductionSession;
 CHECK(reuse->load_resumed_commands(save,n,text,nt,profile,profile==Variant::alien_crossfire,false,1)==LocalOrderError::ok&&reuse->prepare_wake_unit(id,plan)==LocalOrderError::ok);reuse->~LocalProductionSession();reuse=new(storage)LocalProductionSession;
 CHECK(reuse->load_resumed_commands(save,n,text,nt,profile,profile==Variant::alien_crossfire,false,1)==LocalOrderError::ok&&reuse->commit_wake_unit(plan,receipt)==LocalOrderError::stale_plan);reuse->~LocalProductionSession();
 CHECK(emit(guards)&&emit(0)&&fwrite(out,1,n,stdout)==n);
 free(before);free(out);free(again);free(save);free(text);return 0;
}
