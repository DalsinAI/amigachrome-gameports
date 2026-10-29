/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_UI_PANEL_H
#define ALPHA_UI_PANEL_H
#include <stddef.h>
#include <stdint.h>
namespace alpha {
/* Platform-free panel output: pixels go into an RGB24 frame, text is returned
 * as items the window draws with its own fonts (original_ui_text on Amiga).
 * Ink numbers follow alpha::UiInk: 0 body,1 label,2 value,3 title,4 map,5 warning. */
struct UiTextItem { int x,baseline,width;uint8_t ink;bool heading;char text[96]; };
struct UiFrame { uint8_t* rgb;size_t capacity;unsigned width,height; };
struct UiRect { int x,y,w,h; };
bool ui_frame_valid(const UiFrame&);
void ui_fill(const UiFrame&,const UiRect&,const uint8_t rgb[3]);
/* Background plus a one-pixel frame. */
void ui_panel(const UiFrame&,const UiRect&,const uint8_t fill[3],const uint8_t edge[3]);
/* Horizontal bar of up to three adjacent segments, proportional to values over
 * scale (clamped; non-positive values skipped). Returns the painted width. */
int ui_bar(const UiFrame&,const UiRect&,const int32_t values[3],const uint8_t colours[3][3],int32_t scale);
/* Append a bounded text item; returns false when items are full. */
bool ui_text_item(UiTextItem* items,size_t capacity,size_t& count,int x,int baseline,int width,uint8_t ink,bool heading,const char* text);
/* Panel keys. The window maps Amiga keys (keypad 8/2/4/6, cursor keys,
 * RETURN, ESC) to these before passing them to a panel. */
enum UiKey { UIK_NONE=0,UIK_UP,UIK_DOWN,UIK_LEFT,UIK_RIGHT,UIK_ENTER,UIK_BACK };
/* Keypad digits, RETURN/ENTER and ESC as UiKey; anything else UIK_NONE. */
int ui_key_from_char(int c);
/* Panel results. */
enum UiPanelAction { UIA_NONE=0,UIA_MOVED,UIA_APPLY,UIA_CLOSE,UIA_EXTRA1,UIA_EXTRA2,UIA_EXTRA3 };
/* Move a list cursor by one with wrap-around; count 0 leaves it at 0. */
void ui_list_step(int& cursor,int count,int delta);
}
#endif
