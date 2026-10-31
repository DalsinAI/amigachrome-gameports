#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/dos.h>
int main(void){BPTR f=Open("SYS:grafx2-run.log",MODE_NEWFILE);LONG rc;struct TagItem tags[]={{NP_StackSize,524288},{SYS_Input,0},{SYS_Output,f},{SYS_Error,f},{TAG_DONE,0}};if(!f)return 20;rc=SystemTagList("SYS:GrafX2/GrafX2 -verbose SYS:GrafX2/colour-fixture.png",tags);FPrintf(f,"\nEDITOR RETURN CODE: %ld\n",rc);Close(f);return rc;}
