/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original saved faction layouts adapted from Thinker, MIT.
 * See THINKER-LICENSE.txt; pin dabe62f2d1abba03de40f3dd3048494843f6a9af.
 */
#ifndef AC_ALPHA_ORIGINAL_FACTION_H
#define AC_ALPHA_ORIGINAL_FACTION_H
#include "original_game.h"
namespace alpha {
struct OriginalFactionGoal { int16_t type,priority;int32_t x,y,base_id; };
struct OriginalFactionDefinition {
#define WIRE_SCALAR(type,name,offset) type name;
#define WIRE_ARRAY(type,name,count,offset) type name[count];
#define WIRE_STRING(name,count,offset) char name[count+1];
#define WIRE_MATRIX(type,name,rows,cols,offset) type name[rows][cols];
#define WIRE_TEXT_MATRIX(name,rows,cols,offset) char name[rows][cols+1];
#define WIRE_GOALS(name,count,offset) OriginalFactionGoal name[count];
#include "original_faction_definition_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
#undef WIRE_MATRIX
#undef WIRE_TEXT_MATRIX
#undef WIRE_GOALS
};
struct OriginalFactionState {
#define WIRE_SCALAR(type,name,offset) type name;
#define WIRE_ARRAY(type,name,count,offset) type name[count];
#define WIRE_STRING(name,count,offset) char name[count+1];
#define WIRE_MATRIX(type,name,rows,cols,offset) type name[rows][cols];
#define WIRE_TEXT_MATRIX(name,rows,cols,offset) char name[rows][cols+1];
#define WIRE_GOALS(name,count,offset) OriginalFactionGoal name[count];
#include "original_faction_state_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
#undef WIRE_MATRIX
#undef WIRE_TEXT_MATRIX
#undef WIRE_GOALS
};
/* Decodes all original faction-state/definition fields, including AI goals,
 * diplomacy, research, social settings and accounting, without executing them.
 * A validated immutable OriginalGameView is required; index0..7 includes Planet.
 * Legacy facility-announcement8 ->12 and project-intel5 ->8 zero extensions
 * follow the recovered source conversion. Bytes remain original version IDs.
 * All strings retain wire bytes plus a terminator; matrices preserve row shape.
 * Padding and Thinker-labelled reserved original bytes remain inert data.
 * Engine reference/semantic validation is a separate world-initialization step.
 * Outputs remain unchanged on failure. No callbacks or serialized pointers.
 */
GameReadError original_faction_definition(const OriginalGameView& game,size_t index,OriginalFactionDefinition& output);
GameReadError original_faction_state(const OriginalGameView& game,size_t index,OriginalFactionState& output);
}
#endif
