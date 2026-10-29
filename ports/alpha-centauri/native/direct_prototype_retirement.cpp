/* SPDX-License-Identifier: GPL-3.0-or-later. See direct_prototype_retirement.h. */
#include "direct_prototype_retirement.h"
#include "technology_effects_ranges.h"
#include <stdlib.h>
#include <utility>
namespace alpha {
void DirectRetirementProposal::clear(){free(vehicles_);vehicles_=nullptr;count_=0;conversions_=0;queues_.clear();state_={};history_={};replacement_=-1;ready_=false;}
void DirectRetirementProposal::swap(DirectRetirementProposal& p){using std::swap;swap(vehicles_,p.vehicles_);swap(count_,p.count_);swap(conversions_,p.conversions_);swap(state_,p.state_);swap(history_,p.history_);queues_.swap(p.queues_);swap(replacement_,p.replacement_);swap(ready_,p.ready_);}
static bool overlap(const OriginalWorld& w,const DirectRetirementProposal& out){using detail::effects_range_overlap;using detail::effects_array_overlap;
 return effects_range_overlap(&out,sizeof out,&w,sizeof w) || w.overlaps_byte_storage(&out,sizeof out) ||
 effects_array_overlap(&out,sizeof out,w.tile(0),w.game().map.info.tile_count,sizeof(OriginalMapTile)) ||
 effects_array_overlap(&out,sizeof out,w.vehicle(0),w.game().vehicle_count,sizeof(OriginalVehicle)) ||
 effects_array_overlap(&out,sizeof out,w.base(0),w.game().base_count,sizeof(OriginalBase)) ||
 effects_array_overlap(&out,sizeof out,w.prototype(0),512,sizeof(OriginalPrototype)) ||
 effects_array_overlap(&out,sizeof out,w.faction(0),8,sizeof(OriginalFactionState)) ||
 effects_array_overlap(&out,sizeof out,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
DirectRetirementError world_direct_prototype_retirement(const OriginalWorld& w,Variant variant,int32_t faction,int32_t id,DirectRetirementProposal& out){
 if(detail::effects_range_overlap(&out,sizeof out,&w,sizeof w) || !w.loaded() || faction<1 || faction>7 || id<64 || id>=512 || id/64!=faction ||
 (variant!=Variant::base && variant!=Variant::alien_crossfire) || (w.game().data_version<11)!=(variant==Variant::base) || w.game().base_count>512 || w.game().vehicle_count>2048 || overlap(w,out))return DirectRetirementError::bad_state;
 const auto* state=w.faction((size_t)faction);const auto* design=w.prototype((size_t)id);const auto table=w.game().bunglist;
 if(!state || !design || !table.data || table.size!=retirement_history_bytes || state->unk_103<0 || state->unk_103>63)return DirectRetirementError::bad_state;
 const uint8_t bit=(uint8_t)(1u<<(unsigned)faction),humans=w.game().preferences.FactionStatus[0];int32_t replacement=-1,score=-1;
 for(int32_t i=faction*64;i<(faction+1)*64;++i){const auto* p=w.prototype((size_t)i);if(!p)return DirectRetirementError::bad_state;
  if(i!=id && (p->unit_flags&1u) && p->group_id==design->group_id){const int32_t s=2-((p->obsolete_factions&bit)?1:0);if(s>score){score=s;replacement=i;}}
 }
 for(size_t i=w.game().vehicle_count;i>0;--i){const auto* v=w.vehicle(i-1);if(!v)return DirectRetirementError::bad_state;if(v->unit_id!=id)continue;
  if((humans&bit) || replacement<0 || v->faction_id!=faction)return DirectRetirementError::live_callbacks_required;
  if(v->visibility&humans){const auto width=w.game().map.info.width;const auto* tile=w.tile(((size_t)v->y*width+(size_t)v->x)/2);
   if(!tile)return DirectRetirementError::bad_state;
   const uint8_t owner=tile->val2&15u;if(!(tile->items&1u) || !owner || owner>=8)return DirectRetirementError::live_callbacks_required;
  }
 }
 DirectRetirementProposal next;next.count_=w.game().vehicle_count;next.state_=*state;next.replacement_=replacement;
 if(next.count_){next.vehicles_=(OriginalVehicle*)malloc(next.count_*sizeof(OriginalVehicle));if(!next.vehicles_)return DirectRetirementError::out_of_memory;for(size_t i=0;i<next.count_;++i)next.vehicles_[i]=*w.vehicle(i);}
 for(size_t i=next.count_;i>0;--i)if(next.vehicles_[i-1].unit_id==id){next.vehicles_[i-1].unit_id=(int16_t)replacement;
  next.state_.units_active[id]=(uint8_t)(next.state_.units_active[id]-1u);next.state_.units_active[replacement]=(uint8_t)(next.state_.units_active[replacement]+1u);++next.conversions_;}
 if(retirement_history_stage(table.data,table.size,faction,state->unk_103,*design,next.history_)!=LogicError::ok)return DirectRetirementError::bad_state;
 next.state_.unk_103=next.history_.next_slot;
 const auto e=retirement_queue_stage(variant,id,design->unit_flags,w.base(0),w.game().base_count,next.queues_);
 if(e!=RetirementQueueError::ok)return e==RetirementQueueError::out_of_memory?DirectRetirementError::out_of_memory:DirectRetirementError::bad_state;
 next.ready_=true;out.swap(next);return DirectRetirementError::ok;
}
}
