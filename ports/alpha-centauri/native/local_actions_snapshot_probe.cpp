/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "local_actions_snapshot.h"
#include "local_actions_test_state.h"
#include "vehicle_stack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static OriginalWorld world,reload;
static uint32_t hash(const uint8_t* p,size_t n){uint32_t h=2166136261;for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;return h;}
int main(int argc,char** argv){
 if(argc!=4)return 20;
 const int mode=atoi(argv[3]);if(mode<0 || mode>15)return 20;
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
 if(world.load(input,(size_t)length,0xabcdef01)!=WorldLoadError::ok){free(input);free(output);return 20;}
 if(mode==1 || mode==15){
  const auto* v=world.vehicle(0);if(!v)return 21;
  VehicleStackStorage s{(int32_t)world.game().map.info.width,(int32_t)world.game().map.info.height,const_cast<OriginalMapTile*>(world.tile(0)),world.game().map.info.tile_count,const_cast<OriginalVehicle*>(world.vehicle(0)),world.game().vehicle_count};
  if(relocate_vehicle_storage(s,0,v->x+1,v->y+1,0)!=StackError::ok)return 21;
  auto* moved=const_cast<OriginalVehicle*>(world.vehicle(0));moved->moves_spent=3;moved->state|=4;moved->rotate_angle=2;
 }else if(mode==2){auto* b=const_cast<OriginalBase*>(world.base(0));if(!b)return 21;++b->nutrients_accumulated;}
 else if(mode==3)++const_cast<OriginalPrototype*>(world.prototype(0))->cost;
 else if(mode==4)++const_cast<OriginalFactionState*>(world.faction(1))->energy_credits;
 else if(mode==5)++const_cast<OriginalGameView&>(world.game()).preferences.CurrentTurn;
 else if(mode==6){auto* v=const_cast<OriginalVehicle*>(world.vehicle(0));if(!v)return 21;v->next_veh_id_stack=0;}
 else if(mode==8)++const_cast<OriginalFactionDefinition*>(world.faction_definition(1))->rule_morale;
 else if(mode==10){auto* v=const_cast<OriginalVehicle*>(world.vehicle(0));if(!v)return 21;v->unit_id=512;}
 else if(mode==11){auto* v=const_cast<OriginalVehicle*>(world.vehicle(0));if(!v)return 21;v->home_base_id=(int16_t)world.game().base_count;}
 else if(mode==12)const_cast<OriginalMapTile*>(world.tile(0))->items^=2u;
 if(mode==13)++const_cast<OriginalBase*>(world.base(0))->minerals_accumulated_2;
 if(mode==14 || mode==15){
  for(size_t i=0;i<world.game().base_count;++i){auto* b=const_cast<OriginalBase*>(world.base(i));b->minerals_accumulated=(i&1)?INT32_MAX:INT32_MIN;b->queue_items[0]=-1-(int32_t)(i%20);}
  for(size_t i=0;i<8;++i)for(size_t j=0;j<512;++j)const_cast<OriginalFactionState*>(world.faction(i))->units_queue[j]=(uint8_t)(i*41+j*17);
 }
 memset(output,0xa5,(size_t)length);LocalSnapshotReceipt receipt{123,0x12345678};
 const auto error=encode_local_actions_snapshot(world,mode==7?const_cast<uint8_t*>(world.original_bytes()):output,(size_t)length-(mode==9?1u:0u),receipt);
 if(memcmp(world.original_bytes(),input,(size_t)length))return 21;
 if(mode>=2 && mode<14){
  const auto expected=(mode==6 || mode==10 || mode==11 || mode==12)?LocalSnapshotError::bad_reference:(mode==7 || mode==9)?LocalSnapshotError::bad_buffer:LocalSnapshotError::unsupported_change;
  if(error!=expected || receipt.bytes!=123 || receipt.runtime_seed!=0x12345678)return 21;
  for(long i=0;i<length;++i)if(output[i]!=0xa5)return 21;
  printf("error=%s unchanged=1 rc=0\n",local_snapshot_error_name(error));free(input);free(output);return 0;
 }
 if(error!=LocalSnapshotError::ok || receipt.bytes!=(size_t)length || receipt.runtime_seed!=0xabcdef01 ||
    (!mode && memcmp(input,output,(size_t)length)))return 21;
 if(reload.load(output,(size_t)length,receipt.runtime_seed)!=WorldLoadError::ok)return 21;
 if(!alpha_test::same_world(world,reload,false))return 21;
 auto* again=(uint8_t*)malloc((size_t)length);if(!again)return 21;LocalSnapshotReceipt reencoded;
 if(encode_local_actions_snapshot(reload,again,(size_t)length,reencoded)!=LocalSnapshotError::ok || memcmp(again,output,(size_t)length) || reencoded.runtime_seed!=receipt.runtime_seed)return 21;
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
 printf("bytes=%lu snapshot=%08lx seed=%08lx rc=0\n",(unsigned long)receipt.bytes,(unsigned long)hash(output,(size_t)length),(unsigned long)receipt.runtime_seed);
 free(input);free(output);return 0;
}
