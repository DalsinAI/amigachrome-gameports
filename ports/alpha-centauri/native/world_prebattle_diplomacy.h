/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_WORLD_PREBATTLE_DIPLOMACY_H
#define AC_ALPHA_WORLD_PREBATTLE_DIPLOMACY_H
#include "original_world.h"
#include "prebattle_notification.h"
#include "prebattle_aggression.h"
namespace alpha {
struct WorldPrebattleNotificationRuntime {
 BattleEntryObservation observation;
 uint32_t current_faction,human_status,game_flags,popup_flag,turn,prior_warning;
};
struct WorldPrebattleAggressionRuntime {BattleEntryObservation observation;uint32_t current_faction;};
/* Independent read-only proposals from CURRENT actor ownership and faction
 * diplomacy records. Runtime observations must be supplied separately at the
 * appropriate original boundary. No saved current/human/flags/turn fallback,
 * event acknowledgment, original stage advancement or legal attack authority.
 * Notification success does NOT permit aggression execution: earlier events
 * and the original caller order remain the session controller's responsibility.
 */
PrebattleNotificationError inspect_world_prebattle_notification(const OriginalWorld&,size_t,size_t,
 const WorldPrebattleNotificationRuntime&,PrebattleNotificationProposal&);
AggressionError inspect_world_prebattle_aggression(const OriginalWorld&,size_t,size_t,
 const WorldPrebattleAggressionRuntime&,PrebattleAggressionProposal&);
}
#endif
