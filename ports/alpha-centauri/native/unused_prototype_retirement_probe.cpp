/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "unused_prototype_retirement.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static bool emit(uint32_t n){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
static uint32_t hash(const unsigned char* p,size_t n){uint32_t h=2166136261u;for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;return h;}
static uint32_t whole(const alpha::OriginalWorld& w){
 uint32_t h=2166136261u;auto add=[&](const void* data,size_t n){const auto* p=(const unsigned char*)data;for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;};
 add(&w,sizeof w);add(w.original_bytes(),w.original_size());add(w.current_monuments().data,w.current_monuments().size);
 add(w.tile(0),(size_t)w.game().map.info.tile_count*sizeof(alpha::OriginalMapTile));
 add(w.vehicle(0),(size_t)w.game().vehicle_count*sizeof(alpha::OriginalVehicle));add(w.base(0),(size_t)w.game().base_count*sizeof(alpha::OriginalBase));
 add(w.prototype(0),512*sizeof(alpha::OriginalPrototype));add(w.faction(0),8*sizeof(alpha::OriginalFactionState));add(w.faction_definition(0),8*sizeof(alpha::OriginalFactionDefinition));return h;
}
int main(int argc,char** argv){if(argc!=5)return 20;FILE* f=fopen(argv[1],"rb");if(!f)return 20;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<=0)return 20;auto* data=(unsigned char*)malloc((size_t)n);if(!data)return 20;if(fread(data,1,(size_t)n,f)!=(size_t)n)return 20;fclose(f);alpha::OriginalWorld w;if(w.load(data,(size_t)n,123)!=alpha::WorldLoadError::ok)return 20;free(data);const uint32_t before=whole(w);
 auto variant=(alpha::Variant)atoi(argv[2]);int faction=atoi(argv[3]),id=atoi(argv[4]);alpha::UnusedRetirementProposal p;auto e=alpha::world_unused_prototype_retirement(w,variant,faction,id,p);
 if(!emit((uint32_t)e))return 20;
 if(e==alpha::UnusedRetirementError::ok){
  if(!p.ready || p.faction!=faction || p.prototype_id!=id || p.queues.count()!=w.game().base_count)return 21;
  const uint32_t saved=hash(p.history.table,sizeof p.history.table);const auto* old=p.queues.base(0);const size_t count=p.queues.count();const uint16_t flags=p.queues.unit_flags();
  if(alpha::world_unused_prototype_retirement(w,variant,0,id,p)!=alpha::UnusedRetirementError::bad_state || !p.ready || p.queues.base(0)!=old || p.queues.count()!=count || p.queues.unit_flags()!=flags || hash(p.history.table,sizeof p.history.table)!=saved)return 21;
  if(w.game().vehicle_count){
   auto* v=const_cast<alpha::OriginalVehicle*>(w.vehicle(0));const int16_t original=v->unit_id;v->unit_id=(int16_t)id;
   const auto rejected=alpha::world_unused_prototype_retirement(w,variant,faction,id,p);v->unit_id=original;
   if(rejected!=alpha::UnusedRetirementError::live_unit_effects_required || !p.ready || p.queues.base(0)!=old || p.queues.count()!=count || p.queues.unit_flags()!=flags || hash(p.history.table,sizeof p.history.table)!=saved)return 21;
  }
  if(!emit(p.history.next_slot) || !emit(p.queues.unit_flags()) || !emit(p.queues.count()) || fwrite(p.history.table,1,sizeof p.history.table,stdout)!=sizeof p.history.table)return 20;
  for(size_t i=0;i<p.queues.count();++i){const auto* b=p.queues.base(i);const auto* baseline=w.base(i);alpha::OriginalBase copy=*b;copy.queue_size=baseline->queue_size;for(unsigned j=0;j<10;++j)copy.queue_items[j]=baseline->queue_items[j];if(memcmp(&copy,baseline,sizeof copy))return 21;if(!emit((uint32_t)b->queue_size))return 20;for(unsigned j=0;j<10;++j)if(!emit((uint32_t)b->queue_items[j]))return 20;}
 }else if(p.ready || p.queues.count() || p.faction!=-1 || p.prototype_id!=-1)return 21;
 if(whole(w)!=before)return 21;
 const void* aliases[]={&w,w.original_bytes(),w.current_monuments().data,w.tile(0),w.vehicle(0),w.base(0),w.prototype(0),w.faction(0),w.faction_definition(0)};
 for(const void* a:aliases)if(a){auto& fake=*(alpha::UnusedRetirementProposal*)const_cast<void*>(a);if(alpha::world_unused_prototype_retirement(w,variant,faction,id,fake)!=alpha::UnusedRetirementError::bad_state)return 21;}
 if(whole(w)!=before)return 21;
 return 0;
}
