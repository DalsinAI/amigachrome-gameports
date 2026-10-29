/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "local_production_session.h"
#include "local_actions_test_state.h"
#include "movement_snapshot.h"
#include "production_snapshot.h"
#include "local_actions_snapshot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>
using namespace alpha;
static uint8_t* read(const char* path,size_t& n){FILE* f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long size=ftell(f);if(size<=0 || size>32*1024*1024 || fseek(f,0,SEEK_SET)){fclose(f);return nullptr;}auto* p=(uint8_t*)malloc((size_t)size);n=(size_t)size;if(!p || fread(p,1,n,f)!=n){free(p);p=nullptr;}fclose(f);return p;}
static bool emit(uint32_t n){uint8_t b[4];for(unsigned i=0;i<4;++i){b[i]=(uint8_t)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
static LocalProductionSession session,other;
static OriginalWorld reference;
static Rules tech;
static UnitDefinitions units;
static LocalOrderError import(const uint8_t* save,size_t sn,const uint8_t* text,size_t tn){return session.load_local_land_actions(save,sn,text,tn,Variant::base,false,5,0xabcdef01);}
static bool fresh_reference(const uint8_t* save,size_t sn){return reference.load(save,sn,0xabcdef01)==WorldLoadError::ok;}
static bool apply_reference_production(const FacilityOrderReceipt& r){if(!r.changed)return true;auto* b=const_cast<OriginalBase*>(reference.base((size_t)r.applied.base));if(!b)return false;const int old=b->queue_items[0];if(old>=0)const_cast<OriginalFactionState*>(reference.faction(5))->units_queue[(size_t)old]--;b->queue_items[0]=r.applied.proposed_item;b->minerals_accumulated=r.applied.mineral_after;return true;}
int main(int argc,char** argv){
 if(argc!=7)return 20;
 size_t sn=0,tn=0;auto* text=read(argv[1],tn);auto* save=read(argv[2],sn);if(!text || !save)return 20;
 if(read_technologies(text,tn,tech)!=Error::ok || read_unit_definitions(text,tn,tech,Variant::base,units)!=Error::ok)return 21;
 const size_t vehicle=(size_t)strtoul(argv[3],nullptr,10);const int x=atoi(argv[4]),y=atoi(argv[5]),mode=atoi(argv[6]);
 LocalStepCommand empty_command;LocalStepReceipt empty_output;memset(&empty_output,0x5a,sizeof empty_output);unsigned char empty_before[sizeof empty_output];memcpy(empty_before,&empty_output,sizeof empty_output);
 if(session.prepare_clear_land_step(0,0,0,empty_command)!=LocalOrderError::bad_state || session.commit_clear_land_step(empty_command,empty_output)!=LocalOrderError::bad_state || memcmp(empty_before,&empty_output,sizeof empty_output))return 21;
 const auto loaded=import(save,sn,text,tn);if(!emit((uint32_t)loaded))return 20;if(loaded!=LocalOrderError::ok){free(save);free(text);return 0;}
 if(!session.land_actions_enabled() || !fresh_reference(save,sn) || !alpha_test::same_world(*session.world(),reference))return 21;
 LocalStepCommand command;unsigned char before_command[sizeof command];memcpy(before_command,&command,sizeof command);const uint64_t rev=session.revision();const auto prepared=session.prepare_clear_land_step(vehicle,x,y,command);
 LocalStepReceipt receipt;memset(&receipt,0x5a,sizeof receipt);unsigned char before_receipt[sizeof receipt];memcpy(before_receipt,&receipt,sizeof receipt);int committed=-1,status=-1,changed=0;uint32_t price=0;
 if(prepared==LocalOrderError::ok){
  if(command.intent().vehicle!=vehicle || command.intent().x!=x || command.intent().y!=y)return 21;
  // No cross-session use, including sessions at the same numeric revision.
  if(other.load_local_land_actions(save,sn,text,tn,Variant::base,false,5,0xabcdef01)!=LocalOrderError::ok || other.commit_clear_land_step(command,receipt)!=LocalOrderError::stale_plan || memcmp(before_receipt,&receipt,sizeof receipt) || !alpha_test::same_world(*other.world(),reference))return 21;
  other.close();
  if(session.set_suspended(true)!=LocalOrderError::ok || session.commit_clear_land_step(command,receipt)!=LocalOrderError::suspended || memcmp(before_receipt,&receipt,sizeof receipt))return 21;
  LocalStepCommand modal;if(session.prepare_clear_land_step(vehicle,x,y,modal)!=LocalOrderError::suspended || session.set_suspended(false)!=LocalOrderError::ok || session.commit_clear_land_step(command,receipt)!=LocalOrderError::stale_plan || !alpha_test::same_world(*session.world(),reference))return 21;
  if(session.prepare_clear_land_step(vehicle,x,y,command)!=LocalOrderError::ok)return 21;
  const uint64_t before=session.revision();const auto expected=WorldActions::clear_land_step(reference,units,tech,vehicle,5,3,x,y);
  const uint8_t saved=save[0],rules=text[0];save[0]^=0xff;text[0]^=0xff;const auto e=session.commit_clear_land_step(command,receipt);save[0]=saved;text[0]=rules;
  if(e!=LocalOrderError::ok || receipt.result.status!=expected.status || receipt.result.price!=expected.price || receipt.changed!=(expected.status==StepStatus::moved) || receipt.repaint_required!=receipt.changed || receipt.persistence_required!=receipt.changed || receipt.revision!=session.revision() || session.revision()!=before+(receipt.changed?1u:0u) || !alpha_test::same_world(*session.world(),reference))return 21;
  committed=(int)e;status=(int)receipt.result.status;price=receipt.result.price;changed=receipt.changed;
  if(changed && (session.commit_clear_land_step(command,receipt)!=LocalOrderError::stale_plan || !alpha_test::same_world(*session.world(),reference)))return 21;
 }else if(memcmp(before_command,&command,sizeof command) || session.revision()!=rev || !alpha_test::same_world(*session.world(),reference))return 21;
 const int values[]={(int)prepared,committed,status,(int)price,changed};for(int value:values)if(!emit((uint32_t)value))return 20;
 if(mode==1){
  // Reset before the interleaved production/movement state machine.
  if(import(save,sn,text,tn)!=LocalOrderError::ok || !fresh_reference(save,sn))return 21;
  const auto* unit=session.world()->vehicle(0);if(!unit)return 21;const int sx=unit->x,sy=unit->y;
  FacilityOrderPlan first;LocalStepCommand move;
  if(session.prepare_facility(0,1,first)!=LocalOrderError::ok || !first.preview().changes_state || session.prepare_clear_land_step(0,sx+1,sy+1,move)!=LocalOrderError::ok)return 21;
  if(session.commit_clear_land_step(move,receipt)!=LocalOrderError::ok || !receipt.changed || WorldActions::clear_land_step(reference,units,tech,0,5,3,sx+1,sy+1).status!=StepStatus::moved || !alpha_test::same_world(*session.world(),reference))return 21;
  FacilityOrderReceipt production;memset(&production,0x5a,sizeof production);unsigned char frozen[sizeof production];memcpy(frozen,&production,sizeof production);
  if(session.commit(first,production)!=LocalOrderError::stale_plan || memcmp(frozen,&production,sizeof production) || !alpha_test::same_world(*session.world(),reference))return 21;
  LocalStepCommand back;if(session.prepare_clear_land_step(0,sx,sy,back)!=LocalOrderError::ok || session.prepare_facility(0,1,first)!=LocalOrderError::ok || session.commit(first,production)!=LocalOrderError::ok || !production.changed || !apply_reference_production(production) || !alpha_test::same_world(*session.world(),reference))return 21;
  memset(&receipt,0x5a,sizeof receipt);memcpy(before_receipt,&receipt,sizeof receipt);
  if(session.commit_clear_land_step(back,receipt)!=LocalOrderError::stale_plan || memcmp(before_receipt,&receipt,sizeof receipt) || !alpha_test::same_world(*session.world(),reference))return 21;
  if(session.prepare_clear_land_step(0,sx,sy,back)!=LocalOrderError::ok || session.commit_clear_land_step(back,receipt)!=LocalOrderError::ok || !receipt.changed || WorldActions::clear_land_step(reference,units,tech,0,5,3,sx,sy).status!=StepStatus::moved || !alpha_test::same_world(*session.world(),reference))return 21;
  // Exhaustion is a checked no-write rejection and keeps a current facility
  // confirmation valid. No fake next-turn refill is provided.
  if(session.prepare_facility(0,2,first)!=LocalOrderError::ok || session.prepare_clear_land_step(0,sx+1,sy+1,move)!=LocalOrderError::ok || session.commit_clear_land_step(move,receipt)!=LocalOrderError::ok || receipt.changed || receipt.result.status!=StepStatus::exhausted || !alpha_test::same_world(*session.world(),reference) || session.commit(first,production)!=LocalOrderError::ok || !production.changed || !apply_reference_production(production) || !alpha_test::same_world(*session.world(),reference))return 21;
  // Separate serializers must reject this mixed state rather than save half.
  const size_t bytes=session.world()->original_size();auto* buffer=(uint8_t*)malloc(bytes);if(!buffer)return 20;memset(buffer,0x5a,bytes);MovementSnapshotReceipt mr{7,8};ProductionSnapshotReceipt pr{9,10};
  if(encode_movement_snapshot(*session.world(),buffer,bytes,mr)!=SnapshotError::unsupported_change || mr.bytes!=7 || mr.runtime_seed!=8 || encode_production_snapshot(*session.world(),buffer,bytes,pr)!=ProductionSnapshotError::unsupported_change || pr.bytes!=9 || pr.runtime_seed!=10)return 21;
  for(size_t i=0;i<bytes;++i)if(buffer[i]!=0x5a)return 21;
  // The combined boundary preserves both record families. Reload into a new
  // owning session, retaining expenditure and explicit runtime RNG, then
  // continue production and compare complete re-encoded original containers.
  LocalSnapshotReceipt combined;
  if(encode_local_actions_snapshot(*session.world(),buffer,bytes,combined)!=LocalSnapshotError::ok || combined.bytes!=bytes || combined.runtime_seed!=0xabcdef01 || other.load_local_land_actions(buffer,bytes,text,tn,Variant::base,false,5,combined.runtime_seed)!=LocalOrderError::ok || !alpha_test::same_world(*session.world(),*other.world(),false))return 21;
  LocalStepCommand still_exhausted,loaded_exhausted;LocalStepReceipt loaded_receipt;
  const auto sr=session.revision(),orr=other.revision();
  if(session.prepare_clear_land_step(0,sx+1,sy+1,still_exhausted)!=LocalOrderError::ok || other.prepare_clear_land_step(0,sx+1,sy+1,loaded_exhausted)!=LocalOrderError::ok || session.commit_clear_land_step(still_exhausted,receipt)!=LocalOrderError::ok || other.commit_clear_land_step(loaded_exhausted,loaded_receipt)!=LocalOrderError::ok || receipt.result.status!=StepStatus::exhausted || loaded_receipt.result.status!=StepStatus::exhausted || receipt.changed || loaded_receipt.changed || session.revision()!=sr || other.revision()!=orr || !alpha_test::same_world(*session.world(),*other.world(),false))return 21;
  FacilityOrderPlan next_production,loaded_production;FacilityOrderReceipt loaded_order;
  if(session.prepare_facility(0,3,next_production)!=LocalOrderError::ok || other.prepare_facility(0,3,loaded_production)!=LocalOrderError::ok || next_production.preview().previous_item!=-2 || next_production.preview().mineral_baseline!=loaded_production.preview().mineral_baseline || next_production.preview().mineral_after!=loaded_production.preview().mineral_after || session.commit(next_production,production)!=LocalOrderError::ok || other.commit(loaded_production,loaded_order)!=LocalOrderError::ok || !production.changed || !loaded_order.changed || !apply_reference_production(production) || !alpha_test::same_world(*session.world(),reference) || !alpha_test::same_world(*session.world(),*other.world(),false))return 21;
  auto* loaded_buffer=(uint8_t*)malloc(bytes);if(!loaded_buffer)return 20;LocalSnapshotReceipt second_snapshot;
  if(encode_local_actions_snapshot(*session.world(),buffer,bytes,combined)!=LocalSnapshotError::ok || encode_local_actions_snapshot(*other.world(),loaded_buffer,bytes,second_snapshot)!=LocalSnapshotError::ok || memcmp(buffer,loaded_buffer,bytes) || combined.runtime_seed!=second_snapshot.runtime_seed)return 21;
  free(loaded_buffer);other.close();
  if(!emit(1) || !emit((uint32_t)bytes) || !emit(combined.runtime_seed) || fwrite(buffer,1,bytes,stdout)!=bytes)return 20;
  free(buffer);
  // Failed action initialization preserves both the existing state and rules.
  const auto stable_revision=session.revision();
  if(session.load_local_land_actions(save,sn,text,tn,Variant::base,false,5,1,1)!=LocalOrderError::no_memory || session.revision()!=stable_revision || !session.land_actions_enabled() || !alpha_test::same_world(*session.world(),reference))return 21;
  size_t chassis=tn;for(size_t i=0;i+8<=tn;++i)if(!memcmp(text+i,"#CHASSIS",8)){chassis=i;break;}if(chassis==tn)return 21;
  const uint8_t chassis_byte=text[chassis+1];text[chassis+1]='!';const auto missing_units=session.load_local_land_actions(save,sn,text,tn,Variant::base,false,5,1);text[chassis+1]=chassis_byte;
  if(missing_units!=LocalOrderError::bad_rules || session.revision()!=stable_revision || !session.land_actions_enabled() || !alpha_test::same_world(*session.world(),reference))return 21;
  if(session.load_local_land_actions(save,sn,(const uint8_t*)"bad",3,Variant::base,false,5,1)!=LocalOrderError::bad_rules || !alpha_test::same_world(*session.world(),reference))return 21;
  if(session.load_local_land_actions(save,sn,text,tn,Variant::alien_crossfire,true,5,1)!=LocalOrderError::unsupported || !alpha_test::same_world(*session.world(),reference))return 21;
  if(session.prepare_clear_land_step(0,sx+1,sy+1,move)!=LocalOrderError::ok)return 21;
  if(session.load_local(save,sn,text,tn,Variant::base,false,5,0xabcdef01)!=LocalOrderError::ok || session.land_actions_enabled() || session.commit_clear_land_step(move,receipt)!=LocalOrderError::stale_plan || session.prepare_clear_land_step(0,sx+1,sy+1,move)!=LocalOrderError::unsupported)return 21;
  alignas(LocalProductionSession) unsigned char storage[sizeof(LocalProductionSession)];auto* reused=new(storage)LocalProductionSession;
  if(reused->load_local_land_actions(save,sn,text,tn,Variant::base,false,5,0xabcdef01)!=LocalOrderError::ok)return 21;
  LocalStepCommand expired;if(reused->prepare_clear_land_step(0,sx+1,sy+1,expired)!=LocalOrderError::ok)return 21;
  reused->~LocalProductionSession();reused=new(storage)LocalProductionSession;
  if(reused->load_local_land_actions(save,sn,text,tn,Variant::base,false,5,0xabcdef01)!=LocalOrderError::ok || reused->commit_clear_land_step(expired,receipt)!=LocalOrderError::stale_plan || !alpha_test::same_world(*reused->world(),*session.world()))return 21;
  reused->close();if(reused->commit_clear_land_step(expired,receipt)!=LocalOrderError::bad_state)return 21;reused->~LocalProductionSession();
 }
 free(save);free(text);return 0;
}
