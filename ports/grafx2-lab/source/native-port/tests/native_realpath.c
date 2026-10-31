/* Native file/directory canonicalization, without chdir or a short buffer. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _NO_INLINE
#define USE_AMIGA_NATIVE
#include "realpath.c"
static LONG error;static int missing,unlocks,calls;static const char *answer;
BPTR Lock(CONST_STRPTR p,LONG m){return missing?0:1;}
void UnLock(BPTR b){unlocks++;}
LONG IoErr(void){return error;}
LONG SetIoErr(LONG e){LONG old=error;error=e;return old;}
LONG NameFromLock(BPTR b,STRPTR p,LONG n){calls++;if(strlen(answer)+1>(unsigned long)n){error=ERROR_LINE_TOO_LONG;return 0;}strcpy((char *)p,answer);return 1;}
int main(void){char *p;answer="Work:folder/drawing.png";p=Realpath("drawing.png");assert(p&&!strcmp(p,answer)&&unlocks==1);free(p);
 answer="Work:";p=Realpath("Work:");assert(p&&!strcmp(p,"Work:")&&unlocks==2);free(p);
 char big[3000];memset(big,'x',sizeof(big)-1);big[sizeof(big)-1]=0;answer=big;calls=0;p=Realpath("long");assert(p&&strlen(p)==2999&&calls==3&&unlocks==3);free(p);
 missing=1;assert(!Realpath("missing")&&errno==ENOENT&&unlocks==3);assert(!Realpath(NULL)&&errno==EINVAL);
 puts("native realpath: files, volume roots, resizing and failure paths PASS");return 0;}
