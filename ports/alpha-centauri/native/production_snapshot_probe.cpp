/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "production_snapshot.h"
#include "local_production_session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static uint8_t* read(const char* path,size_t& n){FILE* f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long size=ftell(f);if(size<=0 || size>32*1024*1024 || fseek(f,0,SEEK_SET)){fclose(f);return nullptr;}auto* p=(uint8_t*)malloc((size_t)size);if(!p){fclose(f);return nullptr;}n=(size_t)size;if(fread(p,1,n,f)!=n){free(p);p=nullptr;}fclose(f);return p;}
static bool emit(uint32_t value){uint8_t b[4];for(unsigned i=0;i<4;++i){b[i]=(uint8_t)value;value>>=8;}return fwrite(b,1,4,stdout)==4;}
static OriginalWorld world,reloaded;
static LocalProductionSession session,continued;
int main(int argc,char** argv){
 if(argc!=7)return 20;
 uint8_t empty_byte=0x5a;ProductionSnapshotReceipt empty_receipt{77,88};
 if(encode_production_snapshot(world,&empty_byte,1,empty_receipt)!=ProductionSnapshotError::bad_world || empty_byte!=0x5a || empty_receipt.bytes!=77 || empty_receipt.runtime_seed!=88)return 21;
 const int mode=atoi(argv[1]);const Variant profile=!strcmp(argv[2],"base")?Variant::base:Variant::alien_crossfire;const bool expansion=atoi(argv[3])!=0;const int human=atoi(argv[4]);size_t sn=0,tn=0;uint8_t* save=read(argv[6],sn);uint8_t* text=read(argv[5],tn);if(!save || !text)return 20;
 if(world.load(save,sn,0x2468ace0)!=WorldLoadError::ok)return 21;
 const OriginalWorld* w=&world;
 if(mode>=100){
  const auto e=session.load_local(save,sn,text,tn,profile,expansion,human,0x2468ace0);if(!emit((uint32_t)e))return 20;if(e!=LocalOrderError::ok){free(save);free(text);return 0;}
  FacilityOrderPlan p;auto a=session.prepare_facility(0,1,p);if(a==LocalOrderError::ok){FacilityOrderReceipt r;a=session.commit(p,r);}if(!emit((uint32_t)a))return 20;
  if(mode==101){auto b=session.prepare_facility(0,2,p);if(b==LocalOrderError::ok){FacilityOrderReceipt r;b=session.commit(p,r);}if(!emit((uint32_t)b))return 20;}
  w=session.world();
 }else if(mode==1){
  // Test-only mutations exercise the exact serialization field boundary, not
  // authorization of production commands. Real orders use the session above.
  for(size_t i=0;i<w->game().base_count;++i){auto* b=const_cast<OriginalBase*>(w->base(i));b->minerals_accumulated=(i&1)?INT32_MAX:INT32_MIN;b->queue_items[0]=-1-(int32_t)(i%20);}
  for(size_t i=0;i<8;++i)for(size_t j=0;j<512;++j)const_cast<OriginalFactionState*>(w->faction(i))->units_queue[j]=(uint8_t)(i*41+j*17);
 }else if(mode==2)const_cast<OriginalBase*>(w->base(0))->minerals_accumulated_2^=1;
 else if(mode==3)const_cast<OriginalBase*>(w->base(0))->production_id_last^=1;
 else if(mode==4)const_cast<OriginalBase*>(w->base(0))->queue_items[1]^=1;
 else if(mode==5)const_cast<OriginalBase*>(w->base(0))->queue_size^=1;
 else if(mode==6)const_cast<OriginalFactionState*>(w->faction(1))->energy_credits^=1;
 else if(mode==7)const_cast<OriginalGameView&>(w->game()).preferences.CurrentFaction^=1;
 else if(mode==8)const_cast<OriginalVehicle*>(w->vehicle(0))->x^=2;
 else if(mode==9)const_cast<OriginalMapTile*>(w->tile(0))->owner^=1;
 else if(mode==10)const_cast<OriginalPrototype*>(w->prototype(0))->name[0]^=1;
 else if(mode==11)const_cast<OriginalFactionDefinition*>(w->faction_definition(1))->filename[0]^=1;
 else if(mode==12)const_cast<OriginalGameView&>(w->game()).map.info.flat=!w->game().map.info.flat;
 else if(mode==13)const_cast<OriginalGameView&>(w->game()).bases.size^=1;
 const size_t n=w->original_size();uint8_t* out=(uint8_t*)malloc(n);uint8_t* again=(uint8_t*)malloc(n);if(!out || !again)return 20;memset(out,0x5a,n);ProductionSnapshotReceipt receipt;memset(&receipt,0x6b,sizeof receipt);uint8_t frozen[sizeof receipt];memcpy(frozen,&receipt,sizeof receipt);
 auto error=encode_production_snapshot(*w,out,n,receipt);
 if(mode>=2 && mode<=13){if(error!=ProductionSnapshotError::unsupported_change || memcmp(frozen,&receipt,sizeof receipt))return 21;for(size_t i=0;i<n;++i)if(out[i]!=0x5a)return 21;}
 else {
  if(error!=ProductionSnapshotError::ok || receipt.bytes!=n || receipt.runtime_seed!=0x2468ace0 || memcmp(save,w->original_bytes(),sn))return 21;
  if(reloaded.load(out,n,receipt.runtime_seed)!=WorldLoadError::ok)return 21;
  ProductionSnapshotReceipt r;if(encode_production_snapshot(reloaded,again,n,r)!=ProductionSnapshotError::ok || memcmp(out,again,n) || r.runtime_seed!=receipt.runtime_seed)return 21;
  if(mode>=100){
   if(continued.load_local(out,n,text,tn,profile,expansion,human,receipt.runtime_seed)!=LocalOrderError::ok)return 21;
   FacilityOrderPlan p,q;auto a=session.prepare_facility(0,3,p),b=continued.prepare_facility(0,3,q);if(a!=b)return 21;
   if(a==LocalOrderError::ok){if(p.preview().previous_item!=q.preview().previous_item || p.preview().mineral_baseline!=q.preview().mineral_baseline || p.preview().mineral_after!=q.preview().mineral_after)return 21;FacilityOrderReceipt x,y;if(session.commit(p,x)!=LocalOrderError::ok || continued.commit(q,y)!=LocalOrderError::ok || x.changed!=y.changed)return 21;
    if(encode_production_snapshot(*session.world(),out,n,r)!=ProductionSnapshotError::ok || encode_production_snapshot(*continued.world(),again,n,r)!=ProductionSnapshotError::ok || memcmp(out,again,n))return 21;
   }
  }
  // Error boundaries never alter output or receipt. Existing baseline bytes
  // and all owned decoded arrays are forbidden destinations.
  memset(again,0x5a,n);memset(&receipt,0x6b,sizeof receipt);memcpy(frozen,&receipt,sizeof receipt);
  if(encode_production_snapshot(*w,again,n-1,receipt)!=ProductionSnapshotError::bad_buffer || memcmp(frozen,&receipt,sizeof receipt))return 21;
  for(size_t i=0;i<n;++i)if(again[i]!=0x5a)return 21;
  if(encode_production_snapshot(*w,const_cast<OriginalWorld*>(w),n,receipt)!=ProductionSnapshotError::bad_buffer || encode_production_snapshot(*w,const_cast<OriginalPrototype*>(w->prototype(0)),n,receipt)!=ProductionSnapshotError::bad_buffer || encode_production_snapshot(*w,const_cast<OriginalFactionState*>(w->faction(0)),n,receipt)!=ProductionSnapshotError::bad_buffer || encode_production_snapshot(*w,const_cast<OriginalMapTile*>(w->tile(0)),n,receipt)!=ProductionSnapshotError::bad_buffer || encode_production_snapshot(*w,again,n,*reinterpret_cast<ProductionSnapshotReceipt*>(const_cast<OriginalFactionDefinition*>(w->faction_definition(0))))!=ProductionSnapshotError::bad_buffer)return 21;
  if(w->base(0) && encode_production_snapshot(*w,const_cast<OriginalBase*>(w->base(0)),n,receipt)!=ProductionSnapshotError::bad_buffer)return 21;
  if(w->vehicle(0) && encode_production_snapshot(*w,const_cast<OriginalVehicle*>(w->vehicle(0)),n,receipt)!=ProductionSnapshotError::bad_buffer)return 21;
  if(encode_production_snapshot(*w,const_cast<uint8_t*>(w->original_bytes()),n,receipt)!=ProductionSnapshotError::bad_buffer || encode_production_snapshot(*w,again,n,*reinterpret_cast<ProductionSnapshotReceipt*>(again))!=ProductionSnapshotError::bad_buffer || encode_production_snapshot(*w,nullptr,n,receipt)!=ProductionSnapshotError::bad_buffer)return 21;
 }
 if(!emit((uint32_t)error))return 20;
 if(error==ProductionSnapshotError::ok && fwrite(out,1,n,stdout)!=n)return 20;
 free(out);free(again);free(save);free(text);return 0;
}
