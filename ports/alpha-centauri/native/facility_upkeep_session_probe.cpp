/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "facility_upkeep_session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static uint8_t* read(const char* name,size_t& n){
 FILE* f=fopen(name,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}
 const long size=ftell(f);rewind(f);if(size<=0 || size>64*1024*1024){fclose(f);return nullptr;}
 auto* p=(uint8_t*)malloc((size_t)size);if(!p){fclose(f);return nullptr;}
 const bool ok=fread(p,1,(size_t)size,f)==(size_t)size;fclose(f);if(!ok){free(p);return nullptr;}n=(size_t)size;return p;
}
static bool write(const char* name,const uint8_t* p,size_t n){FILE* f=fopen(name,"wb");if(!f)return false;const bool ok=fwrite(p,1,n,f)==n;return fclose(f)==0 && ok;}
static FacilityStageReceipt* receipt_alias(void* p){return static_cast<FacilityStageReceipt*>(p);}
static uint32_t crc(const uint8_t* p,size_t n){uint32_t c=~0u;while(n--){c^=*p++;for(unsigned i=0;i<8;++i)c=(c>>1)^((c&1u)?0xedb88320u:0);}return ~c;}
static void put(uint8_t* p,uint32_t n){for(unsigned i=0;i<4;++i){p[i]=(uint8_t)n;n>>=8;}}
int main(int argc,char** argv){
 if(argc!=6){puts("Usage: upkeep-session-probe RULES WORLD BASE MODE NEW_OUTPUT");return 20;}
 FILE* exists=fopen(argv[5],"rb");if(exists){fclose(exists);return 20;}
 size_t rn=0,n=0;auto* rules=read(argv[1],rn);auto* source=read(argv[2],n);if(!rules || !source)return 20;
 const size_t base=(size_t)strtoul(argv[3],nullptr,10);const int mode=atoi(argv[4]);
 FacilityUpkeepSession session;
 const auto opened=mode==2?session.resume(source,n,rules,rn):session.start(source,n,rules,rn,0xabcdef01u);
 if(opened!=UpkeepSessionError::ok){printf("OPEN %u\n",(unsigned)opened);free(rules);free(source);return 0;}
 const size_t count=session.checkpoint_size();auto* before=(uint8_t*)malloc(count);auto* checkpoint=(uint8_t*)malloc(count);auto* copy=(uint8_t*)malloc(count);
 if(!before || !checkpoint || !copy || session.checkpoint(before,count)!=LocalSnapshotError::ok)return 21;
 if(session.start(source,n,rules,rn,1)!=UpkeepSessionError::bad_state)return 21;
 memset(copy,0x5a,count);if(session.checkpoint(copy,count-1)!=LocalSnapshotError::bad_buffer)return 21;
 for(size_t i=0;i<count;++i)if(copy[i]!=0x5a)return 21;
 auto* alias=const_cast<uint8_t*>(session.world()->original_bytes());
 if(session.checkpoint(alias,count)!=LocalSnapshotError::bad_buffer ||
    session.checkpoint(&session,count)!=LocalSnapshotError::bad_buffer)return 21;
 FacilityStageReceipt receipt{};receipt.base=-123;
 StableIntakeStage stage{Variant::base,false,false,{0,false},0,0};if(mode==1)stage.worker.upkeep_state=1;
 if(session.apply_fresh_materials(base,stage,*receipt_alias(&stage))!=FacilityProductionError::bad_buffer ||
    session.apply_fresh_materials(base,stage,*receipt_alias(alias))!=FacilityProductionError::bad_buffer)return 21;
 const auto status=session.apply_fresh_materials(base,stage,receipt);
 if(session.checkpoint(checkpoint,count)!=LocalSnapshotError::ok)return 21;
 if(status!=FacilityProductionError::ok && (memcmp(before,checkpoint,count) || receipt.base!=-123))return 21;
 FacilityUpkeepSession restored;
 if(restored.resume(checkpoint,count,rules,rn)!=UpkeepSessionError::ok || restored.checkpoint(copy,count)!=LocalSnapshotError::ok || memcmp(copy,checkpoint,count))return 21;
 FacilityStageReceipt repeat{};
 const auto again=restored.apply_fresh_materials(base,stage,repeat);
 if(again!=(status==FacilityProductionError::ok?FacilityProductionError::already_processed:status) ||
    restored.checkpoint(copy,count)!=LocalSnapshotError::ok || memcmp(copy,checkpoint,count))return 21;
 // Every header byte and sampled payload bytes must reject accidental damage,
 // while preserving an already loaded resumed segment, world and ledger.
#if defined(__m68k__)
 const size_t corruption_cases=4;
#else
 const size_t corruption_cases=mode==4?264:4;
#endif
 for(size_t i=0;i<corruption_cases;++i){
  const size_t at=corruption_cases==4?(i==3?count-1:i*20):(i<256?i:256+(i-256)*(count-256)/8);
  memcpy(copy,checkpoint,count);copy[at]^=1;
  if(restored.resume(copy,count,rules,rn)!=UpkeepSessionError::bad_checkpoint ||
     restored.checkpoint(copy,count)!=LocalSnapshotError::ok || memcmp(copy,checkpoint,count))return 21;
 }
 // Correct checksums do not make an inconsistent recompute/choice ledger valid.
 memcpy(copy,checkpoint,count);copy[128]^=1;memset(copy+28,0,4);put(copy+28,crc(copy,count));
 if(restored.resume(copy,count,rules,rn)!=UpkeepSessionError::bad_checkpoint ||
    restored.checkpoint(copy,count)!=LocalSnapshotError::ok || memcmp(copy,checkpoint,count))return 21;
 if(restored.resume(checkpoint,count-1,rules,rn)!=UpkeepSessionError::bad_checkpoint ||
    restored.resume(checkpoint,count,rules,rn,0)!=UpkeepSessionError::no_memory ||
    restored.resume(checkpoint,count,(const uint8_t*)"x",1)!=UpkeepSessionError::bad_rules ||
    restored.checkpoint(copy,count)!=LocalSnapshotError::ok || memcmp(copy,checkpoint,count))return 21;
 if(!write(argv[5],checkpoint,count))return 21;
 printf("SESSION status=%s repeat=%s processed=%u recompute=%u choice=%u complete=%u bytes=%lu\n",facility_production_error_name(status),
  facility_production_error_name(again),(unsigned)restored.processed(base),(unsigned)restored.recompute_pending(base),
  (unsigned)restored.production_choice_pending(base),(unsigned)receipt.production.completed,(unsigned long)count);
 restored.close();if(restored.world() || restored.checkpoint_size() || restored.processed(base) || restored.recompute_pending(base) ||
    restored.production_choice_pending(base) || restored.apply_fresh_materials(base,stage,receipt)!=FacilityProductionError::bad_state)return 21;
 free(before);free(checkpoint);free(copy);free(rules);free(source);return 0;
}
