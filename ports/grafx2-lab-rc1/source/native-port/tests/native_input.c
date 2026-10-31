/* GPL-2.0-only: runs the native event loop with queued IDCMP messages. */
#include <assert.h>
#include <stdio.h>
#define _NO_INLINE
#define __USE_NEW_TIMEVAL__
#define USE_AMIGA_NATIVE
#define GLOBAL_VARIABLES
#include "global.h"
#include <intuition/intuition.h>
#include <devices/inputevent.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include "keycodes.h"
#include "amigakeys.h"
word Amiga_key_modifiers, Input_new_mouse_X, Input_new_mouse_Y;
byte Input_new_mouse_K,Button_inverter,Directional_emulated,Directional_click,Pan_shortcut_pressed;
int Mouse_moved;
static struct Window win;
static struct IntuiMessage queue[8];
static int count,read_index,replies,release_key,release_mod,delays;
struct Window *GFX2_AmigaNativeWindow(void) {return &win;}
struct Message *GetMsg(struct MsgPort *port) {return read_index<count?(void *)&queue[read_index++]:NULL;}
void ReplyMsg(struct Message *msg) {replies++;}
WORD MapRawKey(const struct InputEvent *ie,STRPTR buffer,WORD n,const struct KeyMap *km)
{if(ie->ie_Code==0x10){buffer[0]=(ie->ie_Qualifier&IEQUALIFIER_LSHIFT)?'Q':'q';return 1;}return 0;}
void Delay(LONG ticks) {delays++;}
void BeginRefresh(struct Window *w) {}
void EndRefresh(struct Window *w,LONG complete) {}
static void Color_cycling(void) {}
static void Flush_update(void) {}
static void Update_rect(short x,short y,unsigned short w,unsigned short h) {}
static int Move_cursor_with_constraints(int x,int y) {Mouse_X=x;Mouse_Y=y;Mouse_moved=1;return 1;}
static int Handle_mouse_btn_change(void) {Mouse_K=Input_new_mouse_K;return 1;}
static int Release_control(int k,int m) {release_key=k;release_mod=m;return 1;}
static int Handle_special_key_press(void) {return 0;}
static void Mouse_Emulation(void) {}
static void Compute_paintbrush_coordinates(void) {}
static void Display_cursor(void) {}
static int input(int sleep_time)
{
#include "amigainput.inc"
 return 0;
}
static void event(ULONG cls,UWORD code,UWORD qualifier,int x,int y)
{
 count=1;read_index=0;queue[0].Class=cls;queue[0].Code=code;queue[0].Qualifier=qualifier;queue[0].MouseX=x;queue[0].MouseY=y;
}
int main(void)
{
 Pixel_width=Pixel_height=1;
 event(IDCMP_MOUSEBUTTONS,IECODE_LBUTTON,0,0,0);assert(input(0)==1&&Mouse_K==1);
 event(IDCMP_MOUSEBUTTONS,IECODE_LBUTTON|IECODE_UP_PREFIX,0,0,0);assert(input(0)==1&&!Mouse_K);
 Button_inverter=1;event(IDCMP_MOUSEBUTTONS,IECODE_RBUTTON,0,0,0);input(0);assert(Mouse_K==1);Button_inverter=0;
 Pixel_width=Pixel_height=2;event(IDCMP_MOUSEMOVE,0,0,200,100);input(0);assert(Mouse_X==100&&Mouse_Y==50);
 event(IDCMP_RAWKEY,0x10,IEQUALIFIER_LSHIFT,0,0);input(0);assert(Key==(KEY_q|GFX2_MOD_SHIFT)&&Key_ANSI=='Q');
 event(IDCMP_RAWKEY,0x60|IECODE_UP_PREFIX,0,0,0);input(0);assert(release_key==KEY_LSHIFT&&release_mod==GFX2_MOD_SHIFT);
 event(IDCMP_RAWKEY,0x7a,0,0,0);input(0);assert(Key==KEY_MOUSEWHEELUP);
 event(IDCMP_RAWKEY,0x50,0,0,0);input(0);assert(Key==KEY_F1);
 event(IDCMP_INACTIVEWINDOW,0,0,0,0);input(0);assert(!Mouse_K&&!Amiga_key_modifiers&&!Button_inverter);
 event(IDCMP_CLOSEWINDOW,0,0,0,0);input(0);assert(Quit_is_required);Quit_is_required=0;
 count=read_index=0;input(20);assert(delays==1);assert(replies==10);
 puts("native input: buttons, scaled motion, text/modifiers, key release, wheel, focus and close passed");return 0;
}
