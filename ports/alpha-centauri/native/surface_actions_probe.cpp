/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "local_player.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static uint8_t* read(const char* path,size_t& n){FILE*f=fopen(path,"rb");if(!f)return nullptr;fseek(f,0,SEEK_END);long z=ftell(f);rewind(f);if(z<=0){fclose(f);return nullptr;}auto*p=(uint8_t*)malloc((size_t)z);if(!p||fread(p,1,(size_t)z,f)!=(size_t)z){free(p);p=nullptr;}fclose(f);n=(size_t)z;return p;}
static uint32_t digest(const OriginalWorld&w){uint32_t h=2166136261u;auto add=[&](const void*p,size_t n){auto*b=(const uint8_t*)p;while(n--)h=(h^*b++)*16777619u;};
 add(&w,sizeof w);add(w.original_bytes(),w.original_size());add(w.tile(0),w.game().map.info.tile_count*sizeof(OriginalMapTile));add(w.vehicle(0),w.game().vehicle_count*sizeof(OriginalVehicle));add(w.base(0),w.game().base_count*sizeof(OriginalBase));add(w.prototype(0),512*sizeof(OriginalPrototype));add(w.faction(0),8*sizeof(OriginalFactionState));add(w.faction_definition(0),8*sizeof(OriginalFactionDefinition));return h;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"surface line%d: %s\n",__LINE__,#x);return 21;}++checks;}while(0)
int main(int argc,char**argv){
 if(argc!=8)return 20;
 FILE*exists=fopen(argv[7],"rb");if(exists){fclose(exists);return 20;}
 size_t rn=0,sn=0;auto*r=read(argv[1],rn);auto*s=read(argv[2],sn);if(!r||!s)return 20;
 const size_t id=(size_t)strtoul(argv[3],nullptr,10);const int x=atoi(argv[4]),y=atoi(argv[5]),route=atoi(argv[6]);unsigned checks=0;
 OriginalGameView view{};CHECK(read_original_game(s,sn,view)==GameReadError::ok);
 uint8_t*out=nullptr;size_t bytes=sn;StepResult result{};LocalOrderError order=LocalOrderError::ok;bool dirty=false;
 if(route>=10 && route<=11){
  LocalPlayer player,resumed;CHECK(player.load(s,sn,r,rn,0xabcdef01)==LocalOrderError::ok);
  const auto*v=player.world()->vehicle(id);CHECK(v);const int sx=v->x,sy=v->y;int dx=x-sx;const int width=(int)view.map.info.width;
  if(!view.map.info.flat){if(dx>width/2)dx-=width;else if(dx< -width/2)dx+=width;}
  LocalStepReceipt receipt{};order=player.step(id,dx,y-sy,receipt);result=receipt.result;
  if(route==11){CHECK(order==LocalOrderError::ok&&receipt.changed);CHECK(player.step(id,-dx,sy-y,receipt)==LocalOrderError::ok&&receipt.changed);CHECK(player.step(id,dx,y-sy,receipt)==LocalOrderError::ok&&!receipt.changed&&receipt.result.status==StepStatus::exhausted);result=receipt.result;}
  dirty=player.dirty();bytes=player.checkpoint_size();out=(uint8_t*)malloc(bytes);CHECK(out);CHECK(player.checkpoint(out,bytes)==LocalSnapshotError::ok);
  CHECK(resumed.resume(out,bytes,r,rn)==LocalOrderError::ok);CHECK(!resumed.dirty());auto*second=(uint8_t*)malloc(bytes);CHECK(second);CHECK(resumed.checkpoint(second,bytes)==LocalSnapshotError::ok);CHECK(!memcmp(out,second,bytes));free(second);
 }else if(route==12){
  LocalProductionSession plain,land,surface,other;LocalStepCommand command,oldland;LocalStepReceipt receipt{};
  CHECK(plain.load_local(s,sn,r,rn,Variant::base,false,view.player,0xabcdef01)==LocalOrderError::ok);
  CHECK(!plain.land_actions_enabled()&&!plain.surface_actions_enabled());CHECK(plain.prepare_surface_step(id,x,y,command)==LocalOrderError::unsupported);
  CHECK(land.load_local_land_actions(s,sn,r,rn,Variant::base,false,view.player,0xabcdef01)==LocalOrderError::ok);
  CHECK(land.land_actions_enabled()&&!land.surface_actions_enabled());CHECK(land.prepare_surface_step(id,x,y,command)==LocalOrderError::unsupported);
  CHECK(land.prepare_clear_land_step(id,x,y,oldland)==LocalOrderError::ok);CHECK(land.commit_clear_land_step(oldland,receipt)==LocalOrderError::ok&&!receipt.changed&&receipt.result.status==StepStatus::unsupported);
  CHECK(surface.load_local_surface_actions(s,sn,r,rn,Variant::base,false,view.player,0xabcdef01)==LocalOrderError::ok);
  CHECK(surface.surface_actions_enabled()&&surface.land_actions_enabled());CHECK(surface.prepare_surface_step(id,x,y,command)==LocalOrderError::ok);const uint64_t revision=surface.revision();
  const auto*world=surface.world();const uint32_t unchanged=digest(*world);
  const void*regions[]={&surface,world,world->original_bytes(),world->tile(0),world->vehicle(0),world->prototype(0),world->faction(0),world->faction_definition(0)};
  for(const void*region:regions){void*at=const_cast<void*>(region);
   CHECK(surface.prepare_surface_step(id,x,y,*static_cast<LocalStepCommand*>(at))==LocalOrderError::bad_state);
   CHECK(surface.commit_surface_step(command,*static_cast<LocalStepReceipt*>(at))==LocalOrderError::bad_state);
   CHECK(surface.commit_surface_step(*static_cast<LocalStepCommand*>(at),receipt)==LocalOrderError::bad_state);
   CHECK(surface.revision()==revision&&digest(*world)==unchanged);
  }
  void*command_at=&command;CHECK(surface.commit_surface_step(command,*static_cast<LocalStepReceipt*>(command_at))==LocalOrderError::bad_state);
  CHECK(surface.revision()==revision&&digest(*world)==unchanged);
  LocalStepCommand absent;CHECK(surface.commit_surface_step(absent,receipt)==LocalOrderError::stale_plan);
  CHECK(surface.commit_clear_land_step(command,receipt)==LocalOrderError::unsupported&&surface.revision()==revision);
  CHECK(surface.prepare_clear_land_step(id,x,y,oldland)==LocalOrderError::ok);CHECK(surface.commit_surface_step(oldland,receipt)==LocalOrderError::unsupported&&surface.revision()==revision);
  CHECK(other.load_local_surface_actions(s,sn,r,rn,Variant::base,false,view.player,0xabcdef01)==LocalOrderError::ok);CHECK(other.commit_surface_step(command,receipt)==LocalOrderError::stale_plan);
  const auto*land_before=land.world();const auto land_revision=land.revision();
  CHECK(land.load_local_surface_actions(s,sn,r,rn,Variant::base,false,view.player,0xabcdef01,1)==LocalOrderError::no_memory);
  CHECK(land.world()==land_before&&land.revision()==land_revision&&!land.surface_actions_enabled());
  CHECK(surface.commit_surface_step(command,receipt)==LocalOrderError::ok&&receipt.changed&&surface.revision()==revision+1);result=receipt.result;
  CHECK(surface.commit_surface_step(command,receipt)==LocalOrderError::stale_plan);
  CHECK(surface.set_suspended(true)==LocalOrderError::ok);CHECK(surface.prepare_surface_step(id,x,y,command)==LocalOrderError::suspended);CHECK(surface.commit_surface_step(oldland,receipt)==LocalOrderError::suspended);CHECK(surface.set_suspended(false)==LocalOrderError::ok);
  CHECK(surface.prepare_surface_step(id,x,y,command)==LocalOrderError::ok);CHECK(surface.load_local_surface_actions(s,sn,r,rn,Variant::base,false,view.player,0xabcdef01)==LocalOrderError::ok);CHECK(surface.commit_surface_step(command,receipt)==LocalOrderError::stale_plan);
  // Encode the already moved independent surface session, preserving capability
  // separation on the main session through the checks above.
  CHECK(other.prepare_surface_step(id,x,y,command)==LocalOrderError::ok);CHECK(other.commit_surface_step(command,receipt)==LocalOrderError::ok&&receipt.changed);
  out=(uint8_t*)malloc(bytes);CHECK(out);LocalSnapshotReceipt encoded{};CHECK(encode_local_actions_snapshot(*other.world(),out,bytes,encoded)==LocalSnapshotError::ok);
  CHECK(other.load_local_land_actions(s,sn,r,rn,Variant::base,false,view.player,0xabcdef01)==LocalOrderError::ok);
  CHECK(other.land_actions_enabled()&&!other.surface_actions_enabled());CHECK(other.prepare_surface_step(id,x,y,command)==LocalOrderError::unsupported);
 }else{
  auto*defs=(UnitDefinitions*)calloc(1,sizeof(UnitDefinitions));auto*tech=(Rules*)calloc(1,sizeof(Rules));CHECK(defs&&tech);BasicRules basic{};WorldRules wr{};
  CHECK(read_technologies(r,rn,*tech)==Error::ok);CHECK(read_unit_definitions(r,rn,*tech,Variant::base,*defs)==Error::ok);CHECK(read_basic_rules(r,rn,*tech,Variant::base,basic,wr)==Error::ok);
  OriginalWorld w;CHECK(w.load(s,sn,0xabcdef01)==WorldLoadError::ok);const uint32_t before=digest(w);
  result=route==1?WorldActions::clear_land_step(w,*defs,*tech,id,view.player,basic.move_rate_roads,x,y):WorldActions::clear_sea_step(w,*defs,*tech,id,view.player,route==2?0:basic.move_rate_roads,x,y);
  CHECK(result.status==StepStatus::moved||digest(w)==before);CHECK(w.runtime_seed()==0xabcdef01);CHECK(!memcmp(w.original_bytes(),s,sn));
  out=(uint8_t*)malloc(bytes);CHECK(out);LocalSnapshotReceipt encoded{};CHECK(encode_local_actions_snapshot(w,out,bytes,encoded)==LocalSnapshotError::ok);free(defs);free(tech);
 }
 FILE*f=fopen(argv[7],"wb");CHECK(f);CHECK(fwrite(out,1,bytes,f)==bytes);CHECK(fclose(f)==0);
 printf("surface order=%s status=%s price=%lu dirty=%d bytes=%lu checks=%u\n",local_order_error_name(order),step_status_name(result.status),(unsigned long)result.price,dirty,(unsigned long)bytes,checks);
 free(out);free(r);free(s);return 0;
}
