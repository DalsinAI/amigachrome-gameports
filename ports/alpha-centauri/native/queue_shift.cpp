/* SPDX-License-Identifier: GPL-3.0-or-later. See queue_shift.h. */
#include "queue_shift.h"
#include <string.h>
namespace alpha {
LogicError queue_shift(const QueueShiftInput& in,QueueShiftResult& out){
 if(in.queue_size<0 || in.queue_size>9 || (in.queue_size>0 && in.items[0]>=512))return LogicError::bad_state;
 QueueShiftResult next{};next.queue_size=in.queue_size;
 memcpy(next.items,in.items,sizeof next.items);memcpy(next.units_queue,in.units_queue,sizeof next.units_queue);
 if(in.queue_size>0){
  if(in.items[0]>=0)--next.units_queue[(unsigned)in.items[0]];
  for(int32_t i=0;i<in.queue_size;++i)next.items[i]=in.items[i+1];
  --next.queue_size;next.shifted=true;
 }
 out=next;return LogicError::ok;
}
}
