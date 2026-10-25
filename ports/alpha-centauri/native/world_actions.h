/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_WORLD_ACTIONS_H
#define AC_ALPHA_WORLD_ACTIONS_H
#include "world_movement_price.h"
#include "vehicle_stack.h"
namespace alpha {
enum class StepStatus { moved,exhausted,not_owner,unsupported,bad_state,out_of_memory };
struct StepResult {StepStatus status;uint32_t price;};
/* First bounded action path, NOT a replacement for order_veh. Only ordinary
 * human land units with no special abilities, standing orders or transport,
 * on already-visible cleared land and away from foreign actors/bases. Refuses
 * unimplemented encounters, fog, pods, monoliths, fungus and fractional moves.
 * No fake next turn, research/economy or persistence. Imported wire bytes remain
 * the baseline snapshot; only owned runtime records change on success.
 * UNQUALIFIED integration: do not expose to user input until controls pass.
 */
class WorldActions {
 public:
 static StepResult clear_land_step(OriginalWorld& world,const UnitDefinitions& definitions,
   const Rules& technologies,size_t vehicle,int32_t player,int32_t road_rate,int32_t x,int32_t y);
};
const char* step_status_name(StepStatus status);
}
#endif
