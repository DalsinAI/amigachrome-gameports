/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "local_production_session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <new>
using namespace alpha;
static unsigned char* read(const char* path,size_t& n){n=0;FILE* f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long length=ftell(f);if(length<=0 || length>32*1024*1024 || fseek(f,0,SEEK_SET)){fclose(f);return nullptr;}auto* data=(unsigned char*)malloc((size_t)length);if(!data){fclose(f);return nullptr;}if(fread(data,1,(size_t)length,f)!=(size_t)length){free(data);fclose(f);return nullptr;}fclose(f);n=(size_t)length;return data;}
static bool emit(int32_t value){uint32_t n=(uint32_t)value;unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
/* Within-process object-representation snapshots test absence of unrelated
 * writes, never serialization or cross-endian interpretation of struct bytes. */
class Snapshot {
 public:
  explicit Snapshot(const OriginalWorld& w):raw_(nullptr),records_(nullptr),raw_size_(w.original_size()),seed_(w.runtime_seed()),nb_(w.game().base_count),nv_(w.game().vehicle_count),nt_(w.game().map.info.tile_count){
   raw_=(unsigned char*)malloc(raw_size_);size_t size=nb_*sizeof(OriginalBase)+8*sizeof(OriginalFactionState)+nv_*sizeof(OriginalVehicle)+nt_*sizeof(OriginalMapTile)+512*sizeof(OriginalPrototype)+8*sizeof(OriginalFactionDefinition);
   records_=(unsigned char*)malloc(size);if(!raw_ || !records_)return;memcpy(view_,&w.game(),sizeof view_);memcpy(raw_,w.original_bytes(),raw_size_);unsigned char* p=records_;
   for(size_t i=0;i<nb_;++i){memcpy(p,w.base(i),sizeof(OriginalBase));p+=sizeof(OriginalBase);}for(size_t i=0;i<8;++i){memcpy(p,w.faction(i),sizeof(OriginalFactionState));p+=sizeof(OriginalFactionState);}for(size_t i=0;i<nv_;++i){memcpy(p,w.vehicle(i),sizeof(OriginalVehicle));p+=sizeof(OriginalVehicle);}for(size_t i=0;i<nt_;++i){memcpy(p,w.tile(i),sizeof(OriginalMapTile));p+=sizeof(OriginalMapTile);}for(size_t i=0;i<512;++i){memcpy(p,w.prototype(i),sizeof(OriginalPrototype));p+=sizeof(OriginalPrototype);}for(size_t i=0;i<8;++i){memcpy(p,w.faction_definition(i),sizeof(OriginalFactionDefinition));p+=sizeof(OriginalFactionDefinition);}
  }
  ~Snapshot(){free(raw_);free(records_);}
  bool matches(const OriginalWorld& w,int32_t changed_base=-1,int32_t changed_faction=-1,int32_t changed_unit=-1)const{
   if(!raw_ || !records_ || w.original_size()!=raw_size_ || w.runtime_seed()!=seed_ || w.game().base_count!=nb_ || w.game().vehicle_count!=nv_ || w.game().map.info.tile_count!=nt_ || memcmp(raw_,w.original_bytes(),raw_size_) || memcmp(view_,&w.game(),sizeof view_))return false;
   const unsigned char* p=records_;
   for(size_t i=0;i<nb_;++i){const auto* now=(const unsigned char*)w.base(i);if((int32_t)i==changed_base){const size_t a=offsetof(OriginalBase,minerals_accumulated),b=offsetof(OriginalBase,queue_items);if(memcmp(p,now,a) || memcmp(p+a+4,now+a+4,b-a-4) || memcmp(p+b+4,now+b+4,sizeof(OriginalBase)-b-4))return false;}else if(memcmp(p,now,sizeof(OriginalBase)))return false;p+=sizeof(OriginalBase);}
   for(size_t i=0;i<8;++i){const auto* now=(const unsigned char*)w.faction(i);if((int32_t)i==changed_faction && changed_unit>=0){const size_t offset=offsetof(OriginalFactionState,units_queue)+(size_t)changed_unit;if(memcmp(p,now,offset) || memcmp(p+offset+1,now+offset+1,sizeof(OriginalFactionState)-offset-1))return false;}else if(memcmp(p,now,sizeof(OriginalFactionState)))return false;p+=sizeof(OriginalFactionState);}
   for(size_t i=0;i<nv_;++i){if(memcmp(p,w.vehicle(i),sizeof(OriginalVehicle)))return false;p+=sizeof(OriginalVehicle);}for(size_t i=0;i<nt_;++i){if(memcmp(p,w.tile(i),sizeof(OriginalMapTile)))return false;p+=sizeof(OriginalMapTile);}for(size_t i=0;i<512;++i){if(memcmp(p,w.prototype(i),sizeof(OriginalPrototype)))return false;p+=sizeof(OriginalPrototype);}for(size_t i=0;i<8;++i){if(memcmp(p,w.faction_definition(i),sizeof(OriginalFactionDefinition)))return false;p+=sizeof(OriginalFactionDefinition);}return true;
  }
 private:
  unsigned char *raw_,*records_;unsigned char view_[sizeof(OriginalGameView)];size_t raw_size_;uint32_t seed_;size_t nb_,nv_,nt_;
};
static LocalProductionSession session,other;
int main(int argc,char** argv){
 if(argc!=6)return 20;
 Variant profile;if(!strcmp(argv[1],"base"))profile=Variant::base;else if(!strcmp(argv[1],"expansion"))profile=Variant::alien_crossfire;else return 20;
 if(strcmp(argv[2],"0") && strcmp(argv[2],"1"))return 20;
 const bool expansion=argv[2][0]=='1';const int32_t human=atoi(argv[3]);size_t tn,sn;unsigned char* text=read(argv[4],tn);if(!text)return 20;unsigned char* save=read(argv[5],sn);if(!save){free(text);return 20;}
 FacilityOrderPlan empty_start;FacilityOrderReceipt empty_receipt;memset(&empty_receipt,0x5a,sizeof empty_receipt);unsigned char empty_frozen[sizeof empty_receipt];memcpy(empty_frozen,&empty_receipt,sizeof empty_receipt);if(session.world() || session.prepare_facility(0,1,empty_start)!=LocalOrderError::bad_state || session.commit(empty_start,empty_receipt)!=LocalOrderError::bad_state || memcmp(empty_frozen,&empty_receipt,sizeof empty_receipt) || session.set_suspended(true)!=LocalOrderError::bad_state)return 21;
 auto loaded=session.load_local(save,sn,text,tn,profile,expansion,human,0x2468ace0);if(!emit((int32_t)loaded)){free(save);free(text);return 20;}if(loaded!=LocalOrderError::ok){free(save);free(text);return 0;}
 const int count=profile==Variant::base?72:134;const int targets[]={1,2,3,4,5,7,10,11,12,14,20,21,23,25,27,28,29,30,32,34,39,65,70,71,101,102,133};
 const size_t nb=session.world()->game().base_count;unsigned accepted=0,sequences=0;
 for(size_t base=0;base<nb;++base)for(unsigned j=0;j<sizeof targets/sizeof targets[0];++j){const int facility=targets[j];if(facility>=count)continue;
  loaded=session.load_local(save,sn,text,tn,profile,expansion,human,0x2468ace0);if(loaded!=LocalOrderError::ok)return 21;
  const auto* w=session.world();Snapshot before(*w);const auto* b=w->base(base);const int32_t old=b->queue_items[0],minerals=b->minerals_accumulated,baseline=b->minerals_accumulated_2;const unsigned old_counter=old>=0 && old<512?w->faction((size_t)human)->units_queue[(size_t)old]:0;const uint64_t revision=session.revision();
  FacilityOrderPlan plan;unsigned char plan_before[sizeof plan];memcpy(plan_before,&plan,sizeof plan);auto error=session.prepare_facility((int32_t)base,facility,plan);FacilityOrderReceipt receipt;memset(&receipt,0x5a,sizeof receipt);unsigned char receipt_before[sizeof receipt];memcpy(receipt_before,&receipt,sizeof receipt);
  int32_t commit_error=-1,changed=0,loss=0;
  FacilityOrderPlan alternative;bool have_alternative=false;
  if(!before.matches(*session.world()) || session.revision()!=revision)return 21;
  if(error==LocalOrderError::ok){
   ++accepted;const auto& p=plan.preview();if(p.base!=(int32_t)base || p.facility!=facility || p.previous_item!=old || p.proposed_item!=-facility || p.mineral_before!=minerals || p.mineral_baseline!=baseline)return 21;
   // Unrelated modal/suspension transitions invalidate confirmation plans.
   if(session.set_suspended(true)!=LocalOrderError::ok || session.commit(plan,receipt)!=LocalOrderError::suspended || memcmp(&receipt,receipt_before,sizeof receipt) || !before.matches(*session.world()))return 21;
   FacilityOrderPlan modal;unsigned char modal_before[sizeof modal];memcpy(modal_before,&modal,sizeof modal);if(session.prepare_facility((int32_t)base,facility,modal)!=LocalOrderError::suspended || memcmp(&modal,modal_before,sizeof modal))return 21;
   if(session.set_suspended(false)!=LocalOrderError::ok || session.commit(plan,receipt)!=LocalOrderError::stale_plan || !before.matches(*session.world()))return 21;
   if(session.prepare_facility((int32_t)base,facility,plan)!=LocalOrderError::ok)return 21;
   FacilityOrderPlan sibling=plan;
   for(unsigned k=0;k<sizeof targets/sizeof targets[0];++k)if(targets[k]<count && targets[k]!=facility && session.prepare_facility((int32_t)base,targets[k],alternative)==LocalOrderError::ok && alternative.preview().changes_state){have_alternative=true;break;}
   // A private plan from one owning session cannot authorize another session.
   if(other.load_local(save,sn,text,tn,profile,expansion,human,0x2468ace0)!=LocalOrderError::ok)return 21;
   Snapshot foreign(*other.world());if(other.commit(plan,receipt)!=LocalOrderError::stale_plan || !foreign.matches(*other.world()))return 21;other.close();
   const uint64_t before_commit=session.revision();const uint8_t saved_byte=save[0],text_byte=text[0];save[0]^=0xff;text[0]^=0xff;
   auto committed=session.commit(plan,receipt);commit_error=(int32_t)committed;save[0]=saved_byte;text[0]=text_byte;if(committed!=LocalOrderError::ok)return 21;changed=receipt.changed?1:0;loss=receipt.applied.mineral_loss;if(session.revision()!=before_commit+(receipt.changed?1u:0u))return 21;
   if(receipt.changed!=p.changes_state || receipt.repaint_required!=receipt.changed || receipt.persistence_required!=receipt.changed || receipt.revision!=session.revision())return 21;
   if(!before.matches(*session.world(),changed?(int32_t)base:-1,changed?human:-1,changed && old>=0?old:-1))return 21;
   FacilityOrderReceipt untouched;memset(&untouched,0x5a,sizeof untouched);unsigned char frozen[sizeof untouched];memcpy(frozen,&untouched,sizeof untouched);auto again=session.commit(sibling,untouched);if(changed?(again!=LocalOrderError::stale_plan || memcmp(&untouched,frozen,sizeof untouched)):(again!=LocalOrderError::ok || untouched.changed))return 21;
   if(session.world()->base(base)->queue_items[0]!=-facility || session.world()->base(base)->minerals_accumulated!=receipt.applied.mineral_after)return 21;
  }else if(memcmp(&plan,plan_before,sizeof plan) || !before.matches(*session.world()) || session.revision()!=revision)return 21;
  const auto* after=session.world()->base(base);const unsigned new_counter=old>=0 && old<512?session.world()->faction((size_t)human)->units_queue[(size_t)old]:0;
  const int32_t fields[]={(int32_t)base,facility,(int32_t)error,commit_error,changed,old,after->queue_items[0],minerals,baseline,loss,after->minerals_accumulated,(int32_t)old_counter,(int32_t)new_counter};for(int32_t n:fields)if(!emit(n))return 20;
  // A second different order continues from the live decoded queue. Its
  // penalty is recomputed from the unchanged original baseline/last category,
  // not subtracted again from the first order's current minerals.
  if(changed && have_alternative){
   const auto expected=alternative.preview();Snapshot after_first(*session.world());FacilityOrderReceipt second_receipt;memset(&second_receipt,0x5a,sizeof second_receipt);unsigned char second_frozen[sizeof second_receipt];memcpy(second_frozen,&second_receipt,sizeof second_receipt);
   if(session.commit(alternative,second_receipt)!=LocalOrderError::stale_plan || memcmp(second_frozen,&second_receipt,sizeof second_receipt) || !after_first.matches(*session.world()))return 21;
   FacilityOrderPlan second_plan;if(session.prepare_facility((int32_t)base,expected.facility,second_plan)!=LocalOrderError::ok)return 21;
   const auto next=second_plan.preview();if(next.previous_item!=-facility || next.mineral_baseline!=baseline || next.mineral_loss!=expected.mineral_loss || next.mineral_after!=expected.mineral_after || !next.changes_state)return 21;
   const auto current_revision=session.revision();if(session.commit(second_plan,second_receipt)!=LocalOrderError::ok || !second_receipt.changed || session.revision()!=current_revision+1 || !after_first.matches(*session.world(),(int32_t)base))return 21;
   if(session.world()->base(base)->queue_items[0]!=-expected.facility || session.world()->base(base)->minerals_accumulated!=expected.mineral_after)return 21;
   ++sequences;
  }
 }
 // Invalid tokens/loads cannot mutate an existing live session or its rules.
 Snapshot before(*session.world());const uint64_t revision=session.revision();FacilityOrderPlan empty;FacilityOrderReceipt receipt;memset(&receipt,0x5a,sizeof receipt);unsigned char frozen[sizeof receipt];memcpy(frozen,&receipt,sizeof receipt);if(session.commit(empty,receipt)!=LocalOrderError::stale_plan || memcmp(frozen,&receipt,sizeof receipt))return 21;
 if(session.load_local(save,sn,(const uint8_t*)"bad",3,profile,expansion,human,1)!=LocalOrderError::bad_rules || !before.matches(*session.world()) || session.revision()!=revision)return 21;
 if(session.load_local(save,sn,text,tn,profile,expansion,human,1,1)!=LocalOrderError::no_memory || !before.matches(*session.world()) || session.revision()!=revision)return 21;
 if(session.load_local(save,sn-1,text,tn,profile,expansion,human,1)!=LocalOrderError::bad_world || !before.matches(*session.world()) || session.revision()!=revision)return 21;
 if(session.load_local(save,sn,text,tn,profile,expansion,human==7?1:human+1,1)!=LocalOrderError::not_human || !before.matches(*session.world()) || session.revision()!=revision)return 21;
 const int32_t invalid[][2]={{-1,1},{2147483647,1},{0,0},{0,-1},{0,count},{0,2147483647}};
 for(unsigned i=0;i<6;++i){FacilityOrderPlan invalid_plan;unsigned char frozen_plan[sizeof invalid_plan];memcpy(frozen_plan,&invalid_plan,sizeof invalid_plan);if(session.prepare_facility(invalid[i][0],invalid[i][1],invalid_plan)!=LocalOrderError::bad_state || memcmp(frozen_plan,&invalid_plan,sizeof invalid_plan) || !before.matches(*session.world()) || session.revision()!=revision)return 21;}
 if(session.load_local(save,sn,text,tn,(Variant)7,expansion,human,1)!=LocalOrderError::bad_state || session.load_local(save,sn,text,tn,profile,expansion,0,1)!=LocalOrderError::bad_state || !before.matches(*session.world()) || session.revision()!=revision)return 21;
 // Reload, close and address reuse cannot revive an older private plan.
 FacilityOrderPlan pending;bool issued=false;
 for(size_t base=0;base<nb && !issued;++base)for(unsigned j=0;j<sizeof targets/sizeof targets[0];++j)if(targets[j]<count && session.prepare_facility((int32_t)base,targets[j],pending)==LocalOrderError::ok){issued=true;break;}
 if(issued){
  const uint64_t previous_revision=session.revision();if(session.load_local(save,sn,text,tn,profile,expansion,human,0x2468ace0)!=LocalOrderError::ok || session.revision()!=previous_revision+1)return 21;
  Snapshot reloaded(*session.world());if(session.commit(pending,receipt)!=LocalOrderError::stale_plan || !reloaded.matches(*session.world()))return 21;
  session.close();if(session.world() || session.commit(pending,receipt)!=LocalOrderError::bad_state)return 21;
  alignas(LocalProductionSession) unsigned char storage[sizeof(LocalProductionSession)];auto* reused=new(storage)LocalProductionSession;
  if(reused->load_local(save,sn,text,tn,profile,expansion,human,0x2468ace0)!=LocalOrderError::ok)return 21;
  FacilityOrderPlan expired;if(reused->prepare_facility(pending.preview().base,pending.preview().facility,expired)!=LocalOrderError::ok)return 21;
  reused->~LocalProductionSession();reused=new(storage)LocalProductionSession;
  if(reused->load_local(save,sn,text,tn,profile,expansion,human,0x2468ace0)!=LocalOrderError::ok)return 21;
  Snapshot replacement(*reused->world());if(reused->commit(expired,receipt)!=LocalOrderError::stale_plan || !replacement.matches(*reused->world()))return 21;reused->~LocalProductionSession();
 }
 if(!emit((int32_t)accepted) || !emit((int32_t)sequences))return 20;
 free(save);free(text);return 0;
}
