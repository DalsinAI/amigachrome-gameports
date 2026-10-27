// SPDX-License-Identifier: GPL-3.0-or-later
#include "terrain_sea_late.h"
#include <cstddef>
namespace alpha {
SeaLateStatus inspect_sea_late(const SeaLateContext&c,SeaLatePlan*out){using S=SeaLateStatus;using O=SeaLateOperation;
 if(!out)return S::invalid_input;
 const auto x=reinterpret_cast<std::uintptr_t>(&c),y=reinterpret_cast<std::uintptr_t>(out);if(x<=y?y-x<sizeof(c):x-y<sizeof(*out))return S::invalid_input;
 if(!c.source_context_supplied)return S::missing_context;
 if(c.viewer< -1||c.viewer>7||c.current_faction<0||c.current_faction>7||c.raw_using_faction<0||c.raw_using_faction>7||c.map_x<0||c.map_y<0||c.map_x>32767||c.map_y>32767||((c.map_x+c.map_y)&1)||c.raw_altitude>7||(!c.late_visible&&c.viewer<0))return S::invalid_input;
 if(!c.normal_textured_path||c.raw_altitude>=3||c.source_zoom< -8||c.source_zoom>5||(c.draw_flags&~0x04001421u)||(c.raw_landmarks&0xe000u))return S::held_mode;
 SeaLatePlan p{};
 if(c.late_visible){auto add=[&](O op,unsigned v=0){p.events[p.count++]={op,v};};const bool farm=c.displayed_items&0x8000u,near=c.source_zoom> -8,later=c.draw_flags&0x20u;
  if((c.draw_flags&1u)&&farm)add(O::sea_platform);
  if((later&&farm&&near)||(c.draw_flags&0x1000u)){add(O::crop_yield);p.crop_arguments[0]=c.viewer<0?(c.raw_using_faction?c.raw_using_faction:c.current_faction):c.viewer;p.crop_arguments[1]=-1;p.crop_arguments[2]=c.map_x;p.crop_arguments[3]=c.map_y;p.crop_arguments[4]=0;}
  if(later&&near){if(c.displayed_items&0x80000000u)add(O::item_highbit);if(c.expansion_enabled||c.raw_altitude==2){if(c.displayed_items&0x10u)add(O::item_10);if(c.displayed_items&0x40u)add(O::item_40);}if(c.displayed_items&0x400000u)add(O::item_400000);if(c.displayed_items&0x800000u)add(O::item_800000);if(c.displayed_items&0x1000000u)add(O::item_1000000,(c.raw_landmarks&0x80001000u)==0x1000u);}
 }
 *out=p;return S::inspected;
}
}
