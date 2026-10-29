/* SPDX-License-Identifier: GPL-3.0-or-later. See prototype_retirement_queue.h. */
#include "prototype_retirement_queue.h"
#include <stdlib.h>
namespace alpha {
static bool bad_range(const void* p,size_t n){return n && (!p || n>UINTPTR_MAX-(uintptr_t)p);}
static bool overlaps(const void* a,size_t an,const void* b,size_t bn){if(!an || !bn)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<y+bn && y<x+an;}
void RetirementQueueProposal::clear(){free(bases_);bases_=nullptr;count_=0;unit_flags_=0;changed_=false;}
void RetirementQueueProposal::swap(RetirementQueueProposal& p){OriginalBase* b=bases_;bases_=p.bases_;p.bases_=b;size_t c=count_;count_=p.count_;p.count_=c;uint16_t f=unit_flags_;unit_flags_=p.unit_flags_;p.unit_flags_=f;bool d=changed_;changed_=p.changed_;p.changed_=d;}
RetirementQueueError retirement_queue_stage(Variant variant,int32_t id,uint16_t flags,const OriginalBase* bases,size_t count,RetirementQueueProposal& out){
 if((variant!=Variant::base && variant!=Variant::alien_crossfire) || id<0 || id>=512 || count>512)return RetirementQueueError::bad_state;
 const size_t bytes=count*sizeof(OriginalBase);
 if(bad_range(bases,bytes) || bad_range(&out,sizeof out) || overlaps(bases,bytes,&out,sizeof out))return RetirementQueueError::bad_state;
 for(size_t i=0;i<count;++i)if(bases[i].queue_size < -1 || bases[i].queue_size>9)return RetirementQueueError::bad_state;
 RetirementQueueProposal next;next.unit_flags_=flags;next.count_=count;
 if(count){next.bases_=(OriginalBase*)malloc(bytes);if(!next.bases_)return RetirementQueueError::out_of_memory;for(size_t i=0;i<count;++i)next.bases_[i]=bases[i];}
 if(id>=64){
  for(size_t i=0;i<count;++i){OriginalBase& b=next.bases_[i];
   for(int32_t j=0;j<=b.queue_size;++j)if(b.queue_items[j]==id){
    if(j==0)b.queue_items[0]=variant==Variant::base?-38:-69;
    else{for(int32_t k=j;k<b.queue_size;++k)b.queue_items[k]=b.queue_items[k+1];--b.queue_size;}
    next.changed_=true;
   }
  }
  next.unit_flags_=(uint16_t)(flags&~1u);if(next.unit_flags_!=flags)next.changed_=true;
 }
 out.swap(next);return RetirementQueueError::ok;
}
}
