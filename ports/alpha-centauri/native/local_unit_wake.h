/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_LOCAL_UNIT_WAKE_H
#define ALPHA_LOCAL_UNIT_WAKE_H
#include "world_unit_stats.h"
namespace alpha {
class LocalProductionSession;
struct LocalWakePreview {size_t vehicle;uint32_t speed,contribution,state_before,state_after;uint8_t spent_before,spent_after,turns_before,turns_after,order_before;bool speed_evaluated;};
/* Checked original contribution and wake proposal. Original orders are signed
 * bytes; only4..23 without crawling compute speed and rewind terraform work.
 * Alert auto-type is original byte11 (both editions). Spent subtraction wraps
 * to a byte, including speed<road. Existing speed>255 remains unsupported.
 * Output is immutable proposal; no command delivery/turn authority is supplied.
 * Original source OpenSMACX pin2f7f4e24 contribution/veh_wake, GPL3+, Casey.
 */
LogicError world_terraform_contribution(const OriginalWorld&,const UnitDefinitions&,const Rules&,size_t vehicle,int32_t terraform,uint32_t& output);
LogicError world_unit_wake(const OriginalWorld&,const UnitDefinitions&,const Rules&,size_t vehicle,int32_t road,LocalWakePreview& output);
class LocalWakeCommand {
 public:const LocalWakePreview& preview()const{return preview_;}
 private:friend class LocalProductionSession;uint64_t owner_identity_=0,revision_=0;LocalWakePreview preview_{};
};
struct LocalWakeReceipt {bool changed,repaint_required,persistence_required;uint64_t revision;LocalWakePreview applied;};
}
#endif
