/* SPDX-License-Identifier: GPL-3.0-or-later
 * Separate game CRT rand recurrence verified from the original demo's actual
 * x86 arithmetic routine. See COMBAT-CRT.md for binary pin and scoped proof.
 */
#ifndef ALPHA_COMBAT_CRT_H
#define ALPHA_COMBAT_CRT_H
#include "combat_rounds.h"
namespace alpha {
struct CombatRandomState {uint32_t state;};
/* Caller-owned separate CRT state, never inferred from map/tick/movement RNG.
 * Unsigned wrap is defined, and returned range is exactly0..32767. */
uint16_t combat_crt_draw(CombatRandomState&);
/* Same qualified conventional round policy as resolve_combat_rounds. Only the
 * draws actually consumed by a successful resolution advance the public state.
 * Invalid input/overlap leaves output and seed unchanged. No global CRT/thread
 * state, world mutation or attack/turn authority is installed by this helper.
 */
CombatError resolve_seeded_combat_rounds(const CombatDuel&,CombatRandomState&,CombatResult&);
}
#endif
