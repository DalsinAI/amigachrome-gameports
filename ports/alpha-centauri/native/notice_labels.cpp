/* SPDX-License-Identifier: GPL-3.0-or-later. See notice_labels.h. */
#include "notice_labels.h"
namespace alpha {
LogicError notice_labels(const NoticeLabelsInput& in,NoticeLabelsResult& out){
 if((in.variant!=Variant::base && in.variant!=Variant::alien_crossfire) || in.facility<2 ||
 in.facility>(in.variant==Variant::base?33:41) || (in.variant==Variant::alien_crossfire && in.facility==40) ||
 in.queue_size<0 || in.queue_size>9)return LogicError::bad_state;
 NoticeLabelsResult next{};const char* prefix=in.first_path?"PRODUCEX":"PRODUCE";unsigned n=0;
 while(prefix[n]){next.label[n]=prefix[n];++n;}
 const bool governor=(in.governor_flags&0x80000001u)==0x80000001u;
 next.queue_count_slot=in.queue_count_slot;
 if(in.queue_admitted){next.label[n]='Q';next.queue_count_slot=in.queue_size+1;next.queue_count_written=true;}
 else if(governor)next.label[n]='G';
 next.warning_mask=in.first_path || (!in.queue_admitted && !governor) || (in.warnings&0x2000u)?1u:0u;
 next.icon=in.variant==Variant::alien_crossfire && (in.faction_rule_flags&0x80u)?FacilityNoticeIcon::alien:FacilityNoticeIcon::human;
 out=next;return LogicError::ok;
}
}
