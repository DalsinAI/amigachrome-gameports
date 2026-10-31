/* GPL-2.0-only: original classic tool icon, written by target icon.library. */
#define timerequest TimeRequest
#include <workbench/workbench.h>
#include <proto/exec.h>
#include <proto/icon.h>
struct Library *IconBase;
static UWORD normal_data[128], selected_data[128];
static struct Image normal_image={0,0,32,32,2,normal_data,3,0,0};
static struct Image selected_image={0,0,32,32,2,selected_data,3,0,0};
static void pixel(UWORD *data,int x,int y,int colour)
{
  int plane;
  for(plane=0;plane<2;plane++)
    if(colour&(1<<plane))data[plane*64+y*2+x/16]|=(UWORD)(0x8000>>(x%16));
}
int main(void)
{
  struct DiskObject icon={0};
  int x,y,ok;
  IconBase=OpenLibrary("icon.library",39);
  if(!IconBase)return 20;
  for(y=2;y<30;y++)for(x=2;x<30;x++)
  {
    int c=(x==2||x==29||y==2||y==29)?1:2;
    /* An original diagonal brush on a canvas, with a selected border. */
    if(x+y>=28&&x+y<=33&&x>=7&&x<=24)c=(x<11)?1:3;
    pixel(normal_data,x,y,c);
    pixel(selected_data,x,y,(x==2||x==29||y==2||y==29)?3:c);
  }
  icon.do_Magic=WB_DISKMAGIC;icon.do_Version=WB_DISKVERSION;
  icon.do_Gadget.Width=32;icon.do_Gadget.Height=32;
  icon.do_Gadget.Flags=GFLG_GADGIMAGE|GFLG_GADGHIMAGE;
  icon.do_Gadget.Activation=GACT_RELVERIFY;
  icon.do_Gadget.GadgetType=GTYP_BOOLGADGET;
  icon.do_Gadget.GadgetRender=&normal_image;
  icon.do_Gadget.SelectRender=&selected_image;
  icon.do_Type=WBTOOL;icon.do_CurrentX=NO_ICON_POSITION;
  icon.do_CurrentY=NO_ICON_POSITION;icon.do_StackSize=524288;
  ok=PutDiskObject((CONST_STRPTR)"SYS:GrafX2/GrafX2",&icon);
  CloseLibrary(IconBase);
  return ok?0:20;
}
