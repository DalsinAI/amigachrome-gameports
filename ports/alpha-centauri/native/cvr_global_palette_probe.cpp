/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_palette.h"
#include <stdio.h>
#include <string.h>
int main(int argc,char**argv){
 if(argc!=3)return 20;
 uint8_t input[768],output[768];FILE*f=fopen(argv[1],"rb");if(!f)return 20;const bool read=fread(input,1,sizeof(input),f)==sizeof(input) && fgetc(f)==EOF;fclose(f);if(!read)return 20;
 if(!alpha::build_cvr_default_global_palette(input,sizeof(input),output,sizeof(output)))return 20;
 f=fopen(argv[2],"wb");if(!f)return 20;const bool written=fwrite(output,1,sizeof(output),f)==sizeof(output);fclose(f);if(!written)return 20;
 memset(output,47,sizeof(output));
 if(alpha::build_cvr_default_global_palette(nullptr,768,output,768) || alpha::build_cvr_default_global_palette(input,767,output,768) || alpha::build_cvr_default_global_palette(input,769,output,768) || alpha::build_cvr_default_global_palette(input,768,nullptr,768) || alpha::build_cvr_default_global_palette(input,768,output,767) || alpha::build_cvr_default_global_palette(output,768,output,768))return 20;
 for(auto x:output)if(x!=47)return 20;
 puts("DEFAULT GLOBAL PALETTE 256entries invalid6 RC0");return 0;
}
