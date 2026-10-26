/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original fragment adaptation from Thinker base.cpp, MIT THINKER-LICENSE.txt,
 * dabe62f2d1abba03de40f3dd3048494843f6a9af, retaining edition differences.
 */
#ifndef ALPHA_FACILITY_NOTICE_H
#define ALPHA_FACILITY_NOTICE_H
#include "game_logic.h"
namespace alpha {
enum class FacilityNoticeRoute {none,foreign,local_regular,local_first,subspace_special};
struct FacilityNoticeInput {
 Variant variant;int32_t facility,faction,local_faction;
 uint32_t game_state,warnings_entry,warnings_after_construction;
 uint32_t preferences_before_voice,preferences_after_popup,human_mask;
 uint8_t announced[12];
};
struct FacilityNoticeResult {
 FacilityNoticeRoute route;uint8_t announced[12];
 bool blurb_requested,voice_load_requested,voice_fade_requested;
};
/* PURE separately staged original notification fragments. Not a GUI callback
 * simulator or an acknowledgment: later-stage snapshots must come from real
 * runtime when a caller eventually owns those stages. All proposed bytes stay
 * caller-owned. Local ordinary IDs2..33 SMAC/2..41 SMAX only; SMAX40 reports a
 * separate unsupported Subspace-generator obligation, not ordinary completion.
 * Route foreign is an original outer branch, not proof of visible notification.
 * First path requires unannounced bit, game-done clear and warning1 on entry.
 * After actual construction, live warning1 decides blurb/announced-bit write.
 * Voice load is a CALL obligation only for local human+preference0x4000;
 * after-popup fade0x4000+0x20000 does NOT repeat the human guard. Snapshots may
 * differ; no stable callback flags are invented. Requests do not prove media
 * present, playback, a window constructed/closed or a user choice received.
 * Announced input is a caller-owned 12-byte vector; legacy wire stores only8
 * bytes at0x458, modern12 at0x458. No wire/persistence writes are supplied.
 * No labels/queue/governor/full popup logic, monuments/interludes, World writes,
 * completion transaction, phase/turn or frontend controls. Errors preserve out.
 */
LogicError facility_notice(const FacilityNoticeInput&,FacilityNoticeResult&);
}
#endif
