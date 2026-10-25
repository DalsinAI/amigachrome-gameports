/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "rules_text.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char** argv){
 if(argc!=2)return 20;
 FILE* f=fopen(argv[1],"rb");if(!f)return 20;
 if(fseek(f,0,SEEK_END)){fclose(f);return 20;}long size=ftell(f);rewind(f);
 if(size<0 || size>1048576){fclose(f);return 20;}auto* data=(uint8_t*)malloc((size_t)size+1);
 if(!data){fclose(f);return 20;}bool read=fread(data,1,(size_t)size,f)==(size_t)size;fclose(f);
 alpha::ResourceInfo out;memset(&out,0x55,sizeof(out));alpha::ResourceInfo before=out;
 auto error=read?alpha::read_resource_info(data,(size_t)size,out):alpha::Error::bad_row;free(data);
 if(error!=alpha::Error::ok){if(memcmp(&out,&before,sizeof(out)))return 21;printf("ERROR %d\n",(int)error);return 20;}
 for(unsigned i=0;i<alpha::resource_info_count;++i){auto& r=out.rows[i];printf("%u %ld %ld %ld %ld\n",i,(long)r.nutrients,(long)r.minerals,(long)r.energy,(long)r.unused);}return 0;
}
