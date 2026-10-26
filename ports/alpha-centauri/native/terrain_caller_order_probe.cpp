/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_caller_order.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int32_t get(const uint8_t*p){return(int32_t)((uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24);}
static void put(uint32_t x){for(unsigned k=0;k<4;++k)putchar((uint8_t)(x>>(k*8)));}
int main(int argc,char**argv){if(argc!=2)return 20;FILE*f=fopen(argv[1],"rb");if(!f)return 20;if(fseek(f,0,2)){fclose(f);return 20;}long n=ftell(f);rewind(f);if(n<=0||n>64*1024*1024){fclose(f);return 20;}auto*p=(uint8_t*)malloc(n);if(!p){fclose(f);return 20;}bool ok=fread(p,1,n,f)==(size_t)n;fclose(f);alpha::OriginalWorld w;ok=ok&&w.load(p,n,1)==alpha::WorldLoadError::ok;free(p);if(!ok)return 20;
 alpha::TerrainObservationRuntime v{0,0,1,0,0,0,0,alpha::TerrainSourceEdition::smacx,true};alpha::TerrainCallerRuntime c{0,-256,256,true};alpha::TerrainCallerOrder answer{};answer.pass_minimum_light=99;const auto before=answer;c.supplied=false;if(alpha::plan_world_terrain_order(w,0,0,v,c,answer)!=alpha::TerrainCallerStatus::missing_runtime||memcmp(&answer,&before,sizeof answer))return 20;c.supplied=true;
 const void*q[]={&w,w.original_bytes(),w.tile(0),w.vehicle(0),w.base(0),w.prototype(0),w.faction(0),w.faction_definition(0),&c,&v};for(auto a:q)if(a&&alpha::plan_world_terrain_order(w,0,0,v,c,*reinterpret_cast<alpha::TerrainCallerOrder*>((uintptr_t)a&~uintptr_t(3)))!=alpha::TerrainCallerStatus::invalid_input)return 20;
 uint8_t b[36];while(true){size_t count=fread(b,1,sizeof b,stdin);if(!count)break;if(count!=sizeof b)return 20;int x=get(b),y=get(b+4);v.viewer=get(b+8);v.draw_flags=(uint32_t)get(b+12);v.game_rules=(uint32_t)get(b+16);v.edition=(alpha::TerrainSourceEdition)get(b+20);c={get(b+24),get(b+28),get(b+32),true};answer=before;auto status=alpha::plan_world_terrain_order(w,x,y,v,c,answer);if(status!=alpha::TerrainCallerStatus::resolved&&memcmp(&answer,&before,sizeof answer))return 20;put((uint32_t)status);if(status==alpha::TerrainCallerStatus::resolved){put((uint32_t)answer.path);put(answer.pass_minimum_light);put(answer.pass_maximum_light);put(answer.exit_minimum_light);put(answer.exit_maximum_light);put(answer.land_clamp_before_observation);}else for(unsigned i=0;i<6;++i)put(0);}
 return ferror(stdin)?20:0;
}
