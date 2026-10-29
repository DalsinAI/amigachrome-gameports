/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independent owned implementation of privately verified original popb
 * post-modal state effects. No original bytes or decompiled text included.
 */
#ifndef ALPHA_POPUP_RESPONSE_H
#define ALPHA_POPUP_RESPONSE_H
#include "game_logic.h"
namespace alpha {
enum class PopupResponseRuntime : uint32_t {owned_offline=1};
struct PopupResponseInput {
 Variant variant;PopupResponseRuntime runtime;
 int32_t confirmed_result,base_upkeep_flag;
 uint32_t warnings,warnings_mirror,warning_mask,governor_flags;
 bool disable_warning_checked;
};
struct PopupResponseResult {
 int32_t base_upkeep_flag;uint32_t warnings,warnings_mirror,governor_flags;
 bool staple_required,preferences_save_required;
};
/* Pure original POST-MODAL fragment only, under an explicit owned-offline
 * policy. This query must never fabricate a modal result, constructor return,
 * callback, network/PBEM state or acknowledgment. No original GUI ran here.
 * Checkbox precedence: clear warning_mask, mirror updated warnings, require
 * real preferences persistence; ignore the result's other effects.
 * Otherwise result0 sets BaseUpkeepFlag1 (deferred later base-window request,
 * NOT immediate production selection), result2 enables governor0x80000040,
 * result3 REQUIRES actual action_staple, other raw int32 results preserve state.
 * Nerve stapling/preferences persistence are only obligations, never fake calls.
 * No popup cleanup, actual base window, queue change, World/session authority,
 * filesystem write, accounting/upkeep execution, turn or player input is added.
 * Runtime choice is not inferred from a save. Errors preserve output.
 */
LogicError popup_response(const PopupResponseInput&,PopupResponseResult&);
}
#endif
