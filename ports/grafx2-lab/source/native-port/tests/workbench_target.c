/* GPL-2.0-only. Real WBStartup launch protocol, including a selected project.
 * Run inside the isolated guest. Exit GrafX2 normally to complete the test. */
#define timerequest TimeRequest
#include <stdio.h>
#include <intuition/intuitionbase.h>
#include <proto/intuition.h>
#include <string.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <workbench/startup.h>
#include <proto/exec.h>
#include <proto/dos.h>
struct IntuitionBase *IntuitionBase;
int main(int argc, char **argv)
{
  FILE *report=fopen("SYS:workbench-result.log","w");
  struct MsgPort *reply = CreateMsgPort();
  struct WBStartup *startup = AllocVec(sizeof(*startup), MEMF_PUBLIC|MEMF_CLEAR);
  struct WBArg *args = AllocVec(2*sizeof(*args), MEMF_PUBLIC|MEMF_CLEAR);
  struct Process *child;
  struct Message *reply_message=NULL;
  int seen=0,tries;
  BPTR dir = Lock("SYS:GrafX2", ACCESS_READ);
  BPTR segment = LoadSeg("SYS:GrafX2/GrafX2");
  struct TagItem tags[] = {
    {NP_Seglist,segment},{NP_FreeSeglist,FALSE},{NP_StackSize,524288},
    {NP_Name,(ULONG)"GrafX2 Workbench test"},{NP_Cli,FALSE},
    {NP_HomeDir,0},{NP_CurrentDir,0},{TAG_DONE,0}};
  if (!report || !reply || !startup || !args || !dir || !segment) return 20;
  args[0].wa_Lock=dir; args[0].wa_Name=(STRPTR)"GrafX2";
  args[1].wa_Lock=dir; args[1].wa_Name=(STRPTR)"colour-fixture.png";
  startup->sm_Message.mn_ReplyPort=reply;
  startup->sm_Message.mn_Length=sizeof(*startup);
  startup->sm_Segment=segment; startup->sm_NumArgs=argc>1?2:1;
  startup->sm_ArgList=args;
  tags[5].ti_Data=DupLock(dir); tags[6].ti_Data=DupLock(dir);
  child=CreateNewProc(tags);
  if (!child) return 20;
  startup->sm_Process=&child->pr_MsgPort;
  PutMsg(&child->pr_MsgPort,(struct Message *)startup);
  fprintf(report,"WBStartup delivered; waiting for normal editor exit\n");fflush(report);
  IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
  for(tries=0;tries<600;tries++)
  {
    struct Screen *screen;
    ULONG key;
    if((reply_message=GetMsg(reply))!=NULL)break;
    key=LockIBase(0);
    for(screen=IntuitionBase->FirstScreen;screen;screen=screen->NextScreen)
      if(screen->DefaultTitle && !strcmp((const char *)screen->DefaultTitle,"GrafX2")){seen=1;break;}
    UnlockIBase(key);
    if(seen)break;
    Delay(5);
  }
  fprintf(report,"WORKBENCH EDITOR SCREEN: %s\n",seen?"PASS":"FAIL");fflush(report);
  if(!reply_message){WaitPort(reply);reply_message=GetMsg(reply);}
  if(reply_message!=(struct Message *)startup)return 20;
  CloseLibrary((struct Library *)IntuitionBase);

  fprintf(report,"WORKBENCH STARTUP REPLY: PASS\n");fclose(report);
  UnLoadSeg(segment); UnLock(dir);
  FreeVec(args); FreeVec(startup); DeleteMsgPort(reply);
  (void)argv;
  return seen?0:20;
}
