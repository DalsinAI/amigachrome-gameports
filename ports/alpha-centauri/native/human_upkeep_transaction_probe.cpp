/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "human_upkeep_transaction.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static uint8_t* read(const char* name,size_t& n){FILE*f=fopen(name,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}const long size=ftell(f);rewind(f);if(size<=0||size>64*1024*1024){fclose(f);return nullptr;}auto*p=(uint8_t*)malloc((size_t)size);if(!p){fclose(f);return nullptr;}const bool ok=fread(p,1,(size_t)size,f)==(size_t)size;fclose(f);if(!ok){free(p);return nullptr;}n=(size_t)size;return p;}
static bool write(const char* name,const uint8_t*p,size_t n){FILE*f=fopen(name,"wb");if(!f)return false;const bool ok=fwrite(p,1,n,f)==n;return fclose(f)==0&&ok;}
static uint32_t crc(const uint8_t*p,size_t n){uint32_t c=~0u;while(n--){c^=*p++;for(unsigned i=0;i<8;++i)c=(c>>1)^((c&1)?0xedb88320u:0);}return ~c;}
static void put(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;++i){p[i]=(uint8_t)n;n>>=8;}}
static void seal(uint8_t*p,size_t n,size_t world){put(p+24,crc(p+64,world));put(p+28,crc(p+64+world,world));memset(p+32,0,4);put(p+32,crc(p,n));}
static int extension(const uint8_t*source,size_t n,int mode,const char* output){
 OriginalWorld world;if(world.load(source,n,0xabcdef01u)!=WorldLoadError::ok||!world.base(0))return 20;
 // Controlled private engine state for serialization qualification only;
 // none of these test writes is a public game action or commit authority.
 auto&base=*const_cast<OriginalBase*>(world.base(0));
 auto&faction=*const_cast<OriginalFactionState*>(world.faction((size_t)base.faction_id));
 if(mode==10){base.pop_size=3;base.nutrients_accumulated=-17;base.worked_tiles=7;base.specialist_total=1;
  base.specialist_types[0]=0x01234560;base.specialist_types[1]=0x06543210;
  base.economy_total=19;base.psych_total=23;base.labs_total=29;faction.energy_credits=-31;faction.labs_total=37;
 }else if(mode==11)base.x+=2;
 else if(mode==12)faction.SE_growth_pending+=1;
 else if(mode==13)base.pop_size=-1;
 else if(mode==14)base.worked_tiles=6;
 else if(mode==15)base.economy_total=-1;
 else if(mode==16)base.specialist_adjust+=1;
 else if(mode==17)const_cast<OriginalGamePreferences&>(world.game().preferences).CurrentTurn+=1;
 else return 20;
 auto* bytes=(uint8_t*)malloc(n);if(!bytes)return 20;memset(bytes,0x5a,n);
 LocalSnapshotReceipt receipt{};receipt.bytes=123;receipt.runtime_seed=456;
 const auto error=encode_human_upkeep_snapshot(world,bytes,n,receipt);
 if(error!=LocalSnapshotError::ok){if(receipt.bytes!=123||receipt.runtime_seed!=456)return 21;for(size_t i=0;i<n;++i)if(bytes[i]!=0x5a)return 21;}
 const bool saved=write(output,bytes,n);free(bytes);if(!saved)return 21;
 printf("ENCODE status=%u bytes=%lu\n",(unsigned)error,(unsigned long)n);return 0;
}
int main(int argc,char**argv){
 if(argc!=6){puts("Usage: human-upkeep-probe RULES WORLD BASE MODE NEW_OUTPUT (MODE:0/1 full turn;10..17 encoder controls)");return 20;}
 FILE* exists=fopen(argv[5],"rb");if(exists){fclose(exists);return 20;}
 size_t n=0,rn=0;auto*source=read(argv[2],n);auto*rules=read(argv[1],rn);if(!source||!rules)return 20;
 const int mode=atoi(argv[4]);if(mode>=10){const int result=extension(source,n,mode,argv[5]);free(source);free(rules);return result;}
 HumanUpkeepTransaction transaction;
 const auto opened=transaction.start(source,n,rules,rn,0xabcdef01u);
 if(opened!=HumanUpkeepError::ok){printf("OPEN %u\n",(unsigned)opened);free(source);free(rules);return 0;}
 const size_t base=(size_t)strtoul(argv[3],nullptr,10);const bool full=atoi(argv[4])!=0;const size_t total=transaction.checkpoint_size();
 auto*before=(uint8_t*)malloc(total);auto*checkpoint=(uint8_t*)malloc(total);auto*copy=(uint8_t*)malloc(total);auto*payload=(uint8_t*)malloc(n);
 if(!before||!checkpoint||!copy||!payload||transaction.checkpoint(before,total)!=LocalSnapshotError::ok)return 21;
 if(transaction.prepare_population(transaction.source_world()->game().base_count,full)!=HumanUpkeepError::bad_state ||
    transaction.checkpoint(copy,total)!=LocalSnapshotError::ok||memcmp(copy,before,total))return 21;
 const auto status=transaction.prepare_population(base,full);
 if(transaction.checkpoint(checkpoint,total)!=LocalSnapshotError::ok)return 21;
 if(status!=HumanUpkeepError::ok&&(transaction.phase()!=HumanUpkeepPhase::unprepared||memcmp(before,checkpoint,total)))return 21;
 if(memcmp(source,transaction.source_world()->original_bytes(),n))return 21;
 LocalSnapshotReceipt encoded{};
 if(encode_human_upkeep_snapshot(*transaction.source_world(),payload,n,encoded)!=LocalSnapshotError::ok||memcmp(payload,source,n))return 21;
 if(encode_human_upkeep_snapshot(*transaction.proposed_world(),payload,n,encoded)!=LocalSnapshotError::ok||memcmp(payload,checkpoint+64+n,n))return 21;
 if(transaction.start(source,n,rules,rn,0)!=HumanUpkeepError::bad_state ||
    transaction.checkpoint(copy,total-1)!=LocalSnapshotError::bad_buffer ||
    transaction.checkpoint(&transaction,total)!=LocalSnapshotError::bad_buffer ||
    transaction.checkpoint(const_cast<uint8_t*>(transaction.proposed_world()->original_bytes()),total)!=LocalSnapshotError::bad_buffer)return 21;
 HumanUpkeepTransaction restored;
 if(restored.resume(checkpoint,total,rules,rn)!=HumanUpkeepError::ok||restored.checkpoint(copy,total)!=LocalSnapshotError::ok||memcmp(copy,checkpoint,total))return 21;
 const auto repeated=restored.prepare_population(base,full);
 if(repeated!=(status==HumanUpkeepError::ok?HumanUpkeepError::already_prepared:status)||restored.checkpoint(copy,total)!=LocalSnapshotError::ok||memcmp(copy,checkpoint,total))return 21;
 for(unsigned i=0;i<4;++i){memcpy(copy,checkpoint,total);copy[i==3?total-1:i*20]^=1;
  if(restored.resume(copy,total,rules,rn)!=HumanUpkeepError::bad_checkpoint||restored.checkpoint(copy,total)!=LocalSnapshotError::ok||memcmp(copy,checkpoint,total))return 21;
 }
 // Correctly re-sealed phase and private-payload edits still fail deterministic
 // reconstruction; callers cannot clear a pending specialist/riot event.
 memcpy(copy,checkpoint,total);put(copy+44,transaction.phase()==HumanUpkeepPhase::population_checked?2:4);seal(copy,total,n);
 if(restored.resume(copy,total,rules,rn)!=HumanUpkeepError::bad_checkpoint||restored.checkpoint(copy,total)!=LocalSnapshotError::ok||memcmp(copy,checkpoint,total))return 21;
 memcpy(copy,checkpoint,total);copy[total-1]^=1;seal(copy,total,n);
 if(restored.resume(copy,total,rules,rn)!=HumanUpkeepError::bad_checkpoint||restored.checkpoint(copy,total)!=LocalSnapshotError::ok||memcmp(copy,checkpoint,total))return 21;
 if(restored.resume(checkpoint,total,rules,rn,0)!=HumanUpkeepError::no_memory||
    restored.resume(checkpoint,total,(const uint8_t*)"x",1)!=HumanUpkeepError::bad_rules||
    restored.checkpoint(copy,total)!=LocalSnapshotError::ok||memcmp(copy,checkpoint,total))return 21;
 if(!write(argv[5],checkpoint,total))return 21;
 const auto*r=transaction.receipt();const auto*old=transaction.source_world()->base(base);const auto*next=transaction.proposed_world()->base(base);
 printf("HUMAN status=%u repeat=%u phase=%u food_checked=%u specialists_pending=%u riots_pending=%u before_pop=%ld next_pop=%ld before_food=%ld next_food=%ld worked=%lu bytes=%lu\n",
  (unsigned)status,(unsigned)repeated,(unsigned)transaction.phase(),r?(unsigned)r->food_checked:0,r?(unsigned)r->specialists_pending:0,r?(unsigned)r->riots_pending:0,
  old?(long)old->pop_size:-1L,next?(long)next->pop_size:-1L,old?(long)old->nutrients_accumulated:0L,next?(long)next->nutrients_accumulated:0L,
  next?(unsigned long)(uint32_t)next->worked_tiles:0UL,(unsigned long)total);
 restored.close();if(restored.source_world()||restored.proposed_world()||restored.receipt()||restored.checkpoint_size())return 21;
 free(source);free(rules);free(before);free(checkpoint);free(copy);free(payload);return 0;
}
