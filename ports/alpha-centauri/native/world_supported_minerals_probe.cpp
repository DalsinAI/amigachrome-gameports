/* SPDX-License-Identifier: GPL-3.0-or-later
 * Read-only support policy report for real original rule/world inputs.
 */
#include "world_base_support.h"
#include "world_base_minerals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static alpha::OriginalWorld world;
static alpha::Rules technologies;
static alpha::UnitDefinitions definitions;
static alpha::BasicRules basic;
static alpha::WorldRules world_rules;
static alpha::ResourceInfo resources;
static unsigned char* read_file(const char*path,size_t&n){
 FILE*f=fopen(path,"rb");if(!f)return nullptr;
 if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}
 long len=ftell(f);if(len<=0||len>64*1024*1024||fseek(f,0,SEEK_SET)){fclose(f);return nullptr;}
 auto*b=(unsigned char*)malloc((size_t)len);if(!b){fclose(f);return nullptr;}
 n=(size_t)len;const bool ok=fread(b,1,n,f)==n;fclose(f);if(!ok){free(b);return nullptr;}return b;
}
int main(int argc,char**argv){
 if(argc!=3)return 20;
 size_t n=0;auto*b=read_file(argv[1],n);if(!b)return 20;
 bool ok=alpha::read_technologies(b,n,technologies)==alpha::Error::ok&&alpha::read_unit_definitions(b,n,technologies,alpha::Variant::base,definitions)==alpha::Error::ok&&alpha::read_basic_rules(b,n,technologies,alpha::Variant::base,basic,world_rules)==alpha::Error::ok&&alpha::read_resource_info(b,n,resources)==alpha::Error::ok;
 memset(b,0xff,n);free(b);if(!ok)return 20;
 b=read_file(argv[2],n);if(!b)return 20;const auto loaded=world.load(b,n,0xabcdef01u);memset(b,0xff,n);free(b);if(loaded!=alpha::WorldLoadError::ok)return 20;
 const auto&g=world.game();auto*states=g.vehicle_count?(uint32_t*)malloc((size_t)g.vehicle_count*sizeof(uint32_t)):nullptr;if(g.vehicle_count&&!states)return 20;
 for(size_t id=0;id<g.base_count;++id){
  alpha::SupportResult result{};const auto error=alpha::world_base_support_policy(world,definitions,id,result,states,g.vehicle_count);
  printf("SUPPORT %lu %u",(unsigned long)id,(unsigned)error);
  if(error==alpha::SupportError::ok){
   const auto*f=world.faction((size_t)world.base(id)->faction_id);
   printf(" pending=%ld police=%ld supported=%ld paid=%ld cost=%ld pacifism=%ld drones=%ld skips=%ld free=%ld unit_cost=%ld",(long)f->SE_support_pending,(long)f->SE_police_pending,(long)result.supported,(long)result.paid_units,(long)result.mineral_cost,(long)result.pacifism_units,(long)result.drone_units,(long)result.free_skips,(long)result.free_allowance,(long)result.cost_per_paid_unit);
  }
  putchar('\n');
  if(error==alpha::SupportError::ok)for(size_t i=0;i<g.vehicle_count;++i){const auto*v=world.vehicle(i);if(v->home_base_id==(int32_t)id&&v->faction_id==world.base(id)->faction_id)printf("STATE %lu %lu %08lx %08lx\n",(unsigned long)id,(unsigned long)i,(unsigned long)v->state,(unsigned long)states[i]);}
  if(error==alpha::SupportError::ok){
   const alpha::UpkeepWorldMineralStage stage{{alpha::Variant::base,false,false,{0,false},0,0},result.mineral_cost};
   alpha::UpkeepWorldMineralResult minerals{};
   const auto e=alpha::world_base_minerals(world,technologies,basic,world_rules,resources,id,stage,minerals);
   printf("MINERALS %lu %u",(unsigned long)id,(unsigned)e);
   if(e==alpha::StableIntakeError::ok){
    if(minerals.facts.forces_maintenance!=result.mineral_cost)return 21;
    const auto&m=minerals.minerals;
    printf(" intake=%ld consumption=%ld surplus=%ld final=%ld eco=%ld support=%ld convoy_to=%ld convoy_from=%ld",(long)m.intake,(long)m.consumption,(long)m.surplus,(long)m.surplus_final,(long)m.eco_damage,(long)minerals.facts.forces_maintenance,(long)minerals.facts.convoy_to,(long)minerals.facts.convoy_from);
   }
   putchar('\n');
  }
 }
 free(states);return 0;
}
