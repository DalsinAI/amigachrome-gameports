/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "startup_movie.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char** argv){
 if(argc!=2)return 20;
 FILE* f=fopen(argv[1],"rb");if(!f)return 20;uint8_t header[40];alpha::StartupMovieInfo m;long n=-1;if(!fseek(f,0,SEEK_END))n=ftell(f);rewind(f);
 if(n<40 || fread(header,1,40,f)!=40 || !alpha::startup_movie_header(header,40,uint64_t(n),m)){fclose(f);return 20;}
 auto* yuv=static_cast<uint8_t*>(malloc(m.video_bytes));size_t bytes=size_t(m.width)*m.height*3;auto* rgb=static_cast<uint8_t*>(malloc(bytes));int rc=20;
 if(yuv && rgb && fread(yuv,1,m.video_bytes,f)==m.video_bytes && alpha::startup_movie_rgb(m,yuv,m.video_bytes,rgb,bytes) && fwrite(rgb,1,bytes,stdout)==bytes)rc=0;
 free(yuv);free(rgb);fclose(f);return rc;
}
