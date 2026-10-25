/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in original_map.h. */
#include "original_map.h"
namespace alpha {
static uint32_t word(const uint8_t* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int32_t signed_word(const uint8_t* p){uint32_t n=word(p);return n<=0x7fffffffu?(int32_t)n:-1-(int32_t)(~n);}
MapReadError read_original_map(const uint8_t* bytes,size_t length,OriginalMapView& output){
 const uint8_t magic[]={'T','E','R','R','A','N','M','A','P',0,0x1a};
 if(!bytes || length<sizeof(magic))return MapReadError::bad_header;
 for(unsigned i=0;i<sizeof(magic);++i)if(bytes[i]!=magic[i])return MapReadError::bad_header;
 if(length<15)return MapReadError::truncated;
 if(word(bytes+11)!=5)return MapReadError::unsupported_version;
 if(length<15+original_map_metadata_size)return MapReadError::truncated;
 const uint8_t* m=bytes+15;const int32_t width=signed_word(m),height=signed_word(m+4);
 if(width<2 || width>4096 || (width&1) || height<1 || height>4096)return MapReadError::bad_dimensions;
 // These limits make all count/length operations safe even with32-bit size_t.
 const uint32_t count=(uint32_t)height*((uint32_t)width/2);
 const uint32_t ax=((uint32_t)width+4)/5,ay=((uint32_t)height+4)/5,ac=ay*((ax+1)/2);
 const size_t tile_bytes=(size_t)count*original_map_tile_size;
 const size_t required=15+original_map_metadata_size+tile_bytes+ac;
 if(length<required)return MapReadError::truncated;
 if(length>required)return MapReadError::trailing_data;
 const int32_t landmarks=signed_word(m+32);
 if(landmarks<0 || landmarks>64)return MapReadError::bad_landmarks;
 OriginalMapView result{};auto& info=result.info;
 info.width=(uint32_t)width;info.height=(uint32_t)height;info.tile_count=count;
 info.abstract_width=ax;info.abstract_height=ay;info.abstract_count=ac;info.random_seed=word(m+8);
 info.sea_level=signed_word(m+12);info.council_sea_level=signed_word(m+16);info.flat=word(m+28)!=0;
 info.landmark_count=(uint32_t)landmarks;
 info.stored_counts_differ=word(m+20)!=count || word(m+0xa24)!=ax || word(m+0xa28)!=ay || word(m+0xa2c)!=ac;
 for(unsigned i=0;i<7;++i)info.world_settings[i]=signed_word(m+0xa30+i*4);
 for(uint32_t i=0;i<info.landmark_count;++i){const uint8_t* p=m+36+i*40;
  auto& landmark=info.landmarks[i];landmark.x=signed_word(p);landmark.y=signed_word(p+4);
  landmark.raw_coordinate_on_map=landmark.x>=0 && landmark.x<width && landmark.y>=0 && landmark.y<height && !((landmark.x+landmark.y)&1);
  if(!landmark.raw_coordinate_on_map)++info.off_map_landmarks;
  for(unsigned j=0;j<32;++j)landmark.name[j]=(char)p[8+j];
  landmark.name[32]=0;
 }
 result.metadata=m;result.tiles=m+original_map_metadata_size;result.abstract=result.tiles+tile_bytes;
 output=result;return MapReadError::ok;
}
MapReadError original_map_tile(const OriginalMapView& view,size_t index,OriginalMapTile& output){
 if(!view.tiles || index>=view.info.tile_count)return MapReadError::bad_index;
 const uint8_t* p=view.tiles+index*original_map_tile_size;OriginalMapTile tile;
 tile.climate=p[0];tile.contour=p[1];tile.val2=p[2];tile.region=p[3];tile.visibility=p[4];tile.val3=p[5];tile.unknown=p[6];
 tile.owner=p[7]<=127?(int8_t)p[7]:(int8_t)(-1-(int32_t)(255-p[7]));
 tile.items=word(p+8);tile.landmarks=word(p+12);
 for(unsigned i=0;i<7;++i)tile.visible_items[i]=word(p+16+i*4);
 output=tile;return MapReadError::ok;
}
const char* map_read_error_name(MapReadError e){
 switch(e){case MapReadError::ok:return "ok";case MapReadError::bad_header:return "not an original TERRANMAP";
 case MapReadError::unsupported_version:return "unsupported map version";case MapReadError::bad_dimensions:return "invalid map dimensions";
 case MapReadError::bad_landmarks:return "invalid landmark table";case MapReadError::truncated:return "incomplete map file";
 case MapReadError::trailing_data:return "unexpected bytes after map";case MapReadError::bad_index:return "tile outside map";
 }return "unknown map error";
}
}
