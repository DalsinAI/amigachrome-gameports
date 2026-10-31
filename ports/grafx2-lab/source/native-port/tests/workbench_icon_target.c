/* GPL-2.0-only: ask the running Workbench to launch the packaged tool icon. */
#define timerequest TimeRequest
#include <stdio.h>
#include <proto/exec.h>
#include <proto/wb.h>
struct Library *WorkbenchBase;
int main(void)
{
  BOOL ok;
  WorkbenchBase=OpenLibrary("workbench.library",44);
  if(!WorkbenchBase)return 20;
  ok=OpenWorkbenchObjectA((CONST_STRPTR)"SYS:GrafX2/GrafX2",NULL);
  printf("WORKBENCH ICON OPEN: %s\n",ok?"PASS":"FAIL");
  CloseLibrary(WorkbenchBase);
  return ok?0:20;
}
