/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_flat_context.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char**argv){if(argc!=4)return 20;FILE*f=fopen(argv[1],"rb");if(!f)return 20;if(fseek(f,0,2)){fclose(f);return 20;}long n=ftell(f);rewind(f);if(n<=0||n>64*1024*1024){fclose(f);return 20;}auto*p=(uint8_t*)malloc(n);if(!p){fclose(f);return 20;}const bool ok=fread(p,1,n,f)==(size_t)n;fclose(f);alpha::OriginalWorld w;const bool loaded=ok&&w.load(p,n,1)==alpha::WorldLoadError::ok;free(p);if(!loaded)return 20;const int x=atoi(argv[2]),y=atoi(argv[3]);for(unsigned z=1;z<=2;++z){const auto r=alpha::cvr_source_flat_context(w,x,y,z);printf("%u %u %u %ld\n",z,(unsigned)r.supported,r.relative_corner,(long)r.corner_height);}return 0;}
