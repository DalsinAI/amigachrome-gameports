/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "movement_snapshot.h"
#include "production_snapshot.h"
#include "local_workers_snapshot.h"
#include "local_accounting_snapshot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <initializer_list>
using namespace alpha;
static uint32_t digest(const uint8_t* p,size_t n){uint32_t h=2166136261u;for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;return h;}
template<class E,class R>static bool check(OriginalWorld& w,E(*encode)(const OriginalWorld&,void*,size_t,R&),uint8_t* output,unsigned& controls){
 const size_t n=w.original_size();const auto span=w.current_monuments();
 const uint32_t initial=digest(span.data,span.size);R receipt{};
 if(encode(w,output,n,receipt)!=E::ok || receipt.bytes!=n || receipt.runtime_seed!=0xabcdef01 || memcmp(output,w.original_bytes(),n))return false;
 ++controls;
 for(size_t offset:{size_t(0),size_t(16),span.size-1}){
  R sentinel{123,456};memset(output,0xa5,n);
  if(encode(w,const_cast<uint8_t*>(span.data)+offset,n,sentinel)!=E::bad_buffer || sentinel.bytes!=123 || sentinel.runtime_seed!=456 || digest(span.data,span.size)!=initial)return false;
  ++controls;
 }
 memset(output,0xa5,n);
 // Deliberately hostile buffer arguments, not a runtime write interface.
 auto& alias=*reinterpret_cast<R*>(const_cast<uint8_t*>(span.data));
 if(encode(w,output,n,alias)!=E::bad_buffer || digest(span.data,span.size)!=initial)return false;
 for(size_t i=0;i<n;++i)if(output[i]!=0xa5)return false;
 ++controls;
 for(size_t offset:{size_t(0),size_t(0x4f4),span.size-1}){
  // Test-only corruption of separate current storage. No completion/award or
  // callback is simulated; baseline wire must remain unchanged throughout.
  auto* live=const_cast<uint8_t*>(span.data);live[offset]^=0x80;R sentinel{123,456};
  memset(output,0xa5,n);
  if(encode(w,output,n,sentinel)!=E::unsupported_change || sentinel.bytes!=123 || sentinel.runtime_seed!=456)return false;
  for(size_t i=0;i<n;++i)if(output[i]!=0xa5)return false;
  if(digest(w.game().monuments.data,w.game().monuments.size)!=initial)return false;
  live[offset]^=0x80;++controls;
 }
 return digest(span.data,span.size)==initial;
}
int main(int argc,char** argv){
 if(argc!=2)return 20;
 FILE* f=fopen(argv[1],"rb");if(!f)return 20;
 if(fseek(f,0,SEEK_END)){fclose(f);return 20;}long count=ftell(f);rewind(f);
 if(count<=0 || count>64L*1024*1024){fclose(f);return 20;}
 auto* input=(uint8_t*)malloc((size_t)count);auto* output=(uint8_t*)malloc((size_t)count);
 if(!input || !output){fclose(f);free(input);free(output);return 20;}
 bool read=fread(input,1,(size_t)count,f)==(size_t)count;fclose(f);if(!read){free(input);free(output);return 20;}
 OriginalWorld w;if(w.load(input,(size_t)count,0xabcdef01)!=WorldLoadError::ok)return 20;
 const auto span=w.current_monuments();if(span.size!=0x27a0 || span.data==w.game().monuments.data || memcmp(span.data,w.game().monuments.data,span.size))return 21;
 if(!w.overlaps_byte_storage(span.data,1) || !w.overlaps_byte_storage(span.data+span.size-1,1) ||
    !w.overlaps_byte_storage(w.original_bytes(),1) || w.overlaps_byte_storage(span.data,0) ||
    !w.overlaps_byte_storage((const void*)(UINTPTR_MAX-7),16))return 21;
 const uintptr_t start=(uintptr_t)span.data;
 if(!w.overlaps_byte_storage((const void*)(start-1),2))return 21;
 OriginalWorld absent;if(absent.overlaps_byte_storage(span.data,span.size))return 21;
 unsigned controls=0;
 if(!check(w,encode_movement_snapshot,output,controls) || !check(w,encode_production_snapshot,output,controls) ||
    !check(w,encode_local_actions_snapshot,output,controls) || !check(w,encode_local_accounting_snapshot,output,controls) ||
    !check(w,encode_local_workers_snapshot,output,controls))return 21;
 if(memcmp(input,w.original_bytes(),(size_t)count))return 21;
 printf("version=%lu controls=%u monuments=%08lx baseline=%08lx\n",(unsigned long)w.game().data_version,controls,(unsigned long)digest(span.data,span.size),(unsigned long)digest(input,(size_t)count));
 free(input);free(output);return 0;
}
