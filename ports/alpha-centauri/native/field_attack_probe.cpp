/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "field_attack_snapshot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static alpha::UnitDefinitions definitions;
static alpha::Rules technologies;
static alpha::BasicRules basic;
static alpha::WorldRules world_rules;
static uint8_t*read_file(const char*path,size_t&n,size_t max){FILE*f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long size=ftell(f);rewind(f);if(size<=0||(size_t)size>max){fclose(f);return nullptr;}auto*p=(uint8_t*)malloc((size_t)size);if(!p){fclose(f);return nullptr;}bool ok=fread(p,1,(size_t)size,f)==(size_t)size;fclose(f);if(!ok){free(p);return nullptr;}n=(size_t)size;return p;}
static uint32_t hash(uint32_t h,const void*p,size_t n){const auto*b=(const uint8_t*)p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
static uint32_t digest(const alpha::OriginalWorld&w){uint32_t h=hash(2166136261u,w.original_bytes(),w.original_size());h=hash(h,w.vehicle(0),w.game().vehicle_count*sizeof(alpha::OriginalVehicle));h=hash(h,w.tile(0),w.game().map.info.tile_count*sizeof(alpha::OriginalMapTile));h=hash(h,w.base(0),w.game().base_count*sizeof(alpha::OriginalBase));return hash(h,w.faction(0),8*sizeof(alpha::OriginalFactionState));}
int main(int argc,char**argv){
 if(argc!=8)return 20;
 size_t n=0;auto*data=read_file(argv[1],n,1048576);if(!data)return 20;
 bool good=alpha::read_technologies(data,n,technologies)==alpha::Error::ok&&alpha::read_unit_definitions(data,n,technologies,alpha::Variant::base,definitions)==alpha::Error::ok&&alpha::read_basic_rules(data,n,technologies,alpha::Variant::base,basic,world_rules)==alpha::Error::ok;free(data);if(!good)return 20;
 data=read_file(argv[2],n,64u*1024u*1024u);if(!data)return 20;alpha::OriginalWorld w;const auto loaded=w.load(data,n,0xabcdef01u);free(data);if(loaded!=alpha::WorldLoadError::ok)return 20;
 const uint32_t seed=(uint32_t)strtoul(argv[3],nullptr,0);const size_t attacker=(size_t)strtoul(argv[4],nullptr,0),defender=(size_t)strtoul(argv[5],nullptr,0);
 const bool live=!strcmp(argv[6],"live"),short_buffer=!strcmp(argv[6],"short"),overflow=!strcmp(argv[6],"ledger-overflow");
 if(live){
  auto*v=const_cast<alpha::OriginalVehicle*>(w.vehicle(w.game().vehicle_count-1));if(!v)return 20;v->moves_spent=1;v->order=24;v->waypoint_x[0]=2;v->waypoint_y[0]=0;
  auto*b=const_cast<alpha::OriginalBase*>(w.base(0));if(!b)return 20;b->queue_items[0]=-3;b->minerals_accumulated=11;
 }
 if(!strcmp(argv[6],"unsealed")){auto*faction=const_cast<alpha::OriginalFactionState*>(w.faction(5));if(!faction)return 20;++faction->energy_credits;}
 const auto before=digest(w);alpha::FieldAttackStage stage{};stage.combat_random.state=seed;for(unsigned i=0;i<8;++i){stage.combat_wins[i]=(int32_t)(100+i);stage.combat_losses[i]=(int32_t)(20+i);}if(overflow)for(auto&v:stage.combat_wins)v=2147483647;
 const auto stage_before=stage;alpha::FieldAttackReceipt receipt;memset(&receipt,0x5a,sizeof receipt);uint8_t saved[sizeof receipt];memcpy(saved,&receipt,sizeof receipt);
 auto*output=(uint8_t*)malloc(n+16);if(!output)return 20;memset(output,0x5a,n+16);
 if(alpha::encode_field_attack_snapshot(w,definitions,technologies,basic,attacker,defender,stage,const_cast<uint8_t*>(w.original_bytes()),n,receipt)!=alpha::FieldAttackError::bad_buffer||memcmp(saved,&receipt,sizeof receipt)||digest(w)!=before){free(output);return 21;}
 const size_t capacity=short_buffer?alpha::field_attack_snapshot_size(w)-1:n;
 const auto e=alpha::encode_field_attack_snapshot(w,definitions,technologies,basic,attacker,defender,stage,output,capacity,receipt);
 if(digest(w)!=before||memcmp(&stage,&stage_before,sizeof stage)){free(output);return 21;}
 size_t written=0;
 if(e==alpha::FieldAttackError::ok){written=receipt.bytes;if(written!=n-52){free(output);return 21;}for(size_t i=written;i<n+16;++i)if(output[i]!=0x5a){free(output);return 21;}}
 else {if(memcmp(saved,&receipt,sizeof receipt)){free(output);return 21;}for(size_t i=0;i<n+16;++i)if(output[i]!=0x5a){free(output);return 21;}}
 FILE*f=fopen(argv[7],"wb");if(!f){free(output);return 20;}good=fwrite(output,1,written,f)==written;good=fclose(f)==0&&good;free(output);if(!good)return 20;
 printf("error=%s",alpha::field_attack_error_name(e));
 if(e==alpha::FieldAttackError::ok)printf(" bytes=%lu seed=%lu loser=%ld winner=%ld wf=%ld lf=%ld selected=%ld promoted=%d promotion_draws=%lu rounds=%lu draws=%lu ad=%ld dd=%ld wins=%ld losses=%ld movement_seed=%lu monument=%d first=%ld year=%ld",(unsigned long)receipt.bytes,(unsigned long)receipt.next.combat_random.state,(long)receipt.removed_before,(long)receipt.winner_after,(long)receipt.winner_faction,(long)receipt.loser_faction,(long)receipt.selected_vehicle_after,(int)receipt.promoted,(unsigned long)receipt.promotion_draws,(unsigned long)receipt.combat.rounds,(unsigned long)receipt.combat.random_draws,(long)receipt.combat.attacker_damage,(long)receipt.combat.defender_damage,(long)receipt.next.combat_wins[receipt.winner_faction],(long)receipt.next.combat_losses[receipt.loser_faction],(unsigned long)receipt.movement_random_seed,(int)receipt.monument.changed,(long)receipt.monument.first_faction,(long)receipt.monument.year);
 puts(" unchanged=1 rc=0");return 0;
}
