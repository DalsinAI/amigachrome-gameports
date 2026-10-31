#include <stdio.h>
#include <proto/exec.h>
int main(void){struct Library *b=OpenLibrary("SYS:Libs/datatypes.library",47);printf("OpenDatatypes explicit load: %p\n",b);return b?0:20;}
