// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ALPHA_TERRAIN_SEA_LATE_H
#define ALPHA_TERRAIN_SEA_LATE_H
#include <cstdint>
namespace alpha {
enum class SeaLateOperation:std::uint32_t {none=0,sea_platform=1,crop_yield=2,item_highbit=3,item_10=4,item_40=5,item_400000=6,item_800000=7,item_1000000=8};
struct SeaLateContext {
 std::int32_t viewer,current_faction,raw_using_faction,map_x,map_y,source_zoom;
 std::uint32_t raw_altitude,draw_flags,displayed_items,raw_landmarks;
 bool late_visible,expansion_enabled,normal_textured_path,source_context_supplied;
};
struct SeaLateEvent {SeaLateOperation operation;std::uint32_t variant;};
struct SeaLatePlan {std::uint32_t count;SeaLateEvent events[8];std::int32_t crop_arguments[5];};
enum class SeaLateStatus:std::uint32_t {inspected=0,missing_context=1,held_mode=2,invalid_input=3};
// Inspection ONLY. Never evaluates a yield, draws a sprite, satisfies a callback
// or removes the scene's bit20 hold. Explicit real source observation/late gate,
// CurrentFaction, using-faction and runtime ExpansionEnabled are required.
// Normal sea path, source zoom[-8,5], supported flags04001421 only. Landmark
// overlays2000/4000/8000 and bonus/goody flags remain held, not silently omitted.
// The sea sprite precedes the road boundary; crop is after roads, before later
// item sprites. Even at sea the original crop call MUST occur, though its return
// is used only by land crop icons. Missing crop runtime authority cannot be
// replaced by zero or an ignored return. Source arguments are faction,-1,x,y,0.
// Water platform bit1 is independent of crop bit20 and has no near-zoom guard;
// forced crop flag1000 ignores the near-zoom test but still follows the late gate.
// This is ordered caller intent, NOT whole original Sprite/USER32 qualification.
SeaLateStatus inspect_sea_late(const SeaLateContext&,SeaLatePlan*);
}
#endif
