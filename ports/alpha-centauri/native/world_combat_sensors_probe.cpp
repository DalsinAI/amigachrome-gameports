/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_combat_sensors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static bool put(uint32_t n){uint8_t v[4];for(unsigned i=0;i<4;++i)v[i]=(uint8_t)(n>>(8*i));return fwrite(v,1,4,stdout)==4;}
int main(int argc,char**argv){
 if(argc!=2)return 20;
 FILE*f=fopen(argv[1],"rb");if(!f)return 20;
 if(fseek(f,0,SEEK_END)){fclose(f);return 20;}const long size=ftell(f);rewind(f);
 if(size<=0||(unsigned long)size>64u*1024u*1024u){fclose(f);return 20;}
 auto*bytes=(uint8_t*)malloc((size_t)size);if(!bytes){fclose(f);return 20;}
 const bool good=fread(bytes,1,(size_t)size,f)==(size_t)size;fclose(f);if(!good){free(bytes);return 20;}
 alpha::OriginalWorld w;if(w.load(bytes,(size_t)size,1)!=alpha::WorldLoadError::ok){free(bytes);return 20;}
 uint32_t out=0x5a5a5a5au;
 const int invalid_factions[2]={0,8};
 for(int faction:invalid_factions)if(alpha::world_combat_sensor_mask(w,faction,0,0,out)!=alpha::CombatError::bad_state||out!=0x5a5a5a5au){free(bytes);return 21;}
 if(alpha::world_combat_sensor_mask(w,1,1,0,out)!=alpha::CombatError::bad_state||out!=0x5a5a5a5au){free(bytes);return 21;}
 if(alpha::world_combat_sensor_mask(w,1,0,0,*static_cast<uint32_t*>(static_cast<void*>(const_cast<uint8_t*>(w.original_bytes()))))!=alpha::CombatError::bad_buffer){free(bytes);return 21;}
 const auto&m=w.game().map.info;
 for(int32_t faction=1;faction<=7;++faction)for(size_t i=0;i<m.tile_count;++i){
  const int32_t y=(int32_t)(i/(m.width/2)),x=(int32_t)(2*(i%(m.width/2)))+(y&1);out=0x5a5a5a5au;
  const auto e=alpha::world_combat_sensor_mask(w,faction,x,y,out);
  if(e!=alpha::CombatError::ok&&out!=0x5a5a5a5au){free(bytes);return 21;}
  if(!put((uint32_t)e)||!put(out)){free(bytes);return 20;}
 }
 if(memcmp(bytes,w.original_bytes(),(size_t)size)){free(bytes);return 21;}free(bytes);return fflush(stdout)?20:0;
}
