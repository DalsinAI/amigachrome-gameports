/* GPL-2.0-only: behavioral checks of the actual native backend with OS call mocks.
 * This does not qualify Intuition, RTG, or AmigaOS runtime behavior. */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _NO_INLINE
#define __USE_NEW_TIMEVAL__
#define USE_AMIGA_NATIVE
#include "amigascreen.c"
int Pixel_width=1, Pixel_height=1;
short Screen_width=640, Screen_height=480;
byte Menu_factor_X=1, Menu_factor_Y=1;
word Menu_status_Y;
word Mouse_X, Mouse_Y;
void GFX2_AmigaDataTypesClose(void) {}
static int timer_ok, ports, requests, timers, delays, sends, aborts, io_done;
static ULONG waited_mask, sent_secs, sent_micro;
struct MsgPort *CreateMsgPort(void) {struct MsgPort *p;if(!timer_ok)return NULL;p=calloc(1,sizeof(*p));p->mp_SigBit=20;ports++;return p;}
APTR CreateIORequest(const struct MsgPort *port,ULONG size) {if(!port)return NULL;requests++;return calloc(1,size);}
BYTE OpenDevice(CONST_STRPTR name,ULONG unit,struct IORequest *io,ULONG flags) {if(!timer_ok||strcmp((const char *)name,"timer.device")||unit!=UNIT_MICROHZ)return -1;timers++;return 0;}
void CloseDevice(struct IORequest *io) {if(timer_ok)timers--;}
void DeleteIORequest(APTR io) {if(io){requests--;free(io);}}
void DeleteMsgPort(struct MsgPort *port) {if(port){ports--;free(port);}}
void Delay(LONG ticks) {delays+=ticks;}
ULONG SetSignal(ULONG value,ULONG mask) {return 0;}
void SendIO(struct IORequest *io) {struct TimeRequest *t=(void *)io;assert(io->io_Command==TR_ADDREQUEST);sent_secs=t->tr_time.tv_secs;sent_micro=t->tr_time.tv_micro;sends++;}
ULONG Wait(ULONG mask) {waited_mask=mask;return mask;}
struct IORequest *CheckIO(struct IORequest *io) {return io_done?io:NULL;}
void AbortIO(struct IORequest *io) {aborts++;}
BYTE WaitIO(struct IORequest *io) {return 0;}
BYTE DoIO(struct IORequest *io) {return 0;}
static int allocations, windows, screens, writes, fail_screen, fail_window, fail_alloc;
static UBYTE captured[640*480];
static LONG last_x,last_y,last_right,last_bottom;
struct Library *OpenLibrary(CONST_STRPTR name, ULONG version) { static struct Library lib; return &lib; }
void CloseLibrary(struct Library *lib) {}
APTR AllocVec(ULONG bytes, ULONG flags) { if(fail_alloc)return NULL;void *p=calloc(1,bytes); if(p)allocations++;return p; }
void FreeVec(APTR p) { if(p){allocations--;free(p);} }
struct Message *GetMsg(struct MsgPort *port) {return NULL;}
void ReplyMsg(struct Message *msg) {}
BOOL ModifyIDCMP(struct Window *w, ULONG flags) {return 1;}
void ClearPointer(struct Window *w) {}
void CloseWindow(struct Window *w) {windows--;free(w->UserPort);free(w->RPort);free(w);}
BOOL CloseScreen(struct Screen *s) {screens--;free(s);return 1;}
ULONG BestModeIDA(const struct TagItem *tags) {return 1;}
ULONG BestModeID(ULONG tag, ...) {return 1;}
LONG ModeNotAvailable(ULONG id) {return 0;}
ULONG GetDisplayInfoData(DisplayInfoHandle h, APTR buf, ULONG size, ULONG tag, ULONG id)
{struct DimensionInfo *d=(void *)buf;memset(d,0,sizeof(*d));d->MaxDepth=8;d->Nominal.MaxX=639;d->Nominal.MaxY=479;return sizeof(*d);}
struct Screen *OpenScreenTagList(const struct NewScreen *ns, const struct TagItem *tags)
{
 if(fail_screen)return NULL;
 struct Screen *s=calloc(1,sizeof(*s));s->Width=640;s->Height=480;screens++;return s;
}
struct Window *OpenWindowTagList(const struct NewWindow *nw, const struct TagItem *tags)
{
 if(fail_window)return NULL;
 struct Window *w=calloc(1,sizeof(*w));w->RPort=calloc(1,sizeof(struct RastPort));w->UserPort=calloc(1,sizeof(struct MsgPort));windows++;return w;
}
void SetPointer(struct Window *w, UWORD *ptr, WORD h,WORD width,WORD x,WORD y) {}
void ScreenToFront(struct Screen *s) {}
void WriteChunkyPixels(struct RastPort *rp, UWORD x,UWORD y,UWORD right,UWORD bottom,const UBYTE *p,LONG pitch)
{
 writes++;last_x=x;last_y=y;last_right=right;last_bottom=bottom;
 for(ULONG row=0;row<=bottom-y;row++)memcpy(captured+(y+row)*640+x,p+row*pitch,right-x+1);
}
void SetRGB32(struct ViewPort *v,ULONG index,ULONG r,ULONG g,ULONG b) {}
LONG GetScreenData(APTR buf,UWORD size,UWORD type,const struct Screen *s) {return 0;}
LONG EasyRequestArgs(struct Window *w,const struct EasyStruct *es,ULONG *flags,CONST_APTR args) {assert(!strcmp(es->es_TextFormat,"%s"));return 1;}
void GFX2_Log(GFX2_Log_priority_T p,const char *fmt,...) {}
int main(void)
{
 int w=640,h=480;
 assert(GFX2_AmigaInit());
 assert(GFX2_AmigaModeOK(640,480));assert(!GFX2_AmigaModeOK(320,200));
 GFX2_Set_mode(&w,&h,1);assert(windows==1&&screens==1&&writes==1);
 Set_Screen_pixel(639,479,97);assert(Get_Screen_pixel(639,479)==97);
 Set_Screen_pixel(-1,0,12);assert(Get_Screen_pixel(-1,0)==0);assert(!Get_Screen_pixel_ptr(640,0));
 Screen_FillRect(-2,-2,4,4,5);assert(Get_Screen_pixel(0,0)==5&&Get_Screen_pixel(1,1)==5&&Get_Screen_pixel(2,2)==0);
 Update_rect(0,0,2,2);Flush_update();assert(last_x==0&&last_y==0&&last_right==1&&last_bottom==1);assert(captured[641]==5);
 {int n=writes;Flush_update();assert(writes==n);}
 Pixel_width=2;Pixel_height=2;Update_rect(3,4,2,3);Flush_update();assert(last_x==6&&last_y==8&&last_right==9&&last_bottom==13);Pixel_width=Pixel_height=1;
 T_Components c={12,34,56};assert(GFX2_SetPalette(&c,4,1)==0);assert(Native_palette[4].G==34);assert(GFX2_SetPalette(&c,256,1)==-1);
 {int n=writes;Flush_update();assert(writes==n+1);assert(last_x==0&&last_y==0&&last_right==639&&last_bottom==479);Flush_update();assert(writes==n+1);}
 struct Screen *old=Native_screen;UBYTE *pixels=Native_pixels;
 for(int kind=0;kind<3;kind++)
 {
  fail_screen=kind==0;fail_window=kind==1;fail_alloc=kind==2;w=800;h=600;
  GFX2_Set_mode(&w,&h,1);assert(Native_screen==old&&Native_pixels==pixels&&w==640&&h==480);assert(windows==1&&screens==1);
 }
 fail_screen=fail_window=fail_alloc=0;
 GFX2_Set_mode(&w,&h,1);assert(windows==1&&screens==1&&Native_palette[4].G==34);
 GFX2_AmigaIdleWait(20);assert(delays==1&&!sends);
 GFX2_AmigaClose();assert(!allocations&&!windows&&!screens);GFX2_AmigaClose();
 timer_ok=1;assert(GFX2_AmigaInit());w=640;h=480;GFX2_Set_mode(&w,&h,1);Native_window->UserPort->mp_SigBit=9;
 GFX2_AmigaIdleWait(20);assert(sends==1&&sent_secs==0&&sent_micro==20000&&aborts==1&&delays==1);
 assert(waited_mask==((1UL<<9)|(1UL<<20)));
 io_done=1;GFX2_AmigaIdleWait(1500);assert(sends==2&&sent_secs==1&&sent_micro==500000&&aborts==1);
 GFX2_AmigaClose();assert(!allocations&&!windows&&!screens&&!timers&&!requests&&!ports);
 puts("native screen: clipping, dirty/scaled refresh, palette, rollback, idle wait and teardown passed");return 0;
}
