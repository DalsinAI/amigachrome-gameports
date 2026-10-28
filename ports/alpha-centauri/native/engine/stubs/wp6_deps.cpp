/* SPDX-License-Identifier: GPL-3.0-or-later
 * Placeholders for the WP2 and WP3 entry points that engine_wp.h declares for
 * WP6 (computer players): action_staple (WP2) and action, action_build,
 * action_gate, study_artifact, Path_find, Path_move (WP3). Each raises
 * EV_NOT_IMPLEMENTED and returns a neutral value, like engine/stubs/wpN.cpp.
 *
 * WP2 and WP3 already define these functions on their branches (with the same
 * signatures) but outside engine_wp.h. When a package lands, delete its
 * functions from this file in the same merge; when both have landed, delete
 * the file. A leftover placeholder shows up as a duplicate symbol at link time.
 */
#include "../engine.h"

namespace alpha {
namespace engine {

/* ---- WP2 ---- */
void __cdecl action_staple(int UNUSED(base_id)) {
    not_implemented("action_staple");
}

/* ---- WP3 ---- */
int __cdecl action(int UNUSED(veh_id)) {
    not_implemented("action");
    return 0;
}

int __cdecl action_build(int UNUSED(veh_id), const char* UNUSED(name)) {
    not_implemented("action_build");
    return 0;
}

void __cdecl action_gate(int UNUSED(veh_id), int UNUSED(base_id)) {
    not_implemented("action_gate");
}

int __cdecl study_artifact(int UNUSED(veh_id)) {
    not_implemented("study_artifact");
    return 0;
}

int __cdecl Path_find(int UNUSED(x1), int UNUSED(y1), int UNUSED(x2), int UNUSED(y2),
int UNUSED(unit_id), int UNUSED(faction_id), int UNUSED(flags), int UNUSED(unk)) {
    not_implemented("Path_find");
    return -1;
}

int __cdecl Path_move(int UNUSED(veh_id), int UNUSED(faction_id)) {
    not_implemented("Path_move");
    return -1;
}

} // namespace engine
} // namespace alpha
