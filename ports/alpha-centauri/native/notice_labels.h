/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independently adapted from Thinker base.cpp, MIT THINKER-LICENSE.txt,
 * dabe62f2d1abba03de40f3dd3048494843f6a9af; original helpers verified privately.
 */
#ifndef ALPHA_NOTICE_LABELS_H
#define ALPHA_NOTICE_LABELS_H
#include "game_logic.h"
namespace alpha {
enum class FacilityNoticeIcon {human,alien};
struct NoticeLabelsInput {
 Variant variant;int32_t facility,queue_size,queue_count_slot;
 uint32_t governor_flags,warnings,faction_rule_flags;
 bool first_path,queue_admitted;
};
struct NoticeLabelsResult {
 char label[10];uint32_t warning_mask;int32_t queue_count_slot;
 FacilityNoticeIcon icon;bool queue_count_written;
};
/* Ordinary LOCAL production-notification arguments only, not popup execution.
 * IDs2..33 SMAC/2..41 SMAX, excluding special Subspace40; queue_size0..9.
 * PRODUCEX for first announcement, PRODUCE otherwise. Admitted queue adds Q
 * (before governor G precedence) and writes numeric slot1=queue_size+1.
 * Governing production requires BOTH0x80000000 and0x1. Regular governed/queued
 * warning mask1 only if warning0x2000 enabled; unmanaged regular and first use1.
 * SMAX faction flag0x80 selects alien facility icon; SMAC always human.
 * Existing numeric slot is explicit and preserved without an admitted queue.
 * Caller must obtain actual local route and current queue/governor state; this
 * query supplies no authority, labels for other production types, modal input,
 * acknowledgment, World/parse-global mutation, production completion or turn.
 * Label is terminated and zero padded. All errors preserve output.
 */
LogicError notice_labels(const NoticeLabelsInput&,NoticeLabelsResult&);
}
#endif
