/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_observation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int32_t get(const uint8_t*p){return(int32_t)((uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24);}
static void put(uint32_t x){for(unsigned k=0;k<4;++k)putchar((uint8_t)(x>>(k*8)));}
int main(int argc,char**argv){if(argc!=2)return 20;FILE*f=fopen(argv[1],"rb");if(!f)return 20;if(fseek(f,0,2)){fclose(f);return 20;}long n=ftell(f);rewind(f);if(n<=0||n>64*1024*1024){fclose(f);return 20;}auto*p=(uint8_t*)malloc(n);if(!p){fclose(f);return 20;}bool ok=fread(p,1,n,f)==(size_t)n;fclose(f);alpha::OriginalWorld w;ok=ok&&w.load(p,n,1)==alpha::WorldLoadError::ok;free(p);if(!ok)return 20;
 alpha::TerrainObservationRuntime c{0,-5,1,0,0,0,0,alpha::TerrainSourceEdition::smacx,true};alpha::TerrainObservation answer{};answer.displayed_items=99;const auto before=answer;c.supplied=false;if(alpha::observe_world_terrain(w,0,0,c,answer)!=alpha::TerrainObservationStatus::missing_runtime||memcmp(&answer,&before,sizeof answer))return 20;c.supplied=true;
 const void*q[]={&w,w.original_bytes(),w.tile(0),w.vehicle(0),w.base(0),w.prototype(0),w.faction(0),w.faction_definition(0),&c};for(auto v:q)if(v&&alpha::observe_world_terrain(w,0,0,c,*reinterpret_cast<alpha::TerrainObservation*>((uintptr_t)v&~uintptr_t(3)))!=alpha::TerrainObservationStatus::invalid_input)return 20;
 uint8_t b[40];while(true){size_t count=fread(b,1,sizeof b,stdin);if(!count)break;if(count!=sizeof b)return 20;int x=get(b),y=get(b+4);c={get(b+8),get(b+12),get(b+16),(uint32_t)get(b+20),(uint32_t)get(b+24),(uint32_t)get(b+28),(uint32_t)get(b+32),(alpha::TerrainSourceEdition)get(b+36),true};answer=before;auto status=alpha::observe_world_terrain(w,x,y,c,answer);if(status!=alpha::TerrainObservationStatus::resolved&&memcmp(&answer,&before,sizeof answer))return 20;put((uint32_t)status);if(status==alpha::TerrainObservationStatus::resolved){put(answer.displayed_items);put(answer.rainfall);put(answer.rockiness);put(answer.source_faction);put(answer.concealed);put(answer.visible);put(answer.landmarks_after);put(answer.far_profile);}else for(unsigned i=0;i<8;++i)put(0);}
 return 0;
}
