/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_PREBATTLE_NOTIFICATION_H
#define AC_ALPHA_PREBATTLE_NOTIFICATION_H
#include "battle_entry.h"
namespace alpha {
enum class PrebattleNotificationError {ok,bad_state,missing_runtime,unsupported};
enum class PrebattleNotificationRoute {observer,current_defender,remote_popup,silent_contact};
struct PrebattleNotificationFacts {
 BattleEntryObservation observation;Variant edition;
 uint32_t attacker,defender,current_faction,human_status,game_flags,popup_flag;
 uint32_t turn,prior_warning,prior_spoke,status[64];
};
struct PrebattleNotificationProposal {
 PrebattleNotificationRoute route;
 uint32_t warning,spoke;
};
/* Bounded ordinary-plan surprise/observer selector AFTER outer plan tests.
 * Explicit current facts, including full directional status matrix. No caller
 * stage, notification completion or runtime authority inferred. Event routes
 * preserve prior spoke: actual notification must finish before later stamping.
 * silent_contact proposes the original immediate timestamp; observer proposes
 * only its local warning. Does not execute aggression, callbacks or World writes.
 */
PrebattleNotificationError inspect_prebattle_notification(const PrebattleNotificationFacts&,PrebattleNotificationProposal&);
}
#endif
