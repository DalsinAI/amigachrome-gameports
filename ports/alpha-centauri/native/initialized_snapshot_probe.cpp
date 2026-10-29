/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "initialized_snapshot.h"
#include "local_actions_test_state.h"
#include "vehicle_stack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static OriginalWorld world,reload;
static uint32_t hash(const uint8_t* p,size_t n){uint32_t h=2166136261;for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;return h;}
static uint32_t world_digest(const alpha::OriginalWorld& w){uint32_t h=2166136261;auto add=[&](const void* p,size_t n){const auto* b=(const unsigned char*)p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;};add(&w,sizeof w);if(w.loaded()){const auto& g=w.game();add(w.original_bytes(),w.original_size());add(w.current_monuments().data,w.current_monuments().size);add(w.tile(0),g.map.info.tile_count*sizeof(alpha::OriginalMapTile));add(w.vehicle(0),g.vehicle_count*sizeof(alpha::OriginalVehicle));add(w.base(0),g.base_count*sizeof(alpha::OriginalBase));add(w.prototype(0),512*sizeof(alpha::OriginalPrototype));add(w.faction(0),8*sizeof(alpha::OriginalFactionState));add(w.faction_definition(0),8*sizeof(alpha::OriginalFactionDefinition));}return h;}
int main(int argc,char** argv){
 if(argc!=5)return 20;
 const int requested=atoi(argv[3]);if(requested<0 || requested>43)return 20;
 const int mode=requested==26?19:requested;
 // Evidence paths are fresh and exclusively owned by the test runner. Refuse
 // a readable existing target, including aliases of the input. This is not an
 // atomic filesystem commit or an assumption that the SDK supports O_EXCL.
 FILE* existing=fopen(argv[2],"rb");if(existing){fclose(existing);return 20;}
 FILE* f=fopen(argv[1],"rb");if(!f)return 20;
 if(fseek(f,0,SEEK_END)){fclose(f);return 20;}const long length=ftell(f);rewind(f);
 if(length<=0 || length>64*1024*1024){fclose(f);return 20;}
 auto* input=(uint8_t*)malloc((size_t)length);auto* output=(uint8_t*)malloc((size_t)length);
 if(!input || !output){fclose(f);free(input);free(output);return 20;}
 if(fread(input,1,(size_t)length,f)!=(size_t)length){fclose(f);free(input);free(output);return 20;}fclose(f);
 FILE* text=fopen(argv[4],"rb");if(!text)return 20;
 if(fseek(text,0,SEEK_END)){fclose(text);return 20;}long text_length=ftell(text);rewind(text);if(text_length<=0 || text_length>1024*1024){fclose(text);return 20;}
 auto*text_bytes=(uint8_t*)malloc((size_t)text_length);static Rules rules_storage;auto*rules=&rules_storage;
 if(!text_bytes || fread(text_bytes,1,(size_t)text_length,text)!=(size_t)text_length)return 20;
 fclose(text);if(read_technologies(text_bytes,(size_t)text_length,*rules)!=Error::ok)return 20;free(text_bytes);
 if(world.load_with_record_repairs(input,(size_t)length,0xabcdef01,*rules)!=WorldLoadError::ok){free(input);free(output);return 20;}
 if(mode==1 || mode==15 || mode==17 || mode==19){
  const auto* v=world.vehicle(0);if(!v)return 21;
  VehicleStackStorage s{(int32_t)world.game().map.info.width,(int32_t)world.game().map.info.height,const_cast<OriginalMapTile*>(world.tile(0)),world.game().map.info.tile_count,const_cast<OriginalVehicle*>(world.vehicle(0)),world.game().vehicle_count};
  if(relocate_vehicle_storage(s,0,v->x+1,v->y+1,0)!=StackError::ok)return 21;
  auto* moved=const_cast<OriginalVehicle*>(world.vehicle(0));moved->moves_spent=3;moved->state|=4;moved->rotate_angle=2;
 }else if(mode==2){auto* b=const_cast<OriginalBase*>(world.base(0));if(!b)return 21;++b->governor_flags;}
 else if(mode==3)++const_cast<OriginalPrototype*>(world.prototype(0))->cost;
 else if(mode==4)++const_cast<OriginalFactionState*>(world.faction(1))->energy_credits;
 else if(mode==5)++const_cast<OriginalGameView&>(world.game()).preferences.CurrentTurn;
 else if(mode==6){auto* v=const_cast<OriginalVehicle*>(world.vehicle(0));if(!v)return 21;v->next_veh_id_stack=0;}
 else if(mode==8)++const_cast<OriginalFactionDefinition*>(world.faction_definition(1))->rule_morale;
 else if(mode==10){auto* v=const_cast<OriginalVehicle*>(world.vehicle(0));if(!v)return 21;v->unit_id=512;}
 else if(mode==11){auto* v=const_cast<OriginalVehicle*>(world.vehicle(0));if(!v)return 21;v->home_base_id=(int16_t)world.game().base_count;}
 else if(mode==12)const_cast<OriginalMapTile*>(world.tile(0))->items^=2u;
 if(mode==13)++const_cast<OriginalBase*>(world.base(0))->minerals_accumulated_2;
 if(mode==14 || mode==15 || mode==17 || mode==19){
  for(size_t i=0;i<world.game().base_count;++i){auto* b=const_cast<OriginalBase*>(world.base(i));b->minerals_accumulated=(i&1)?INT32_MAX:INT32_MIN;b->queue_items[0]=-1-(int32_t)(i%20);}
  for(size_t i=0;i<8;++i)for(size_t j=0;j<512;++j)const_cast<OriginalFactionState*>(world.faction(i))->units_queue[j]=(uint8_t)(i*41+j*17);
 }
 if(mode==16 || mode==17 || mode==19){
  for(size_t i=0;i<world.game().base_count;++i){auto* b=const_cast<OriginalBase*>(world.base(i));unsigned at=0;
#define ACCOUNTING_FIELD(name,offset) b->name=((at++ + i)&1)?INT32_MAX:INT32_MIN;
#define ACCOUNTING_SPECIALISTS(offset) for(unsigned j=0;j<2;++j)b->specialist_types[j]=((at++ + i)&1)?INT32_MAX:INT32_MIN;
#include "local_accounting_fields.inc"
#undef ACCOUNTING_FIELD
#undef ACCOUNTING_SPECIALISTS
  }
 }
 if(mode==18||mode==19){for(size_t i=0;i<world.game().base_count;++i){auto*b=const_cast<OriginalBase*>(world.base(i));if(b->pop_size<0)return 21;
  b->worked_tiles=1;b->specialist_total=b->pop_size;b->specialist_adjust=0;b->state_flags^=0x8100;
  b->nutrient_intake=INT32_MIN;b->mineral_intake=INT32_MAX;b->energy_intake=-123;b->unused_intake=0;b->unused_intake_2=0;
 }}
 if(mode==20)const_cast<OriginalBase*>(world.base(0))->worked_tiles=(int32_t)0x80000001u;
 if(mode==21)const_cast<OriginalBase*>(world.base(0))->specialist_total=128;
 if(mode==22)const_cast<OriginalBase*>(world.base(0))->specialist_adjust=1;
 if(mode==23)const_cast<OriginalBase*>(world.base(0))->state_flags^=0x10000000;
 if(mode==24)++const_cast<OriginalBase*>(world.base(0))->pad_3;
 if(requested==25 || requested==26){for(size_t i=0;i<8;++i){auto&f=*const_cast<OriginalFactionState*>(world.faction(i));f.player_flags&=~0x20u;unsigned cell=0;
  auto pattern=[&](){const uint32_t u=(++cell*0x01020304u)^((uint32_t)i*0x10203040u);int32_t v;memcpy(&v,&u,4);return v;};
#define PHASE_SCALAR(name,offset) f.name=pattern();
#define PHASE_ARRAY(name,count,offset) for(auto&v:f.name)v=pattern();
#define PHASE_MATRIX(name,rows,cols,offset) for(auto&row:f.name)for(auto&v:row)v=pattern();
#include "phase_faction_fields.inc"
#undef PHASE_SCALAR
#undef PHASE_ARRAY
#undef PHASE_MATRIX
 }}
 if(requested==27)++const_cast<OriginalFactionState*>(world.faction(1))->SE_economy_base;
 if(requested==28)++const_cast<OriginalFactionState*>(world.faction(1))->SE_Politics_pending;
 if(requested==29)++const_cast<OriginalFactionState*>(world.faction(1))->loan_payment[2];
 if(requested==30)const_cast<OriginalFactionState*>(world.faction(1))->player_flags^=1;
 if(requested==31)++const_cast<OriginalFactionState*>(world.faction(1))->ranking;
 if(requested==32)const_cast<uint8_t*>(world.current_monuments().data)[0]^=1;
 if(requested==33)++const_cast<OriginalGameView&>(world.game()).canvas_width;
 if(requested==34)const_cast<OriginalGameView&>(world.game()).preferences.TechOwners[0]^=1;
 if(requested==36)const_cast<OriginalGameView&>(world.game()).map.metadata+=1;
 if(requested==37){auto&f=*const_cast<OriginalFactionState*>(world.faction(1));if(f.player_flags&0x20u)return 21;f.player_flags|=0x20u;}
 if(requested==38)const_cast<OriginalGameView&>(world.game()).preferences.FactionStatus[1]^=2;
 if(requested==39)const_cast<OriginalFactionState*>(world.faction(1))->player_flags^=0x200u;
 if(requested==40)const_cast<OriginalFactionState*>(world.faction(0))->player_flags^=0x200u;
 if(requested==41)const_cast<OriginalGameView&>(world.game()).preferences.FactionStatus[0]^=2;
 if(requested==42)rules->count=88;
 if(requested==43)rules->technologies[0].prerequisites[0]=89;
 const uint32_t world_before=world_digest(world),rules_before=hash((const uint8_t*)rules,sizeof *rules);
 memset(output,0xa5,(size_t)length);LocalSnapshotReceipt receipt{123,0x12345678};
 const void*aliases[]={&world,world.original_bytes(),world.current_monuments().data,world.tile(0),world.vehicle(0),world.base(0),world.prototype(0),world.faction(0),world.faction_definition(0)};
 for(const void*at:aliases)if(at){
  if(encode_initialized_snapshot(world,*rules,const_cast<void*>(at),(size_t)length,receipt)!=LocalSnapshotError::bad_buffer || receipt.bytes!=123 || receipt.runtime_seed!=0x12345678)return 21;
  auto&r=*reinterpret_cast<LocalSnapshotReceipt*>(const_cast<void*>(at));if(encode_initialized_snapshot(world,*rules,output,(size_t)length,r)!=LocalSnapshotError::bad_buffer)return 21;
 }
 if(encode_initialized_snapshot(world,*rules,nullptr,(size_t)length,receipt)!=LocalSnapshotError::bad_buffer ||
    encode_initialized_snapshot(world,*rules,output,(size_t)length-1,receipt)!=LocalSnapshotError::bad_buffer ||
    encode_initialized_snapshot(world,*rules,(void*)(UINTPTR_MAX-3),(size_t)length,receipt)!=LocalSnapshotError::bad_buffer ||
    encode_initialized_snapshot(world,*rules,output,(size_t)length,*reinterpret_cast<LocalSnapshotReceipt*>(output))!=LocalSnapshotError::bad_buffer)return 21;
 if(encode_initialized_snapshot(world,*rules,rules,(size_t)length,receipt)!=LocalSnapshotError::bad_buffer)return 21;
 if(encode_initialized_snapshot(world,*rules,output,(size_t)length,*reinterpret_cast<LocalSnapshotReceipt*>(rules))!=LocalSnapshotError::bad_buffer)return 21;
 OriginalWorld empty;if(encode_initialized_snapshot(empty,*rules,output,(size_t)length,receipt)!=LocalSnapshotError::bad_world || receipt.bytes!=123 || receipt.runtime_seed!=0x12345678)return 21;
 for(long i=0;i<length;++i)if(output[i]!=0xa5)return 21;
 if(requested==25 || requested==26 || mode==4){LocalSnapshotReceipt old{123,0x12345678};if(encode_local_workers_snapshot(world,output,(size_t)length,old)!=LocalSnapshotError::unsupported_change || old.bytes!=123 || old.runtime_seed!=0x12345678)return 21;for(long i=0;i<length;++i)if(output[i]!=0xa5)return 21;}

 OriginalGameView imported{};if(read_original_game(input,(size_t)length,imported)!=GameReadError::ok)return 21;
 bool needs_extension=imported.preferences.FactionStatus[1]!=world.game().preferences.FactionStatus[1];
 for(size_t i=0;i<8;++i){OriginalFactionState baseline;if(original_faction_state(imported,i,baseline)!=GameReadError::ok)return 21;needs_extension|=((baseline.player_flags^world.faction(i)->player_flags)&0x200u)!=0;}
 if(needs_extension){LocalSnapshotReceipt old{123,0x12345678};if(encode_phase_snapshot(world,output,(size_t)length,old)!=LocalSnapshotError::unsupported_change || old.bytes!=123 || old.runtime_seed!=0x12345678)return 21;for(long i=0;i<length;++i)if(output[i]!=0xa5)return 21;}
 const auto error=encode_initialized_snapshot(world,*rules,mode==7?const_cast<uint8_t*>(world.original_bytes()):output,(size_t)length-(mode==9?1u:0u),receipt,requested==35?2*world.memory_bytes()-1:2*world.memory_bytes());
 if(memcmp(world.original_bytes(),input,(size_t)length) || world_before!=world_digest(world) || rules_before!=hash((const uint8_t*)rules,sizeof *rules))return 21;
 if((mode>=2 && mode<14 && mode!=4)||(mode>=20 && mode<=24)||requested>=27){
  const auto expected=requested==42 || requested==43?LocalSnapshotError::bad_world:requested==35?LocalSnapshotError::out_of_memory:(mode==6 || mode==10 || mode==11 || mode==12 || (mode>=20&&mode<=22))?LocalSnapshotError::bad_reference:(mode==7 || mode==9)?LocalSnapshotError::bad_buffer:LocalSnapshotError::unsupported_change;
  if(error!=expected || receipt.bytes!=123 || receipt.runtime_seed!=0x12345678)return 21;
  for(long i=0;i<length;++i)if(output[i]!=0xa5)return 21;
  printf("error=%s unchanged=1 rc=0\n",workers_snapshot_error_name(error));free(input);free(output);return 0;
 }
 if(error!=LocalSnapshotError::ok || receipt.bytes!=(size_t)length || receipt.runtime_seed!=0xabcdef01 ||
    false)return 21;
 if(reload.load(output,(size_t)length,receipt.runtime_seed)!=WorldLoadError::ok)return 21;
 if(!alpha_test::same_world(world,reload,false))return 21;
 auto* again=(uint8_t*)malloc((size_t)length);if(!again)return 21;LocalSnapshotReceipt reencoded;
 if(encode_initialized_snapshot(reload,*rules,again,(size_t)length,reencoded)!=LocalSnapshotError::ok || memcmp(again,output,(size_t)length) || reencoded.runtime_seed!=receipt.runtime_seed)return 21;
 free(again);
 for(size_t i=0;i<world.game().vehicle_count;++i){const auto* a=world.vehicle(i);const auto* b=reload.vehicle(i);
#define WIRE_SCALAR(type,name,offset) if(a->name!=b->name)return 21;
#define WIRE_ARRAY(type,name,count,offset) for(size_t j=0;j<count;++j)if(a->name[j]!=b->name[j])return 21;
#define WIRE_STRING(name,count,offset) if(memcmp(a->name,b->name,count+1))return 21;
#include "original_vehicle_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
 }
 for(size_t i=0;i<world.game().map.info.tile_count;++i){const auto* a=world.tile(i);const auto* b=reload.tile(i);
  if(a->climate!=b->climate || a->contour!=b->contour || a->val2!=b->val2 || a->region!=b->region || a->visibility!=b->visibility ||
     a->val3!=b->val3 || a->unknown!=b->unknown || a->owner!=b->owner || a->items!=b->items || a->landmarks!=b->landmarks ||
     memcmp(a->visible_items,b->visible_items,sizeof(a->visible_items)))return 21;
 }
 f=fopen(argv[2],"wb");if(!f)return 21;const bool written=fwrite(output,1,(size_t)length,f)==(size_t)length;const bool closed=fclose(f)==0;
 if(!written || !closed)return 21;
 const uint32_t initial_hash=hash(output,(size_t)length);
 if(requested==25 || requested==26 || mode==4){
  auto&faction=*const_cast<OriginalFactionState*>(reload.faction(1));uint32_t bits;memcpy(&bits,&faction.energy_credits,4);bits^=1u;memcpy(&faction.energy_credits,&bits,4);
  LocalSnapshotReceipt continued;if(encode_initialized_snapshot(reload,*rules,output,(size_t)length,continued)!=LocalSnapshotError::ok)return 21;
  char path[4096];if(snprintf(path,sizeof path,"%s.continued",argv[2])<0 || strlen(path)>=sizeof(path)-1)return 21;
  FILE* prior=fopen(path,"rb");if(prior){fclose(prior);return 21;}
  FILE*next=fopen(path,"wb");if(!next)return 21;bool ok=fwrite(output,1,(size_t)length,next)==(size_t)length;ok=fclose(next)==0&&ok;if(!ok)return 21;
 }
 printf("bytes=%lu snapshot=%08lx seed=%08lx rc=0\n",(unsigned long)receipt.bytes,(unsigned long)initial_hash,(unsigned long)receipt.runtime_seed);
 free(input);free(output);return 0;
}
