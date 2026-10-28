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

int main(int argc,char**argv){
 if(argc!=3)return 20;
 size_t n=0,nt=0;auto*save=read(argv[2],n);auto*text=read(argv[1],nt);if(!save||!text)return 20;
 OriginalGameView v{};if(read_original_game(save,n,v)!=GameReadError::ok || !v.vehicle_count)return 20;
 const auto profile=v.data_version<11?Variant::base:Variant::alien_crossfire;const size_t id=v.vehicle_count-1;unsigned guards=0;
 LocalProductionSession s;CHECK(s.load_resumed_commands(save,n,text,nt,profile,profile==Variant::alien_crossfire,false,0xabcdef01)==LocalOrderError::ok);
 UnitReadiness result{};result.ready=result.jailed=result.speed_evaluated=true;result.speed=123;unsigned char sentinel[sizeof result];memcpy(sentinel,&result,sizeof result);const auto rev=s.revision();
 CHECK(s.unit_readiness(v.vehicle_count,result)==LocalOrderError::bad_state&&!memcmp(&result,sentinel,sizeof result));
 CHECK(s.unit_readiness(id,alias<UnitReadiness>((uintptr_t)&s))==LocalOrderError::bad_state);
 CHECK(s.unit_readiness(id,alias<UnitReadiness>((uintptr_t)s.world()->original_bytes()))==LocalOrderError::bad_state);
 CHECK(s.unit_readiness(id,alias<UnitReadiness>(((uintptr_t)s.world()->vehicle(id)+alignof(UnitReadiness)-1)&~(uintptr_t)(alignof(UnitReadiness)-1)))==LocalOrderError::bad_state);
 CHECK(s.set_suspended(true)==LocalOrderError::ok&&s.unit_readiness(id,result)==LocalOrderError::suspended);
 CHECK(s.set_suspended(false)==LocalOrderError::ok);
 const auto e=s.unit_readiness(id,result);const unsigned status=e==LocalOrderError::ok?0:e==LocalOrderError::overflow?2:1;
 if(e!=LocalOrderError::ok)CHECK(!memcmp(&result,sentinel,sizeof result));
 CHECK(s.revision()==rev+2&&s.world()->runtime_seed()==0xabcdef01);
 const unsigned bits=e==LocalOrderError::ok?(unsigned)result.ready|((unsigned)result.jailed<<1)|((unsigned)result.speed_evaluated<<2):0;
 const unsigned speed=e==LocalOrderError::ok?result.speed:0;unsigned after=0;
 if(e==LocalOrderError::ok&&result.ready){LocalSkipCommand command;LocalSkipReceipt receipt{};CHECK(s.prepare_skip_unit(id,command)==LocalOrderError::ok&&s.commit_skip_unit(command,receipt)==LocalOrderError::ok&&receipt.changed);UnitReadiness r{};CHECK(s.unit_readiness(id,r)==LocalOrderError::ok&&!r.ready);after=(unsigned)r.ready;}
 auto*out=(uint8_t*)malloc(n);if(!out)return 20;LocalSnapshotReceipt snap{};CHECK(encode_local_workers_snapshot(*s.world(),out,n,snap)==LocalSnapshotError::ok);
 CHECK(emit(guards)&&emit(status)&&emit(bits)&&emit(speed)&&emit(after)&&fwrite(out,1,n,stdout)==n);
 free(out);free(save);free(text);return 0;
}
