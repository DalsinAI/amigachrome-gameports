/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "local_player.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static uint8_t* read(const char* p,size_t& n){FILE* f=fopen(p,"rb");if(!f)return nullptr;fseek(f,0,SEEK_END);long l=ftell(f);rewind(f);if(l<=0){fclose(f);return nullptr;}n=(size_t)l;auto* b=(uint8_t*)malloc(n);if(!b || fread(b,1,n,f)!=n){free(b);b=nullptr;}fclose(f);return b;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 21;}++checks;}while(0)
int main(int argc,char** argv){
 if(argc!=4)return 20;
 size_t rn=0,sn=0;auto* r=read(argv[1],rn);auto* s=read(argv[2],sn);if(!r || !s)return 20;
 unsigned checks=0;LocalPlayer p,q;LocalStepReceipt move{};FacilityOrderReceipt order{};
 CHECK(p.checkpoint_size()==0);CHECK(p.step(0,1,1,move)==LocalOrderError::bad_state);
 CHECK(p.load(s,sn,r,rn,0xabcdef01)==LocalOrderError::ok);CHECK(!p.dirty());
 CHECK(p.next_facility(0)==LocalOrderError::ok);CHECK(p.proposal());CHECK(p.proposal()->facility==1);
 CHECK(p.next_facility(0)==LocalOrderError::ok);CHECK(p.proposal()->facility==2);
 CHECK(p.step(0,0,0,move)==LocalOrderError::bad_state);CHECK(p.proposal());
 CHECK(p.step(0,1,1,move)==LocalOrderError::ok);CHECK(move.changed);CHECK(!p.proposal());CHECK(p.dirty());
 CHECK(p.confirm_facility(order)==LocalOrderError::bad_state);
 CHECK(p.next_facility(0)==LocalOrderError::ok);CHECK(p.confirm_facility(order)==LocalOrderError::ok);CHECK(order.changed);
 CHECK(p.world()->base(0)->queue_items[0]==-1);
 CHECK(p.step(0,-1,-1,move)==LocalOrderError::ok);CHECK(move.changed);
 CHECK(p.next_facility(0)==LocalOrderError::ok);CHECK(p.proposal()->facility==1);
 CHECK(p.next_facility(0)==LocalOrderError::ok);CHECK(p.proposal()->facility==2);
 CHECK(p.step(0,1,1,move)==LocalOrderError::ok);CHECK(move.result.status==StepStatus::exhausted);CHECK(!move.changed);CHECK(p.proposal());
 CHECK(p.confirm_facility(order)==LocalOrderError::ok);CHECK(p.world()->base(0)->queue_items[0]==-2);
 const size_t n=p.checkpoint_size();auto* out=(uint8_t*)malloc(n);if(!out)return 20;memset(out,0x5a,n);
 CHECK(p.checkpoint(out,n-1)==LocalSnapshotError::bad_buffer);bool untouched=true;for(size_t i=0;i<n;++i)if(out[i]!=0x5a)untouched=false;CHECK(untouched);
 CHECK(p.checkpoint((void*)p.world()->original_bytes(),n)==LocalSnapshotError::bad_buffer);
 CHECK(p.checkpoint((void*)p.world()->vehicle(0),n)==LocalSnapshotError::bad_buffer);
 CHECK(p.checkpoint(out,n)==LocalSnapshotError::ok);CHECK(p.dirty());p.saved();CHECK(!p.dirty());
 CHECK(q.resume(out,n,r,rn)==LocalOrderError::ok);CHECK(!q.dirty());CHECK(q.world()->runtime_seed()==0xabcdef01);
 CHECK(q.world()->vehicle(0)->movement_turns==p.world()->vehicle(0)->movement_turns);
 CHECK(q.world()->base(0)->queue_items[0]==-2);CHECK(q.world()->base(0)->minerals_accumulated==p.world()->base(0)->minerals_accumulated);
 CHECK(q.step(0,1,1,move)==LocalOrderError::ok);CHECK(move.result.status==StepStatus::exhausted);
 auto* second=(uint8_t*)malloc(n);if(!second)return 20;CHECK(q.checkpoint(second,n)==LocalSnapshotError::ok);CHECK(!memcmp(out,second,n));
 CHECK(q.next_facility(0)==LocalOrderError::ok);const auto* stable=q.world();
 for(size_t i=0;i<28;++i){out[i]^=1;CHECK(q.resume(out,n,r,rn)==(i>=24?LocalOrderError::bad_rules:LocalOrderError::bad_world));out[i]^=1;CHECK(q.world()==stable);CHECK(q.proposal());}
 out[n-1]^=1;CHECK(q.resume(out,n,r,rn)==LocalOrderError::bad_world);out[n-1]^=1;
 CHECK(q.resume(out,n-1,r,rn)==LocalOrderError::bad_world);CHECK(q.resume(out,0,r,rn)==LocalOrderError::bad_world);
 CHECK(q.resume(out,n,(const uint8_t*)"bad",3)==LocalOrderError::bad_rules);CHECK(q.world()==stable);CHECK(q.proposal());
 CHECK(q.confirm_facility(order)==LocalOrderError::ok);CHECK(q.dirty());
 FILE* f=fopen(argv[3],"wb");if(!f || fwrite(out,1,n,f)!=n || fclose(f))return 20;
 q.close();CHECK(!q.world());CHECK(!q.dirty());CHECK(!q.proposal());
 printf("local-player checks=%u checkpoint=%lu\n",checks,(unsigned long)n);free(out);free(second);free(r);free(s);return 0;
}
