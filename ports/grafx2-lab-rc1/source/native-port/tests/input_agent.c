#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <exec/execbase.h>
#include <intuition/intuitionbase.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dostags.h>
#include <proto/intuition.h>
struct IntuitionBase *IntuitionBase;
int main(void){struct MsgPort *port=CreateMsgPort();struct IOStdReq *io=(void *)CreateIORequest(port,sizeof(*io));int n,a,b,qual=0;char op[20];struct InputEvent ev;struct IEPointerPixel pos;if(!port||!io||OpenDevice("input.device",0,(void *)io,0))return 20;IntuitionBase=(void *)OpenLibrary("intuition.library",39);for(n=0;n<36000;n++){FILE *f=fopen("SYS:input.command","r");if(f){a=b=0;fscanf(f,"%19s %d %d",op,&a,&b);fclose(f);DeleteFile("SYS:input.command");memset(&ev,0,sizeof(ev));if(!strcmp(op,"QUIT"))break;if(!strcmp(op,"EXEC")){BPTR out=Open("SYS:exec.log",MODE_NEWFILE);struct TagItem tags[]={{SYS_Input,0},{SYS_Output,out},{SYS_Error,out},{TAG_DONE,0}};SystemTagList("Execute SYS:exec.script",tags);if(out)Close(out);continue;}if(!strcmp(op,"MOVE")){ev.ie_Class=IECLASS_NEWPOINTERPOS;ev.ie_SubClass=IESUBCLASS_PIXEL;pos.iepp_Screen=IntuitionBase->FirstScreen;pos.iepp_Position.X=a;pos.iepp_Position.Y=b;ev.ie_EventAddress=&pos;}else if(!strcmp(op,"BUTTON")){ev.ie_Class=IECLASS_RAWMOUSE;ev.ie_Code=a;if(a==IECODE_LBUTTON)qual|=IEQUALIFIER_LEFTBUTTON;else qual&=~IEQUALIFIER_LEFTBUTTON;}else if(!strcmp(op,"KEY")){ev.ie_Class=IECLASS_RAWKEY;ev.ie_Code=a;}ev.ie_Qualifier=qual;io->io_Command=IND_WRITEEVENT;io->io_Data=&ev;io->io_Length=sizeof(ev);DoIO((void *)io);if(ev.ie_Class==IECLASS_RAWKEY){Delay(3);ev.ie_Code|=IECODE_UP_PREFIX;DoIO((void *)io);}}Delay(5);}CloseDevice((void *)io);DeleteIORequest(io);DeleteMsgPort(port);CloseLibrary((void *)IntuitionBase);return 0;}
