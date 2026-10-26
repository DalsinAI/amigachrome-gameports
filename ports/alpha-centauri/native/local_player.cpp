/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "local_player.h"
#include <stdlib.h>
#include <string.h>
namespace alpha {
namespace {
constexpr size_t header_size=28;
const uint8_t magic[8]={'A','C','L','O','C','A','L','1'};
uint32_t get(const uint8_t* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
void put(uint8_t* p,uint32_t v){for(unsigned i=0;i<4;++i){p[i]=(uint8_t)v;v>>=8;}}
uint32_t crc(const uint8_t* p,size_t n){uint32_t c=~0u;while(n--){c^=*p++;for(unsigned i=0;i<8;++i)c=(c>>1)^((c&1u)?0xedb88320u:0);}return ~c;}
bool overlaps(const void* a,size_t n,const void* b,size_t m){
 if(!n || !m)return false;
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return n>UINTPTR_MAX-x || m>UINTPTR_MAX-y || (x<y+m && y<x+n);
}
bool aliases(const OriginalWorld& w,const void* p,size_t n){
 return overlaps(p,n,&w,sizeof(w)) || overlaps(p,n,w.original_bytes(),w.original_size()) ||
 overlaps(p,n,w.tile(0),w.game().map.info.tile_count*sizeof(OriginalMapTile)) ||
 overlaps(p,n,w.vehicle(0),w.game().vehicle_count*sizeof(OriginalVehicle)) ||
 overlaps(p,n,w.base(0),w.game().base_count*sizeof(OriginalBase)) ||
 overlaps(p,n,w.prototype(0),512*sizeof(OriginalPrototype)) ||
 overlaps(p,n,w.faction(0),8*sizeof(OriginalFactionState)) ||
 overlaps(p,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
}
LocalOrderError LocalPlayer::load(const uint8_t* bytes,size_t length,const uint8_t* rules,size_t rules_length,uint32_t seed){
 OriginalGameView view{};
 if(read_original_game(bytes,length,view)!=GameReadError::ok)return LocalOrderError::bad_world;
 auto e=session_.load_local_land_actions(bytes,length,rules,rules_length,Variant::base,false,view.player,seed);
 if(e==LocalOrderError::ok){pending_=false;dirty_=false;rules_crc_=crc(rules,rules_length);}return e;
}
LocalOrderError LocalPlayer::resume(const uint8_t* bytes,size_t length,const uint8_t* rules,size_t rules_length){
 if(!bytes || length<header_size || memcmp(bytes,magic,8))return LocalOrderError::bad_world;
 if(!rules || get(bytes+24)!=crc(rules,rules_length))return LocalOrderError::bad_rules;
 const size_t n=get(bytes+12);
 if(n!=length-header_size || get(bytes+16)!=crc(bytes+header_size,n))return LocalOrderError::bad_world;
 // CRC also covers the seed/length so a damaged header cannot change RNG state.
 if(get(bytes+8)!=crc(bytes+12,length-12))return LocalOrderError::bad_world;
 return load(bytes+header_size,n,rules,rules_length,get(bytes+20));
}
LocalOrderError LocalPlayer::step(size_t id,int32_t dx,int32_t dy,LocalStepReceipt& out){
 const auto* w=world();const auto* v=w?w->vehicle(id):nullptr;
 if(!v)return LocalOrderError::bad_state;
 const bool diagonal=(dx==1 || dx==-1)&&(dy==1 || dy==-1);
 const bool axial=(dx==0 && (dy==2 || dy==-2)) || (dy==0 && (dx==2 || dx==-2));
 if(!diagonal && !axial)return LocalOrderError::bad_state;
 int32_t x=v->x+dx,y=v->y+dy;const int32_t width=(int32_t)w->game().map.info.width;
 if(!w->game().map.info.flat){if(x<0)x+=width;else if(x>=width)x-=width;}
 LocalStepCommand command;auto e=session_.prepare_clear_land_step(id,x,y,command);
 if(e==LocalOrderError::ok)e=session_.commit_clear_land_step(command,out);
 if(e==LocalOrderError::ok && out.changed){dirty_=true;pending_=false;}return e;
}
LocalOrderError LocalPlayer::next_facility(int32_t base){
 if(!world())return LocalOrderError::bad_state;
 const int start=pending_ && plan_.preview().base==base?plan_.preview().facility:0;
 for(int offset=1;offset<72;++offset){const int id=(start+offset-1)%71+1;FacilityOrderPlan next;
  auto e=session_.prepare_facility(base,id,next);
  if(e==LocalOrderError::ok){plan_=next;pending_=true;return e;}
  if(e!=LocalOrderError::unavailable){pending_=false;return e;}
 }
 pending_=false;return LocalOrderError::unavailable;
}
LocalOrderError LocalPlayer::confirm_facility(FacilityOrderReceipt& out){
 if(!pending_)return LocalOrderError::bad_state;
 auto e=session_.commit(plan_,out);pending_=false;
 if(e==LocalOrderError::ok && out.changed)dirty_=true;
 return e;
}
size_t LocalPlayer::checkpoint_size()const{return world()?header_size+world()->original_size():0;}
LocalSnapshotError LocalPlayer::checkpoint(void* output,size_t capacity)const{
 const auto* w=world();if(!w)return LocalSnapshotError::bad_world;
 const size_t n=w->original_size();if(!output || n>UINT32_MAX || n>SIZE_MAX-header_size || capacity<header_size+n ||
 aliases(*w,output,header_size+n) || overlaps(output,header_size+n,this,sizeof(*this)))return LocalSnapshotError::bad_buffer;
 // Build separately: failed encoding leaves the caller's buffer untouched.
 auto* tmp=(uint8_t*)malloc(header_size+n);if(!tmp)return LocalSnapshotError::out_of_memory;
 LocalSnapshotReceipt r{};auto e=encode_local_actions_snapshot(*w,tmp+header_size,n,r);
 if(e==LocalSnapshotError::ok){
  memcpy(tmp,magic,8);put(tmp+12,(uint32_t)n);put(tmp+16,crc(tmp+header_size,n));put(tmp+20,r.runtime_seed);put(tmp+24,rules_crc_);
  put(tmp+8,crc(tmp+12,header_size+n-12));memcpy(output,tmp,header_size+n);
 }
 free(tmp);return e;
}
}
