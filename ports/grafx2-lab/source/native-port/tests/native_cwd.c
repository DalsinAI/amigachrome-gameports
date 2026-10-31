/* GPL-2.0-only. Regression for libnix getcwd(NULL,0) buffer corruption. */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "global.h"
#include "io.h"
static size_t seen_size;
char *getcwd(char *buffer,size_t size)
{
 assert(buffer && size>0);seen_size=size;
 if(size<sizeof("DH0:GrafX2"))return NULL;
 memcpy(buffer,"DH0:GrafX2",sizeof("DH0:GrafX2"));return buffer;
}
void *GFX2_malloc_and_log(size_t size,const char *file,unsigned line){return malloc(size);}
int main(void)
{
 word *unicode=(void *)1;char *p=Get_current_directory(NULL,&unicode,0);
 assert(p&&!strcmp(p,"DH0:GrafX2")&&!unicode&&seen_size>=256);free(p);
 char small[4],buf[32];assert(!Get_current_directory(small,NULL,sizeof(small)));
 assert(Get_current_directory(buf,NULL,sizeof(buf))==buf&&!strcmp(buf,"DH0:GrafX2"));
 puts("native cwd: allocated buffer, explicit bounds and failure path passed");return 0;
}
