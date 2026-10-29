/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "local_production_session.h"
#include "local_workers_snapshot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static uint8_t*read(const char*p,size_t&n){FILE*f=fopen(p,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long z=ftell(f);rewind(f);if(z<=0||z>64*1024*1024){fclose(f);return nullptr;}auto*b=(uint8_t*)malloc((size_t)z);if(!b){fclose(f);return nullptr;}bool ok=fread(b,1,(size_t)z,f)==(size_t)z;fclose(f);if(!ok){free(b);return nullptr;}n=(size_t)z;return b;}
static bool emit(uint32_t n){uint8_t b[4];for(unsigned i=0;i<4;++i){b[i]=(uint8_t)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"failed line %d\n",__LINE__);return 21;}++guards;}while(0)

template<class T>static T& alias(uintptr_t p){return *reinterpret_cast<T*>(p);}

static int32_t word(const uint8_t*p){uint32_t x=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);return x<=0x7fffffffu?(int32_t)x:-1-(int32_t)(~x);}
int main(int argc,char**argv){
 if(argc!=4)return 20;
 size_t n=0,nt=0,nf=0;auto*save=read(argv[2],n);auto*text=read(argv[1],nt);auto*f=read(argv[3],nf);if(!save||!text||!f||nf!=16)return 20;
 ReadySearchFacts facts{word(f),word(f+4),word(f+8),word(f+12)};OriginalGameView v{};if(read_original_game(save,n,v)!=GameReadError::ok)return 20;
 const auto profile=v.data_version<11?Variant::base:Variant::alien_crossfire;unsigned guards=0;
 LocalProductionSession s,other;CHECK(s.load_resumed_commands(save,n,text,nt,profile,profile==Variant::alien_crossfire,false,0xabcdef01)==LocalOrderError::ok);
 ReadySearchPlan plan,unissued;ReadySearchReceipt receipt{};
 CHECK(s.commit_ready_search(unissued,receipt)==LocalOrderError::stale_plan);
 CHECK(s.prepare_ready_search(facts,alias<ReadySearchPlan>((uintptr_t)&s))==LocalOrderError::bad_state);
 CHECK(s.prepare_ready_search(facts,alias<ReadySearchPlan>((uintptr_t)s.world()->original_bytes()))==LocalOrderError::bad_state);
 ReadySearchFacts invalid=facts;invalid.reference=(int32_t)v.vehicle_count;CHECK(s.prepare_ready_search(invalid,plan)==LocalOrderError::bad_state);
 CHECK(s.prepare_ready_search(facts,plan)==LocalOrderError::ok);
 const auto preview=plan.preview();
 CHECK(other.load_resumed_commands(save,n,text,nt,profile,profile==Variant::alien_crossfire,false,1)==LocalOrderError::ok&&other.commit_ready_search(plan,receipt)==LocalOrderError::stale_plan);
 CHECK(s.commit_ready_search(plan,alias<ReadySearchReceipt>((uintptr_t)&plan))==LocalOrderError::bad_state);
 CHECK(s.set_suspended(true)==LocalOrderError::ok&&s.commit_ready_search(plan,receipt)==LocalOrderError::suspended);
 CHECK(s.set_suspended(false)==LocalOrderError::ok&&s.commit_ready_search(plan,receipt)==LocalOrderError::stale_plan);
 CHECK(s.prepare_ready_search(facts,plan)==LocalOrderError::ok);
 const auto revision=s.revision();bool changed=false;for(auto x:preview.clear_deferred)changed|=x!=0;
 CHECK(s.commit_ready_search(plan,receipt)==LocalOrderError::ok&&receipt.changed==changed&&s.revision()==revision+(changed?1:0));
 auto*out=(uint8_t*)malloc(n);auto*again=(uint8_t*)malloc(n);if(!out||!again)return 20;LocalSnapshotReceipt snap{};
 CHECK(encode_local_workers_snapshot(*s.world(),out,n,snap)==LocalSnapshotError::ok);
 CHECK(s.prepare_ready_search(facts,plan)==LocalOrderError::ok&&s.commit_ready_search(plan,receipt)==LocalOrderError::ok&&!receipt.changed);
 CHECK(s.load_resumed_commands(out,n,text,nt,profile,profile==Variant::alien_crossfire,false,snap.runtime_seed)==LocalOrderError::ok&&s.commit_ready_search(plan,receipt)==LocalOrderError::stale_plan);
 CHECK(s.prepare_ready_search(facts,plan)==LocalOrderError::ok&&s.commit_ready_search(plan,receipt)==LocalOrderError::ok&&!receipt.changed);
 CHECK(encode_local_workers_snapshot(*s.world(),again,n,snap)==LocalSnapshotError::ok&&!memcmp(out,again,n)&&s.world()->runtime_seed()==0xabcdef01);
 CHECK(emit(guards)&&emit((uint32_t)preview.selected)&&emit((uint32_t)preview.distance)&&emit(preview.passes)&&fwrite(preview.clear_deferred,1,256,stdout)==256&&fwrite(out,1,n,stdout)==n);
 free(out);free(again);free(save);free(text);free(f);return 0;
}
