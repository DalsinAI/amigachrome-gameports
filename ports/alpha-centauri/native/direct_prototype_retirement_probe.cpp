/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "direct_prototype_retirement.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static bool emit(uint32_t n){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
static uint32_t whole(const alpha::OriginalWorld& w){uint32_t h=2166136261u;auto add=[&](const void* data,size_t n){const auto* p=(const unsigned char*)data;for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;};add(&w,sizeof w);add(w.original_bytes(),w.original_size());add(w.current_monuments().data,w.current_monuments().size);add(w.tile(0),w.game().map.info.tile_count*sizeof(alpha::OriginalMapTile));add(w.vehicle(0),w.game().vehicle_count*sizeof(alpha::OriginalVehicle));add(w.base(0),w.game().base_count*sizeof(alpha::OriginalBase));add(w.prototype(0),512*sizeof(alpha::OriginalPrototype));add(w.faction(0),8*sizeof(alpha::OriginalFactionState));add(w.faction_definition(0),8*sizeof(alpha::OriginalFactionDefinition));return h;}
int main(int argc,char** argv){if(argc!=5)return 20;FILE* f=fopen(argv[1],"rb");if(!f)return 20;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<=0)return 20;auto* b=(uint8_t*)malloc((size_t)n);if(!b || fread(b,1,(size_t)n,f)!=(size_t)n)return 20;fclose(f);alpha::OriginalWorld w;if(w.load(b,(size_t)n,123)!=alpha::WorldLoadError::ok)return 20;free(b);uint32_t before=whole(w);auto variant=(alpha::Variant)atoi(argv[2]);int faction=atoi(argv[3]),id=atoi(argv[4]);alpha::DirectRetirementProposal p;auto e=alpha::world_direct_prototype_retirement(w,variant,faction,id,p);if(!emit((uint32_t)e))return 20;
 if(e==alpha::DirectRetirementError::ok){
  if(!p.ready() || p.count()!=w.game().vehicle_count)return 21;
  const auto* ptr=p.vehicle(0);const int32_t cursor=p.history().next_slot;
  if(alpha::world_direct_prototype_retirement(w,variant,0,id,p)!=alpha::DirectRetirementError::bad_state || !p.ready() || p.vehicle(0)!=ptr || p.history().next_slot!=cursor)return 21;
  if(!emit(p.history().next_slot) || !emit(p.queues().unit_flags()) || !emit(p.count()) || !emit(p.queues().count()) || !emit((uint32_t)p.replacement()) || !emit(p.conversions()) || fwrite(p.history().table,1,4096,stdout)!=4096 || fwrite(p.faction_state().units_active,1,512,stdout)!=512)return 20;
  alpha::OriginalFactionState state=p.faction_state();state.unk_103=w.faction(faction)->unk_103;memcpy(state.units_active,w.faction(faction)->units_active,512);if(memcmp(&state,w.faction(faction),sizeof state))return 21;
  for(size_t i=0;i<p.count();++i){alpha::OriginalVehicle copy=*p.vehicle(i);copy.unit_id=w.vehicle(i)->unit_id;if(memcmp(&copy,w.vehicle(i),sizeof copy) || !emit((uint32_t)p.vehicle(i)->unit_id))return 21;}
  for(size_t i=0;i<p.queues().count();++i){const auto* q=p.queues().base(i);const auto* original=w.base(i);if(!q || !original)return 21;auto copy=*q;copy.queue_size=original->queue_size;memcpy(copy.queue_items,original->queue_items,sizeof copy.queue_items);if(memcmp(&copy,original,sizeof copy) || !emit((uint32_t)q->queue_size))return 21;for(unsigned j=0;j<10;++j)if(!emit((uint32_t)q->queue_items[j]))return 20;}
 }else if(p.ready() || p.count())return 21;
 if(whole(w)!=before)return 21;
 const void* aliases[]={&w,w.original_bytes(),w.current_monuments().data,w.tile(0),w.vehicle(0),w.base(0),w.prototype(0),w.faction(0),w.faction_definition(0)};
 for(const void* a:aliases)if(a){auto& out=*(alpha::DirectRetirementProposal*)const_cast<void*>(a);if(alpha::world_direct_prototype_retirement(w,variant,faction,id,out)!=alpha::DirectRetirementError::bad_state)return 21;}
 return whole(w)==before?0:21;
}
