/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "original_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void word(uint32_t& h,uint32_t n){for(unsigned s=0;s<32;s+=8)h=(h^((n>>s)&255u))*16777619u;}
static uint32_t bytes(const uint8_t* p,size_t n){uint32_t h=2166136261u;for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;return h;}
int main(int argc,char** argv){
 if(argc!=2)return 20;
 FILE* f=fopen(argv[1],"rb");if(!f)return 20;
 if(fseek(f,0,SEEK_END)){fclose(f);return 20;}long length=ftell(f);rewind(f);
 if(length<0 || length>512L*1024*1024){fclose(f);return 20;}
 auto* data=(uint8_t*)malloc((size_t)length+1);if(!data){fclose(f);return 20;}
 bool read=fread(data,1,(size_t)length,f)==(size_t)length;fclose(f);
 alpha::OriginalMapView before{};before.info.width=123;before.metadata=data;
 alpha::OriginalMapView view;memcpy(&view,&before,sizeof(view));auto error=read?alpha::read_original_map(data,(size_t)length,view):alpha::MapReadError::truncated;
 if(error!=alpha::MapReadError::ok){if(memcmp(&view,&before,sizeof(view))){free(data);return 21;}printf("ERROR %s\n",alpha::map_read_error_name(error));free(data);return 20;}
 uint32_t digest=2166136261u,land=0,fungus=0,forests=0,rivers=0,rocky=0;
 for(size_t i=0;i<view.info.tile_count;++i){alpha::OriginalMapTile t;
  if(alpha::original_map_tile(view,i,t)!=alpha::MapReadError::ok){free(data);return 21;}
  const uint32_t fields[]={t.climate,t.contour,t.val2,t.region,t.visibility,t.val3,t.unknown,(uint32_t)(int32_t)t.owner,t.items,t.landmarks};
  for(auto value:fields)word(digest,value);
  for(auto value:t.visible_items)word(digest,value);
  bool is_land=(t.climate>>5)>=3;land+=is_land;fungus+=(t.items&0x20u)!=0 && (t.climate>>5)>=2;
  forests+=(t.items&0x200000u)!=0;rivers+=(t.items&0x80u)!=0;rocky+=is_land && (t.val3&0x80u)!=0;
 }
 alpha::OriginalMapTile sentinel{};sentinel.items=123, sentinel.owner=-128;
 alpha::OriginalMapTile outside;memcpy(&outside,&sentinel,sizeof(outside));
 if(alpha::original_map_tile(view,view.info.tile_count,outside)!=alpha::MapReadError::bad_index || memcmp(&outside,&sentinel,sizeof(outside))){free(data);return 21;}
 uint32_t info_digest=2166136261u;
 const uint32_t info_fields[]={view.info.width,view.info.height,view.info.tile_count,view.info.abstract_width,view.info.abstract_height,
  view.info.abstract_count,view.info.random_seed,(uint32_t)view.info.sea_level,(uint32_t)view.info.council_sea_level,
  (uint32_t)view.info.flat,view.info.landmark_count,(uint32_t)view.info.stored_counts_differ,view.info.off_map_landmarks};
 for(auto value:info_fields)word(info_digest,value);
 for(auto value:view.info.world_settings)word(info_digest,(uint32_t)value);
 uint32_t landmarks=2166136261u;
 for(uint32_t i=0;i<view.info.landmark_count;++i){const auto& lm=view.info.landmarks[i];word(landmarks,(uint32_t)lm.x);word(landmarks,(uint32_t)lm.y);word(landmarks,lm.raw_coordinate_on_map);
  for(unsigned j=0;j<32;++j)landmarks=(landmarks^(uint8_t)lm.name[j])*16777619u;}
 printf("width=%lu height=%lu tiles=%lu abstract=%lu seed=%08lx flat=%u landmarks=%lu counts_differ=%u off_map_landmarks=%lu land=%lu fungus=%lu forests=%lu rivers=%lu rocky=%lu tile_digest=%08lx info_digest=%08lx landmark_digest=%08lx metadata_digest=%08lx abstract_digest=%08lx rc=0\n",
  (unsigned long)view.info.width,(unsigned long)view.info.height,(unsigned long)view.info.tile_count,(unsigned long)view.info.abstract_count,
  (unsigned long)view.info.random_seed,(unsigned)view.info.flat,(unsigned long)view.info.landmark_count,(unsigned)view.info.stored_counts_differ,(unsigned long)view.info.off_map_landmarks,
  (unsigned long)land,(unsigned long)fungus,(unsigned long)forests,(unsigned long)rivers,(unsigned long)rocky,(unsigned long)digest,
  (unsigned long)info_digest,(unsigned long)landmarks,(unsigned long)bytes(view.metadata,alpha::original_map_metadata_size),(unsigned long)bytes(view.abstract,view.info.abstract_count));
 free(data);return 0;
}
