// SPDX-License-Identifier: GPL-3.0-or-later
#include "terrain_water_sprites.h"
#include <cstdlib>
#include <cstring>
namespace alpha {namespace {
bool overlap(const void*a,size_t n,const void*b,size_t m){if(!n||!m)return false;auto x=reinterpret_cast<uintptr_t>(a),y=reinterpret_cast<uintptr_t>(b);return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(x<y+m&&y<x+n);}
int slot(SeaLateOperation op,unsigned v){const auto k=static_cast<unsigned>(op);if(k<4||k>8||v>1||(k!=8&&v))return -1;return k==8?4+(int)v:(int)k-4;}
const SpriteCrop crops[6]={{506,64,100,62},{506,127,100,62},{506,253,100,62},{607,253,100,62},{708,253,100,62},{708,190,100,62}};
const SpriteTrim trims[6]={{9,54,54,48,100,62,26,0},{9,48,48,33,100,62,28,9},{9,63,63,38,100,62,16,2},{9,56,56,39,100,62,21,1},{9,73,73,35,100,62,18,6},{9,62,62,41,100,62,12,2}};
}
bool water_item_sprite_crop(SeaLateOperation op,unsigned v,SpriteCrop*out){const int k=slot(op,v);if(!out||k<0)return false;*out=crops[k];return true;}
WaterItemDrawStatus render_water_item_sprites(const SeaLateContext&c,const WaterItemDrawRuntime&in,const WaterItemSprites&r,const SpritePixelSurface&s){using S=WaterItemDrawStatus;
 if(!s.pixels||s.width<1||s.height<1||s.width>256||s.height>256||s.pitch<s.width||s.pitch>256)return S::invalid_input;
 const size_t used=(size_t)(s.height-1)*s.pitch+s.width;
 if(used>s.bytes||overlap(s.pixels,used,&c,sizeof c)||overlap(s.pixels,used,&in,sizeof in)||overlap(s.pixels,used,&r,sizeof r)||overlap(s.pixels,used,&s,sizeof s))return S::invalid_input;
 for(const auto&item:r.item)if(overlap(s.pixels,used,item.pixels.pixels,item.pixels.bytes))return S::invalid_input;
 if(!in.supplied||!in.coherent_source_cache_supplied)return S::missing_runtime;
 if(in.caller_x< -524287||in.caller_x>524287||in.caller_y< -524287||in.caller_y>524287||in.minimum_width<0||in.minimum_height<0||in.minimum_width>16384||in.minimum_height>16384||in.clip.left<0||in.clip.top<0||in.clip.right<in.clip.left||in.clip.bottom<in.clip.top||in.clip.right>s.width||in.clip.bottom>s.height)return S::invalid_input;
 SeaLatePlan p{};const auto status=inspect_sea_late(c,&p);if(status==SeaLateStatus::missing_context)return S::missing_runtime;if(status==SeaLateStatus::held_mode)return S::held_profile;if(status!=SeaLateStatus::inspected)return S::invalid_input;
 for(unsigned i=0;i<p.count;++i)if(p.events[i].operation==SeaLateOperation::crop_yield)return S::held_crop;
 if(c.source_zoom!=-5&&c.source_zoom!=5)return S::held_profile;
 for(unsigned i=0;i<p.count;++i){const int k=slot(p.events[i].operation,p.events[i].variant);if(k<0)return S::held_operation;const auto&item=r.item[k];if(!item.source_supplied)return S::missing_runtime;if(std::memcmp(&item.trim,&trims[k],sizeof(SpriteTrim)))return S::invalid_input;}
 const size_t span=(size_t)s.pitch*s.height;auto*mem=static_cast<unsigned char*>(std::malloc(span));if(!mem)return S::out_of_memory;std::memset(mem,0,span);std::memcpy(mem,s.pixels,used);auto stage=s;stage.pixels=mem;stage.bytes=span;
 SpriteDrawContext dc{c.source_zoom+16,16,in.minimum_width,in.minimum_height,true};S result=S::rendered;
 for(unsigned i=0;i<p.count;++i){const auto&item=r.item[slot(p.events[i].operation,p.events[i].variant)];const auto drawn=draw_sprite_ram(dc,item.trim,in.clip,in.caller_x,in.caller_y,item.pixels,stage);if(drawn!=SpriteDrawStatus::ready&&drawn!=SpriteDrawStatus::clipped&&drawn!=SpriteDrawStatus::minimum_size){result=S::failed_stage;break;}}
 if(result==S::rendered){std::memcpy(s.pixels,mem,used);}std::free(mem);return result;
}
}
